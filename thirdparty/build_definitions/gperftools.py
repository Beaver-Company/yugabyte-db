#
# Copyright (c) YugaByte, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
# in compliance with the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License
# is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
# or implied. See the License for the specific language governing permissions and limitations
# under the License.
#

import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from build_definitions import *

class GPerfToolsDependency(Dependency):
    def __init__(self):
        super(GPerfToolsDependency, self).__init__(
                'gperftools', '2.2.1',
                'https://github.com/gperftools/gperftools/releases/download/gperftools-{0}/'
                        'gperftools-{0}.tar.gz',
                BUILD_GROUP_INSTRUMENTED)
        self.copy_sources = True
        self.patch_version = 4
        self.patch_strip = 1
        self.patches = ['gperftools-Change-default-TCMALLOC_TRANSFER_NUM_OBJ-to-40.patch',
                        'gperftools-hook-mi_force_unlock-on-OSX-instead-of-pthread_atfork.patch',
                        'gperftools-Fix-finding-default-zone-on-Mac-Sierra.patch',
                        'gperftools-fix-sanitized-debugallocation.patch']
        self.post_patch = ['autoreconf', '-fvi']

    def build(self, builder):
        log_prefix = builder.log_prefix(self)
        os.environ["YB_REMOTE_BUILD"] = "0"
        log_output(log_prefix, ['./configure', '--prefix={}'.format(builder.prefix),
                                '--enable-frame-pointers', '--enable-heap-checker', '--with-pic'])
        log_output(log_prefix, ['make', 'clean'])
        log_output(log_prefix, ['make', 'install', '-j', '1'])

    def should_build(self, skip_build, instrumented):
        return not skip_build and not instrumented
