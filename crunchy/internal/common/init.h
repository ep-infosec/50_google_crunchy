// Copyright 2017 The CrunchyCrypt Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRUNCHY_INTERNAL_COMMON_INIT_H_
#define CRUNCHY_INTERNAL_COMMON_INIT_H_
#include <gtest/gtest.h>

namespace crunchy {

inline void InitCrunchyTest(const char* usage, int* argc, char*** argv,
                            bool remove_flags) {
  ::testing::InitGoogleTest(argc, *argv);
}

}  // namespace crunchy

#endif  // CRUNCHY_INTERNAL_COMMON_INIT_H_
