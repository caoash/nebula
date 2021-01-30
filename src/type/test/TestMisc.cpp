/*
 * Copyright 2017-present varchar.io
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace nebula {
namespace type {
namespace test {
TEST(MiscTest, StringUtf8) {
  std::string str1 = "abcd";
  EXPECT_EQ(str1.size(), 4);

  std::string str2 = "星云";
  EXPECT_EQ(str2.size(), 6);
  for (size_t i = 0; i < str2.size(); ++i) {
    LOG(INFO) << "CHAR: " << ((int)str2.at(i));
  }
}
} // namespace test
} // namespace type
} // namespace nebula
