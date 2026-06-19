#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

namespace shine {

class SaelinAdaptiveEngine {
public:
    struct ParameterRange {
        float min;
        float max;
    };

    void prepare(double sampleRate, int maxBlockSize) {
        sr = sampleRate;
        slowEnvCoeff = std::exp(-1.0f / (static_cast<float>(sr) * 0.300f));
        outputSmoothCoeff = std::exp(-1.0f / (static_cast<float>(sr) * 0.280f));
        gateCoeff = std::exp(-1.0f / (static_cast<float>(sr) * 0.050f));

        slowEnvelope = 0.0f;
        gateGain = 0.0f;

        for (auto& p : params) {
            p.modulatedBuffer.resize(static_cast<size_t>(maxBlockSize), p.baseValue);
            p.smoothedOutput = p.baseValue;
        }
    }

    int registerParameter(const std::string& name, float baseValue,
                          ParameterRange range, float modulationDepth, bool inverted) {
        int idx = static_cast<int>(params.size());
        ParamEntry entry;
        entry.name = name;
        entry.baseValue = baseValue;
        entry.range = range;
        entry.modulationDepth = modulationDepth;
        entry.inverted = inverted;
        entry.smoothedOutput = baseValue;
        params.push_back(std::move(entry));
        return idx;
    }

    void updateBaseValue(int index, float baseValue) {
        if (index >= 0 && index < static_cast<int>(params.size()))
            params[static_cast<size_t>(index)].baseValue = baseValue;
    }

    void process(const float* buffer, int numSamples) {
        for (auto& p : params) {
            if (static_cast<int>(p.modulatedBuffer.size()) < numSamples)
                p.modulatedBuffer.resize(static_cast<size_t>(numSamples));
        }

        for (int i = 0; i < numSamples; ++i) {
            float absSample = std::abs(buffer[i]);

            slowEnvelope = slowEnvCoeff * slowEnvelope + (1.0f - slowEnvCoeff) * absSample;

            float levelDb = 20.0f * std::log10(std::max(1e-6f, slowEnvelope));
            float intentLvl = std::clamp((levelDb + 60.0f) / 60.0f, 0.0f, 1.0f);

            float gateTarget = (slowEnvelope > 1e-5f) ? 1.0f : 0.0f;
            gateGain = gateCoeff * gateGain + (1.0f - gateCoeff) * gateTarget;

            for (auto& p : params) {
                float intent = p.inverted ? (1.0f - intentLvl) : intentLvl;

                float signedIntent = intent * 2.0f - 1.0f;
                float modulated = p.baseValue + signedIntent * p.modulationDepth * gateGain;

                modulated = std::clamp(modulated, p.range.min, p.range.max);

                p.smoothedOutput = outputSmoothCoeff * p.smoothedOutput
                                 + (1.0f - outputSmoothCoeff) * modulated;

                p.modulatedBuffer[static_cast<size_t>(i)] = p.smoothedOutput;
            }
        }
    }

    const float* getModulatedBuffer(int index) const {
        if (index >= 0 && index < static_cast<int>(params.size()))
            return params[static_cast<size_t>(index)].modulatedBuffer.data();
        return nullptr;
    }

    const float* getModulatedBuffer(const std::string& name) const {
        for (const auto& p : params)
            if (p.name == name) return p.modulatedBuffer.data();
        return nullptr;
    }

    float getIntentLevel() const {
        float levelDb = 20.0f * std::log10(std::max(1e-6f, slowEnvelope));
        return std::clamp((levelDb + 60.0f) / 60.0f, 0.0f, 1.0f);
    }

    void reset() {
        slowEnvelope = 0.0f;
        gateGain = 0.0f;
        for (auto& p : params) {
            p.smoothedOutput = p.baseValue;
            std::fill(p.modulatedBuffer.begin(), p.modulatedBuffer.end(), p.baseValue);
        }
    }

private:
    struct ParamEntry {
        std::string name;
        float baseValue = 0.0f;
        ParameterRange range{0.0f, 1.0f};
        float modulationDepth = 0.10f;
        bool inverted = false;
        float smoothedOutput = 0.0f;
        std::vector<float> modulatedBuffer;
    };

    double sr = 48000.0;
    float slowEnvCoeff = 0.0f;
    float outputSmoothCoeff = 0.0f;
    float gateCoeff = 0.0f;
    float slowEnvelope = 0.0f;
    float gateGain = 0.0f;
    std::vector<ParamEntry> params;
};

} // namespace shine
