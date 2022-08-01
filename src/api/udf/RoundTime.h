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

#pragma once

#include <fmt/format.h>
#include "common/Evidence.h"

#include "surface/eval/UDF.h"

/**
 * Define expressions used in the nebula DSL.
 */
namespace nebula {
namespace api {
namespace udf {
// Round a time
using UdfRoundBase = nebula::surface::eval::UDF<nebula::type::Kind::BIGINT, nebula::type::Kind::BIGINT>;
class RoundTime : public UdfRoundBase {
  using BigIntType = typename nebula::type::TypeTraits<nebula::type::Kind::BIGINT>::CppType;

public:
  RoundTime(const std::string& name,
            std::unique_ptr<nebula::surface::eval::ValueEval> expr,
            BigIntType unit,
            BigIntType beginTime)
    : UdfRoundBase(
      name,
      std::move(expr),
      [unit, beginTime](const std::optional<InputType>& origin) -> std::optional<int64_t> {
        if (N_UNLIKELY(origin == std::nullopt)) {
          return std::nullopt;
        }

        static constexpr int32_t HOUR_CASE = 1;
        static constexpr int32_t DAY_CASE = 2;
        static constexpr int32_t WEEK_CASE = 3;
        static constexpr int32_t MONTH_CASE = 4;
        static constexpr int32_t QUARTER_CASE = 5;
        static constexpr int32_t YEAR_CASE = 6;

        auto timeSecs = origin.value();
        LOG(INFO) << timeSecs;
        BigIntType res = std::numeric_limits<int8_t>::min();
        std::time_t timePoint = (std::time_t)timeSecs;
        std::time_t roundedPoint = (time_t) -1;
        switch (unit) {
        case HOUR_CASE: {
          roundedPoint = nebula::common::Evidence::hour(timePoint);
          break;
        }
        case DAY_CASE: {
          roundedPoint = nebula::common::Evidence::date(timePoint);
          break;
        }
        case WEEK_CASE: {
          roundedPoint = nebula::common::Evidence::week(timePoint);
          break;
        }
        case MONTH_CASE: {
          roundedPoint = nebula::common::Evidence::month(timePoint);
          break;
        }
        case QUARTER_CASE: {
          roundedPoint = nebula::common::Evidence::quarter(timePoint);
          break;
        }
        case YEAR_CASE: {
          roundedPoint = nebula::common::Evidence::year(timePoint);
          break;
        }
        }
        // LOG(INFO) << nebula::common::Evidence::fmt_normal(timeSecs);
        auto form = nebula::common::Evidence::fmt_normal(roundedPoint);
        N_ENSURE_NE(roundedPoint, (time_t) -1, "rounded point not changed");
        res = (BigIntType)roundedPoint;
        N_ENSURE_NE(res, std::numeric_limits<BigIntType>::lowest(), "res should be defined");

        // rounding sometimes produces a time value before beginTime (EX: beginTime is 3/14, rounding produces 3/1)
        // since values can be negative, we take a max to ensure the answer is at least 0
        BigIntType minValue = (BigIntType)(0);

        return std::max(minValue, res - beginTime);
      }) {}
  virtual ~RoundTime() = default;
};

} // namespace udf
} // namespace api
} // namespace nebula