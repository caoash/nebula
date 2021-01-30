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
#include <valarray>

#include "common/Memory.h"
#include "fmt/format.h"
#include "memory/keyed/FlatBuffer.h"
#include "memory/keyed/HashFlat.h"
#include "meta/TestTable.h"
#include "surface/DataSurface.h"
#include "surface/MockSurface.h"
#include "surface/StaticData.h"
#include "type/Serde.h"

/**
 * Flat Buffer is used to store / compute run time data. 
 * Test its interfaces and functions here.
 */
namespace nebula {
namespace memory {
namespace test {

using nebula::common::Evidence;
using nebula::memory::keyed::FlatBuffer;
using nebula::memory::keyed::HashFlat;
using nebula::surface::MockRowData;
using nebula::surface::RowData;
using nebula::type::TypeSerializer;

// print single row as string.
static constexpr auto line = [](const RowData& r) {
  std::string s;
  if (!r.isNull("items")) {
    const auto list = r.readList("items");
    for (auto k = 0; k < list->getItems(); ++k) {
      s += (list->isNull(k) ? "NULL" : fmt::format("{0},", list->readString(k)));
    }
  }

  return fmt::format("({0}, {1}, [{2}], {3})",
                     r.isNull("id") ? 0 : r.readInt("id"),
                     r.isNull("event") ? "NULL" : r.readString("event"),
                     s,
                     r.isNull("flag") ? true : r.readBool("flag"));
};

TEST(FlatBufferTest, TestFlatBufferWrite) {
  nebula::meta::TestTable test;

  // initialize a flat row with given schema
  FlatBuffer fb(test.schema(), test.testFields());

  // add 10 rows
  constexpr auto rows2test = 1024;

  auto seed = Evidence::unix_timestamp();
  MockRowData row(seed);
  std::vector<nebula::surface::StaticRow> rows;
  // fill rows
  for (auto i = 0; i < rows2test; ++i) {
    rows.push_back({ row.readLong("_time_"),
                     row.readInt("id"),
                     row.readString("event"),
                     row.readList("items"),
                     // row.isNull("items") ? nullptr : row.readList("items"),
                     row.readBool("flag"),
                     row.readByte("value"),
                     row.readInt128("i128"),
                     row.readDouble("weight") });
  }

  LOG(INFO) << "Data was generated with seed: " << seed;

  // write the data into the flat buffer
  for (auto i = 0; i < rows2test; ++i) {
    fb.add(rows[i]);
  }

  // read it back
  EXPECT_EQ(fb.getRows(), rows2test);
  LOG(INFO) << "Flat buffer has rows:" << fb.getRows();
  for (auto i = 0; i < rows2test; ++i) {
    const auto& r = fb.row(i);
    EXPECT_EQ(line(r), line(rows[i]));
  }
}

TEST(FlatBufferTest, TestRollback) {
  nebula::meta::TestTable test;

  // initialize a flat row with given schema
  FlatBuffer fb(test.schema(), test.testFields());

  // add 10 rows
  constexpr auto rows2test = 5;
  auto seed = Evidence::unix_timestamp();
  MockRowData row(seed);

  // add 5 rows
  for (auto i = 0; i < rows2test; ++i) {
    fb.add(row);
  }

  std::vector<std::string> lines;
  lines.reserve(rows2test);
  for (auto i = 0; i < rows2test; ++i) {
    lines.push_back(line(fb.row(i)));
  }

  EXPECT_EQ(fb.getRows(), rows2test);
  // rollback last one
  fb.rollback();
  EXPECT_EQ(fb.getRows(), rows2test - 1);

  // rollback every new row
  for (auto i = 0; i < rows2test; ++i) {
    fb.add(row);
    fb.rollback();
  }

  // last row is different
  fb.add(row);

  EXPECT_EQ(fb.getRows(), rows2test);
  for (auto i = 0; i < rows2test - 1; ++i) {
    EXPECT_EQ(line(fb.row(i)), lines[i]);
  }
}

TEST(FlatBufferTest, TestSerde) {
  nebula::meta::TestTable test;

  // initialize a flat row with given schema
  FlatBuffer fb(test.schema(), test.testFields());

  // add some rows
  constexpr auto rows2test = 21053;
  auto seed = Evidence::unix_timestamp();
  MockRowData row(seed);

  // add 5 rows
  for (auto i = 0; i < rows2test; ++i) {
    fb.add(row);
  }

  EXPECT_EQ(fb.getRows(), rows2test);

  auto size = fb.prepareSerde();
  auto buffer = static_cast<NByte*>(nebula::common::Pool::getDefault().allocate(size));

  // serialize size should equal expected bin size
  EXPECT_EQ(size, fb.serialize(buffer));

  // deserialize this data into another flat buffer
  FlatBuffer fb2(test.schema(), test.testFields(), buffer);

  // check these two buffers are exactly the same
  EXPECT_EQ(fb2.getRows(), rows2test);

  // check every single row are the same
  for (auto i = 0; i < rows2test; ++i) {
    const auto& r = fb.row(i);
    const auto& r2 = fb2.row(i);
    EXPECT_EQ(line(r), line(r2));
  }

  // DO NOT release buffer as it's owned by flatbuffer
  // it has moved in semantic and own it
  // delete[] buffer;
}

TEST(FlatBufferTest, TestHashFlatSerde) {
  auto schema = TypeSerializer::from("ROW<id:int, count:int>");

  // initialize a flat row with given schema
  auto c1 = nebula::surface::eval::constant(1);
  auto c2 = nebula::surface::eval::constant(2);
  nebula::surface::eval::Fields f;
  f.reserve(2);
  f.emplace_back(std::move(c1));
  f.emplace_back(std::move(c2));
  HashFlat hf(schema, f);

  // add some rows
  constexpr auto rows2test = 101053;
  auto seed = Evidence::unix_timestamp();
  // seed = 1583448824;
  // LOG(INFO) << "seed: " << seed;
  MockRowData row(seed);

  // add 5 rows
  for (auto i = 0; i < rows2test; ++i) {
    hf.add(row);
  }

  EXPECT_EQ(hf.getRows(), rows2test);

  auto size = hf.prepareSerde();
  auto buffer = static_cast<NByte*>(nebula::common::Pool::getDefault().allocate(size));

  // serialize size should equal expected bin size
  EXPECT_EQ(size, hf.serialize(buffer));

  // deserialize this data into another flat buffer
  FlatBuffer fb2(schema, f, buffer);

  // check these two buffers are exactly the same
  EXPECT_EQ(fb2.getRows(), rows2test);

#define VERIFY_COL(C)                         \
  {                                           \
    auto rn = r.isNull(C);                    \
    auto rn2 = r2.isNull(C);                  \
    EXPECT_EQ(rn, rn2);                       \
    if (!rn) {                                \
      EXPECT_EQ(r.readInt(C), r2.readInt(C)); \
    }                                         \
  }

  // check every single row are the same
  for (auto i = 0; i < rows2test; ++i) {
    const auto& r = hf.row(i);
    const auto& r2 = fb2.row(i);

    VERIFY_COL(0)
    VERIFY_COL(1)
  }
}

} // namespace test
} // namespace memory
} // namespace nebula