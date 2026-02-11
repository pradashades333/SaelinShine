#pragma once

#include "BiquadFilter.h"
#include "ShineParams.h"
#include <cmath>

namespace shine {

// Signal flow matching shine_dsp.py:
// Input -> HPF(60Hz) -> PresenceLow(2.5kHz) -> PresenceHigh(4.5kHz) -> AirShelf(6kHz) -> Output
class ShineChain {
public:
    void prepare(double sampleRate) {
        sr = sampleRate;

        highpass.setSampleRate(sampleRate);
        highpass.reset();
        highpass.configure(60.0f);

        presenceLow.setSampleRate(sampleRate);
        presenceLow.reset();
        presenceLow.configure(2500.0f, 0.8f);

        presenceHigh.setSampleRate(sampleRate);
        presenceHigh.reset();
        presenceHigh.configure(4500.0f, 1.0f);

        airShelf.setSampleRate(sampleRate);
        airShelf.reset();

        updateFilters();
    }

    void setParams(const ShineParams& newParams) {
        params = newParams;
        updateFilters();
    }

    float process(float input) {
        float sample = highpass.process(input);
        sample = presenceLow.process(sample);
        sample = presenceHigh.process(sample);
        sample = airShelf.process(sample);

        // NaN/inf protection — if filters go bad, reset and pass through
        if (std::isnan(sample) || std::isinf(sample)) {
            highpass.reset();
            highpass.configure(60.0f);
            presenceLow.reset();
            presenceHigh.reset();
            airShelf.reset();
            return input;
        }

        return sample;
    }

private:
    void updateFilters() {
        // Clamp gains to safe ranges
        float presGain = std::min(params.presence, 6.0f);
        float airGain = std::min(params.air, 1.0f);

        // Presence: dual-band mapping from shine_dsp.py
        presenceLow.setGain(presGain * 2.0f);
        presenceHigh.setGain(presGain * 1.3f);

        // Air: 0-1 maps to 0-10 dB shelf above 6kHz
        airShelf.setGain(airGain * 10.0f);
    }

    double sr = 48000.0;
    ShineParams params;
    HighPassFilter highpass;
    PresenceBandFilter presenceLow;
    PresenceBandFilter presenceHigh;
    AirShelfFilter airShelf;
};

} // namespace shine
