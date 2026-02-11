#pragma once

#include "../DSP/ShineParams.h"
#include "ShineFeatureExtractor.h"
#include "ShineModelWeights.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace shine {

class ShineModelInference {
public:
    // Run inference on 30 input features, returns ShineParams
    ShineParams predict(const std::array<float, ShineFeatureExtractor::TotalFeatures>& features) {
        // Normalize input
        std::array<float, 30> normalized;
        for (int i = 0; i < 30; ++i) {
            float std_val = weights::X_std[i];
            if (std_val < 1e-8f) std_val = 1e-8f;
            normalized[i] = (features[i] - weights::X_mean[i]) / std_val;
        }

        // Layer 1: 30 -> 64 + LeakyReLU
        std::array<float, 64> h1;
        linearLayer<30, 64>(normalized.data(), h1.data(), weights::layer1_weight, weights::layer1_bias);
        leakyRelu(h1.data(), 64);

        // Layer 2: 64 -> 64 + LeakyReLU
        std::array<float, 64> h2;
        linearLayer<64, 64>(h1.data(), h2.data(), weights::layer2_weight, weights::layer2_bias);
        leakyRelu(h2.data(), 64);

        // Layer 3: 64 -> 32 + LeakyReLU
        std::array<float, 32> h3;
        linearLayer<64, 32>(h2.data(), h3.data(), weights::layer3_weight, weights::layer3_bias);
        leakyRelu(h3.data(), 32);

        // Output: 32 -> 2
        std::array<float, 2> output;
        linearLayer<32, 2>(h3.data(), output.data(), weights::output_weight, weights::output_bias);

        // NaN protection + clamp to valid ranges
        ShineParams params;
        params.presence = (std::isnan(output[0]) || std::isinf(output[0])) ? 0.0f : std::clamp(output[0], 0.0f, 6.0f);
        params.air = (std::isnan(output[1]) || std::isinf(output[1])) ? 0.0f : std::clamp(output[1], 0.0f, 1.0f);

        return params;
    }

    // Smoothed prediction for real-time use
    ShineParams getSmoothedParams(const std::array<float, ShineFeatureExtractor::TotalFeatures>& features) {
        ShineParams predicted = predict(features);

        smoothedPresence = smoothingFactor * smoothedPresence + (1.0f - smoothingFactor) * predicted.presence;
        smoothedAir = smoothingFactor * smoothedAir + (1.0f - smoothingFactor) * predicted.air;

        ShineParams result;
        result.presence = smoothedPresence;
        result.air = smoothedAir;
        return result;
    }

    void resetSmoothing() {
        smoothedPresence = 0.0f;
        smoothedAir = 0.0f;
    }

private:
    float smoothingFactor = 0.85f;
    float smoothedPresence = 0.0f;
    float smoothedAir = 0.0f;

    template<int InSize, int OutSize>
    static void linearLayer(const float* input, float* output, const float* weight, const float* bias) {
        for (int o = 0; o < OutSize; ++o) {
            float sum = bias[o];
            for (int i = 0; i < InSize; ++i) {
                sum += weight[o * InSize + i] * input[i];
            }
            output[o] = sum;
        }
    }

    static void leakyRelu(float* data, int size, float negativeSlope = 0.01f) {
        for (int i = 0; i < size; ++i) {
            if (data[i] < 0.0f)
                data[i] *= negativeSlope;
        }
    }
};

} // namespace shine
