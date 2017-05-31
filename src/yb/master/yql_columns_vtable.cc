// Copyright (c) YugaByte, Inc.

#include "yb/common/yql_value.h"
#include "yb/master/catalog_manager.h"
#include "yb/master/yql_columns_vtable.h"

namespace yb {
namespace master {

YQLColumnsVTable::YQLColumnsVTable(const Master* const master)
    : YQLVirtualTable(master::kSystemSchemaColumnsTableName, master, CreateSchema()) {
}

Status YQLColumnsVTable::PopulateColumnInformation(const Schema& schema,
                                                   const string& keyspace_name,
                                                   const string& table_name,
                                                   const size_t col_idx,
                                                   YQLRow* const row) const {
  RETURN_NOT_OK(SetColumnValue(kKeyspaceName, keyspace_name, row));
  RETURN_NOT_OK(SetColumnValue(kTableName, table_name, row));
  RETURN_NOT_OK(SetColumnValue(kColumnName, schema.column(col_idx).name(), row));
  RETURN_NOT_OK(SetColumnValue(kClusteringOrder, schema.column(col_idx).sorting_type_string(),
                               row));
  RETURN_NOT_OK(SetColumnValue(kType, schema.column(col_idx).type()->ToString(), row));
  return Status::OK();
}

Status YQLColumnsVTable::RetrieveData(const YQLReadRequestPB& request,
                                      std::unique_ptr<YQLRowBlock>* vtable) const {
  vtable->reset(new YQLRowBlock(schema_));
  std::vector<scoped_refptr<TableInfo> > tables;
  master_->catalog_manager()->GetAllTables(&tables, true);
  for (scoped_refptr<TableInfo> table : tables) {
    Schema schema;
    RETURN_NOT_OK(table->GetSchema(&schema));

    // Get namespace for table.
    NamespaceIdentifierPB nsId;
    nsId.set_id(table->namespace_id());
    scoped_refptr<NamespaceInfo> nsInfo;
    RETURN_NOT_OK(master_->catalog_manager()->FindNamespace(nsId, &nsInfo));

    const string& keyspace_name = nsInfo->name();
    const string& table_name = table->name();

    // Fill in the hash keys first.
    int32_t num_hash_columns = schema.num_hash_key_columns();
    for (int32_t i = 0; i < num_hash_columns; i++) {
      YQLRow& row = (*vtable)->Extend();
      RETURN_NOT_OK(PopulateColumnInformation(schema, keyspace_name, table_name, i,
                                              &row));
      // kind (always partition_key for hash columns)
      RETURN_NOT_OK(SetColumnValue(kKind, "partition_key", &row));
      RETURN_NOT_OK(SetColumnValue(kPosition, i, &row));
    }

    // Now fill in the range columns
    int32_t num_range_columns = schema.num_range_key_columns();
    for (int32_t i = num_hash_columns; i < num_hash_columns + num_range_columns; i++) {
      YQLRow& row = (*vtable)->Extend();
      RETURN_NOT_OK(PopulateColumnInformation(schema, keyspace_name, table_name, i,
                                              &row));
      // kind (always clustering for range columns)
      RETURN_NOT_OK(SetColumnValue(kKind, "clustering", &row));
      RETURN_NOT_OK(SetColumnValue(kPosition, i - num_hash_columns, &row));
    }

    // Now fill in the rest of the columns.
    for (int32_t i = num_hash_columns + num_range_columns; i < schema.num_columns(); i++) {
      YQLRow &row = (*vtable)->Extend();
      RETURN_NOT_OK(PopulateColumnInformation(schema, keyspace_name, table_name, i,
                                              &row));
      // kind (always regular for regular columns)
      RETURN_NOT_OK(SetColumnValue(kKind, "regular", &row));
      RETURN_NOT_OK(SetColumnValue(kPosition, -1, &row));
    }
  }

  return Status::OK();
}

Schema YQLColumnsVTable::CreateSchema() const {
  SchemaBuilder builder;
  CHECK_OK(builder.AddKeyColumn(kKeyspaceName, DataType::STRING));
  CHECK_OK(builder.AddKeyColumn(kTableName, DataType::STRING));
  CHECK_OK(builder.AddKeyColumn(kColumnName, DataType::STRING));
  CHECK_OK(builder.AddColumn(kClusteringOrder, DataType::STRING));
  CHECK_OK(builder.AddColumn(kColumnNameBytes, DataType::BINARY));
  CHECK_OK(builder.AddColumn(kKind, DataType::STRING));
  CHECK_OK(builder.AddColumn(kPosition, DataType::INT32));
  CHECK_OK(builder.AddColumn(kType, DataType::STRING));
  return builder.Build();
}

}  // namespace master
}  // namespace yb
