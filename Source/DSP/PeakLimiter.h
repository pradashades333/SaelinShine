#pragma once

#include <cmath>
#include <algorithm>

namespace shine {

class PeakLimiter {
public:
    void prepare(double sampleRate) {
        attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.0001f));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.050f));
        envelope = 0.0f;
    }

    void process(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float absSample = std::abs(buffer[i]);

            if (absSample > envelope)
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * absSample;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * absSample;

            if (envelope > threshold) {
                float gain = threshold / envelope;
                buffer[i] *= gain;
            }
        }
    }

    void reset() {
        envelope = 0.0f;
    }

private:
    float threshold = 1.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
};

} // namespace shine
