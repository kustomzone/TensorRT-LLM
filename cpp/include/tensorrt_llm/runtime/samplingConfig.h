/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "tensorrt_llm/runtime/common.h"

#include <optional>
#include <vector>

namespace tensorrt_llm::runtime
{

class SamplingConfig
{
private:
    using FloatType = float;

    template <typename T>
    using OptVec = std::optional<std::vector<T>>;

private:
    template <typename T>
    static OptVec<T> fuseValues(
        std::vector<SamplingConfig> const& configs, std::function<OptVec<T>(SizeType ci)> accessor)
    {
        std::vector<T> values;
        auto const hasValues = accessor(0).has_value();
        for (size_t ci = 0; ci < configs.size(); ++ci)
        {
            const auto& configValue = accessor(ci);
            TLLM_CHECK(hasValues == configValue.has_value());
            if (hasValues)
            {
                TLLM_CHECK(configValue.value().size() == 1);
                values.push_back(configValue.value().front());
            }
        }

        if (!hasValues)
        {
            return std::nullopt;
        }
        return std::make_optional<std::vector<T>>(values);
    }

public:
    explicit SamplingConfig(SizeType beamWidth = 1)
        : beamWidth{beamWidth}
    {
    }

    explicit SamplingConfig(std::vector<SamplingConfig> const& configs)
    {
        TLLM_CHECK(configs.size() > 0);
        beamWidth = configs.front().beamWidth;
        temperature = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].temperature; });
        minLength = fuseValues<SizeType>(configs, [&configs](SizeType ci) { return configs[ci].minLength; });
        repetitionPenalty
            = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].repetitionPenalty; });
        presencePenalty
            = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].presencePenalty; });
        topK = fuseValues<SizeType>(configs, [&configs](SizeType ci) { return configs[ci].topK; });
        topP = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].topP; });
        randomSeed = fuseValues<uint64_t>(configs, [&configs](SizeType ci) { return configs[ci].randomSeed; });
        topPDecay = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].topPDecay; });
        topPMin = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].topPMin; });
        topPResetIds = fuseValues<SizeType>(configs, [&configs](SizeType ci) { return configs[ci].topPResetIds; });
        beamSearchDiversityRate
            = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].beamSearchDiversityRate; });
        lengthPenalty = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].lengthPenalty; });
        draftAcceptanceThreshold
            = fuseValues<FloatType>(configs, [&configs](SizeType ci) { return configs[ci].draftAcceptanceThreshold; });
    }

public:
    SizeType beamWidth;

    OptVec<FloatType> temperature;       // [1] or [batch_size] on cpu
    OptVec<SizeType> minLength;          // [1] or [batch_size] on cpu
    OptVec<FloatType> repetitionPenalty; // [1] or [batch_size] on cpu
    OptVec<FloatType> presencePenalty;   // [1] or [batch_size] on cpu
    OptVec<FloatType> frequencyPenalty;  // [1] or [batch_size] on cpu

    // sampling layers
    OptVec<SizeType> topK;         // [1] or [batch_size] on cpu
    OptVec<FloatType> topP;        // [1] or [batch_size] on cpu
    OptVec<uint64_t> randomSeed;   // [1] or [batch_size] on cpu
    OptVec<FloatType> topPDecay;   // [batch_size], must between [0, 1]
    OptVec<FloatType> topPMin;     // [batch_size], must between [0, 1]
    OptVec<SizeType> topPResetIds; // [batch_size]

    // beam search layer
    OptVec<FloatType> beamSearchDiversityRate;
    OptVec<FloatType> lengthPenalty;

    // speculative decoding
    OptVec<FloatType> draftAcceptanceThreshold;

    std::optional<bool> normalizeLogProbs;
};

} // namespace tensorrt_llm::runtime
