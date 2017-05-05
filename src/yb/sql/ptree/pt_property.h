// Copyright (c) YugaByte, Inc.

#ifndef YB_SQL_PTREE_PT_PROPERTY_H_
#define YB_SQL_PTREE_PT_PROPERTY_H_

#include "yb/gutil/strings/substitute.h"
#include "yb/master/master.pb.h"
#include "yb/sql/ptree/list_node.h"
#include "yb/sql/ptree/pt_expr.h"
#include "yb/sql/ptree/tree_node.h"

namespace yb {
namespace sql {

static const auto invalid_argument_len = std::strlen("Invalid argument: ");
#define RETURN_SEM_CONTEXT_ERROR_NOT_OK(s) do { \
    ::yb::Status _s = (s); \
    if (PREDICT_FALSE(!_s.ok())) { \
      auto err_str = s.ToString(false); \
      err_str.replace(0, invalid_argument_len, ""); \
      return sem_context->Error(loc(), err_str.c_str()); \
    } \
  } while (0)

class PTProperty : public TreeNode {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTProperty> SharedPtr;
  typedef MCSharedPtr<const PTProperty> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructors and destructor.
  PTProperty(MemoryContext *memctx,
             YBLocation::SharedPtr loc,
             const MCString::SharedPtr& lhs_,
             const PTExpr::SharedPtr& rhs_);

  PTProperty(MemoryContext *memctx,
             YBLocation::SharedPtr loc);

  virtual ~PTProperty();

  template<typename... TypeArgs>
  inline static PTProperty::SharedPtr MakeShared(MemoryContext *memctx,
                                                 TypeArgs&&... args) {
    return MCMakeShared<PTProperty>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Node semantics analysis.
  virtual CHECKED_STATUS Analyze(SemContext *sem_context) = 0;

  MCString::SharedPtr lhs() const {
    return lhs_;
  }

  PTExpr::SharedPtr rhs() const {
    return rhs_;
  }

  static CHECKED_STATUS GetIntValueFromExpr(PTExpr::SharedPtr expr,
                                            const string& property_name,
                                            int64_t *val);

  static CHECKED_STATUS GetDoubleValueFromExpr(PTExpr::SharedPtr expr,
                                               const string& property_name,
                                               double *val);

  static CHECKED_STATUS GetBoolValueFromExpr(PTExpr::SharedPtr expr,
                                             const string& property_name,
                                             bool *val);

  static CHECKED_STATUS GetStringValueFromExpr(PTExpr::SharedPtr expr,
                                               bool to_lower_case,
                                               const string& property_name,
                                               string *val);

  // Returns true if s ends with substring end, and s has at least one more character before
  // end. If left is a valid string pointer, it will contain s minus the end substring.
  // Example 1: s = "15ms", end = "ms", then this function will return true and set left to "15".
  // Example 2: s = "ms", end = "ms", this function will return false.
  static bool StringEndsWith(const string& s, const char *end, size_t end_len, string *left);

 protected:
  // Parts of an expression 'lhs_ = rhs_' where lhs stands for left-hand side, and rhs for
  // right-hand side.
  MCString::SharedPtr lhs_;
  PTExpr::SharedPtr rhs_;
};

class PTPropertyListNode : public TreeListNode<PTProperty> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTPropertyListNode> SharedPtr;
  typedef MCSharedPtr<const PTPropertyListNode> SharedPtrConst;

  explicit PTPropertyListNode(MemoryContext *memory_context,
                                   YBLocation::SharedPtr loc,
                                   const MCSharedPtr<PTProperty>& tnode = nullptr)
      : TreeListNode<PTProperty>(memory_context, loc, tnode) {
  }

  virtual ~PTPropertyListNode() {
  }

  // Append a PTPropertyList to this list.
  void AppendList(const MCSharedPtr<PTPropertyListNode>& tnode_list) {
    if (tnode_list == nullptr) {
      return;
    }
    for (const auto tnode : tnode_list->node_list()) {
      Append(tnode);
    }
  }

  template<typename... TypeArgs>
  inline static PTPropertyListNode::SharedPtr MakeShared(MemoryContext *memctx,
                                                              TypeArgs&&...args) {
    return MCMakeShared<PTPropertyListNode>(memctx, std::forward<TypeArgs>(args)...);
  }

  virtual CHECKED_STATUS Analyze(SemContext *sem_context) override;
};

} // namespace sql
} // namespace yb

#endif // YB_SQL_PTREE_PT_PROPERTY_H_
