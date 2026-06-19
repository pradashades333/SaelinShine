#pragma once

#include "BiquadFilter.h"
#include "ShineParams.h"
#include "PeakLimiter.h"
#include <cmath>

namespace shine {

class ShineChain {
public:
    void prepare(double sampleRate) {
        sr = sampleRate;
        gainSmoothCoeff = std::exp(-1.0f / (static_cast<float>(sr) * 0.010f));

        highpass.setSampleRate(sampleRate);
        highpass.reset();
        highpass.configure(60.0f);

        presenceLow.setSampleRate(sampleRate);
        presenceLow.reset();
        presenceLow.configure(2500.0f, 1.0f);

        presenceHigh.setSampleRate(sampleRate);
        presenceHigh.reset();
        presenceHigh.configure(4500.0f, 1.0f);

        airShelf.setSampleRate(sampleRate);
        airShelf.reset();
        airShelf.configure(6000.0f, 0.5f);

        airPeak.setSampleRate(sampleRate);
        airPeak.reset();
        airPeak.configure(8500.0f, 1.4f);

        limiter.prepare(sampleRate);

        smoothedPresScaler = 1.0f;
        smoothedAirScaler = 1.0f;

        updateFilters(1.0f, 1.0f);
    }

    void processBlock(float* buffer, int numSamples,
                      const float* presenceModulated,
                      const float* airModulated) {
        for (int i = 0; i < numSamples; ++i) {
            float presScaler = knobToScaler(presenceModulated[i]);
            float airScaler  = knobToScaler(airModulated[i]);

            smoothedPresScaler = gainSmoothCoeff * smoothedPresScaler
                               + (1.0f - gainSmoothCoeff) * presScaler;
            smoothedAirScaler  = gainSmoothCoeff * smoothedAirScaler
                               + (1.0f - gainSmoothCoeff) * airScaler;

            updateFilters(smoothedPresScaler, smoothedAirScaler);

            float sample = highpass.process(buffer[i]);
            sample = presenceLow.process(sample);
            sample = presenceHigh.process(sample);
            sample = airShelf.process(sample);
            sample = airPeak.process(sample);

            if (std::isnan(sample) || std::isinf(sample)) {
                resetFilters();
                buffer[i] = buffer[i];
            } else {
                buffer[i] = sample;
            }
        }

        limiter.process(buffer, numSamples);
    }

    float processStatic(float input, float presScaler, float airScaler) {
        updateFilters(presScaler, airScaler);
        float sample = highpass.process(input);
        sample = presenceLow.process(sample);
        sample = presenceHigh.process(sample);
        sample = airShelf.process(sample);
        sample = airPeak.process(sample);

        if (std::isnan(sample) || std::isinf(sample)) {
            resetFilters();
            return input;
        }
        return sample;
    }

private:
    void updateFilters(float presScaler, float airScaler) {
        float presGain = std::min(presScaler, 1.15f);
        float airGainVal = std::min(airScaler, 1.15f);

        presenceLow.setGain(4.5f * presGain);
        presenceHigh.setGain(2.2f * presGain);
        airShelf.setGain(10.0f * airGainVal);
        airPeak.setGain(7.0f * airGainVal);
    }

    void resetFilters() {
        highpass.reset();
        highpass.configure(60.0f);
        presenceLow.reset();
        presenceHigh.reset();
        airShelf.reset();
        airPeak.reset();
    }

    double sr = 48000.0;
    float gainSmoothCoeff = 0.0f;
    float smoothedPresScaler = 1.0f;
    float smoothedAirScaler = 1.0f;

    HighPassFilter highpass;
    PresenceBandFilter presenceLow;
    PresenceBandFilter presenceHigh;
    AirShelfFilter airShelf;
    PresenceBandFilter airPeak;
    PeakLimiter limiter;
};

} // namespace shine
