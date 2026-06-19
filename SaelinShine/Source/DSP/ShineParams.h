#pragma once

namespace shine {

struct ShineParams {
    float presence = 0.5f;
    float air = 0.5f;
};

enum class Preset {
    VocalClarity   = 0,
    AcousticDetail = 1
};

inline ShineParams getPresetParams(Preset preset) {
    switch (preset) {
        case Preset::VocalClarity:
            return { 0.67f, 0.89f };
        case Preset::AcousticDetail:
            return { 0.89f, 0.44f };
        default:
            return { 0.5f, 0.5f };
    }
}

inline const char* getPresetName(Preset preset) {
    switch (preset) {
        case Preset::VocalClarity:   return "Vocal Clarity";
        case Preset::AcousticDetail: return "Acoustic Detail";
        default:                     return "Vocal Clarity";
    }
}

inline float knobToScaler(float knob) {
    if (knob <= 0.5f)
        return knob * 2.0f;
    else
        return 1.0f + (knob - 0.5f) * 0.3f;
}

inline const char* getPresenceWord(float value) {
    if (value < 0.2f) return "Settled";
    if (value < 0.4f) return "Clear";
    if (value < 0.6f) return "Forward";
    if (value < 0.8f) return "Lifted";
    return "Crisp";
}

inline const char* getAirWord(float value) {
    if (value < 0.2f) return "Close";
    if (value < 0.4f) return "Natural";
    if (value < 0.6f) return "Balanced";
    if (value < 0.8f) return "Open";
    return "Airy";
}

} // namespace shine
