#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace shine {

class ShineFeatureExtractor {
public:
    static constexpr int FeaturesPerFrame = 5;
    static constexpr int ContextFrames = 6;  // current + 5 history
    static constexpr int TotalFeatures = FeaturesPerFrame * ContextFrames; // 30

    // Extract 5 features from a single audio frame
    std::array<float, FeaturesPerFrame> extractFrame(const float* samples, int numSamples) {
        std::array<float, FeaturesPerFrame> features{};

        if (numSamples == 0) return features;

        // 1. RMS
        float sumSq = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSq += samples[i] * samples[i];
        features[0] = std::sqrt(sumSq / static_cast<float>(numSamples));

        // 2. Spectral centroid (approximate via zero-crossing weighted)
        float weightedSum = 0.0f;
        float totalMag = 0.0f;
        for (int i = 1; i < numSamples; ++i) {
            float mag = std::abs(samples[i]);
            weightedSum += static_cast<float>(i) * mag;
            totalMag += mag;
        }
        features[1] = (totalMag > 1e-10f) ? (weightedSum / totalMag) / static_cast<float>(numSamples) : 0.0f;

        // 3. Spectral tilt (energy ratio high vs low)
        int mid = numSamples / 2;
        float lowEnergy = 0.0f, highEnergy = 0.0f;
        for (int i = 0; i < mid; ++i)
            lowEnergy += samples[i] * samples[i];
        for (int i = mid; i < numSamples; ++i)
            highEnergy += samples[i] * samples[i];
        features[2] = (lowEnergy > 1e-10f) ? highEnergy / (lowEnergy + 1e-10f) : 0.0f;

        // 4. Zero crossing rate
        int crossings = 0;
        for (int i = 1; i < numSamples; ++i) {
            if ((samples[i] >= 0.0f) != (samples[i - 1] >= 0.0f))
                ++crossings;
        }
        features[3] = static_cast<float>(crossings) / static_cast<float>(numSamples - 1);

        // 5. Crest factor
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = std::max(peak, std::abs(samples[i]));
        features[4] = (features[0] > 1e-10f) ? peak / features[0] : 0.0f;

        return features;
    }

    // Extract features with context window, returns 30-element feature vector
    std::array<float, TotalFeatures> extract(const float* samples, int numSamples) {
        auto frameFeatures = extractFrame(samples, numSamples);

        // Shift history
        for (int i = 0; i < ContextFrames - 1; ++i)
            history[i] = history[i + 1];
        history[ContextFrames - 1] = frameFeatures;

        // Flatten context into single vector
        std::array<float, TotalFeatures> result{};
        for (int i = 0; i < ContextFrames; ++i)
            for (int j = 0; j < FeaturesPerFrame; ++j)
                result[i * FeaturesPerFrame + j] = history[i][j];

        return result;
    }

    void reset() {
        for (auto& h : history)
            h.fill(0.0f);
    }

private:
    std::array<std::array<float, FeaturesPerFrame>, ContextFrames> history{};
};

} // namespace shine
