#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace shine {

class BiquadFilter {
public:
    void reset() {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    void setSampleRate(double sr) {
        sampleRate = sr;
    }

    float process(float input) {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        return output;
    }

protected:
    void setCoefficients(float _b0, float _b1, float _b2, float _a1, float _a2) {
        b0 = _b0; b1 = _b1; b2 = _b2; a1 = _a1; a2 = _a2;
    }

    double sampleRate = 48000.0;

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};

class HighPassFilter : public BiquadFilter {
public:
    void configure(float cutoff = 60.0f) {
        float w0 = 2.0f * static_cast<float>(M_PI) * cutoff / static_cast<float>(sampleRate);
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * 0.707f);

        float a0 = 1.0f + alpha;
        float _b0 = (1.0f + cosw0) / 2.0f;
        float _b1 = -(1.0f + cosw0);
        float _b2 = (1.0f + cosw0) / 2.0f;
        float _a1 = -2.0f * cosw0;
        float _a2 = 1.0f - alpha;

        setCoefficients(_b0 / a0, _b1 / a0, _b2 / a0, _a1 / a0, _a2 / a0);
    }
};

class PresenceBandFilter : public BiquadFilter {
public:
    void configure(float frequency, float q) {
        freq = frequency;
        Q = q;
    }

    void setGain(float gainDb) {
        if (std::abs(gainDb - lastGainDb) < 0.005f) return;
        lastGainDb = gainDb;

        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * static_cast<float>(M_PI) * freq / static_cast<float>(sampleRate);
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * Q);

        float a0 = 1.0f + alpha / A;
        float _a1 = -2.0f * cosw0;
        float _a2 = 1.0f - alpha / A;
        float _b0 = 1.0f + alpha * A;
        float _b1 = -2.0f * cosw0;
        float _b2 = 1.0f - alpha * A;

        setCoefficients(_b0 / a0, _b1 / a0, _b2 / a0, _a1 / a0, _a2 / a0);
    }

private:
    float freq = 2500.0f;
    float Q = 1.0f;
    float lastGainDb = -999.0f;
};

class AirShelfFilter : public BiquadFilter {
public:
    void configure(float freq, float q) {
        frequency = freq;
        Q = q;
    }

    void setGain(float gainDb) {
        if (std::abs(gainDb - lastGainDb) < 0.005f) return;
        lastGainDb = gainDb;

        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * static_cast<float>(M_PI) * frequency / static_cast<float>(sampleRate);
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * Q);

        float _b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha);
        float _b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
        float _b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha);
        float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha;
        float _a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
        float _a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha;

        setCoefficients(_b0 / a0, _b1 / a0, _b2 / a0, _a1 / a0, _a2 / a0);
    }

private:
    float frequency = 6000.0f;
    float Q = 0.5f;
    float lastGainDb = -999.0f;
};

} // namespace shine
