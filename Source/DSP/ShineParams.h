#pragma once

namespace shine {

// Two ML outputs from the Shine model
struct ShineParams {
    float presence = 0.0f;   // 0.0-6.0: Dual-band presence boost
    float air = 0.0f;        // 0.0-1.0: High shelf air boost
};

// ---------------------------------------------------------------
// PRESETS — easy to update, no DSP/GUI rebuild needed
// ---------------------------------------------------------------
enum class Preset {
    Auto         = 0,  // ML-driven (adaptive)
    VocalClarity = 1,  // Fixed: P=3.0 / A=0.40  (FINAL)
    AcousticDetail = 2 // Fixed: P=4.5 / A=0.20  (PROVISIONAL — awaiting client sign-off)
};

inline ShineParams getPresetParams(Preset preset) {
    switch (preset) {
        case Preset::VocalClarity:
            return { 3.0f, 0.40f };
        case Preset::AcousticDetail:
            return { 4.5f, 0.20f };  // <-- UPDATE THIS when client confirms final values
        default:
            return { 0.0f, 0.0f };   // Auto: ML drives everything
    }
}

inline const char* getPresetName(Preset preset) {
    switch (preset) {
        case Preset::VocalClarity:   return "Vocal Clarity";
        case Preset::AcousticDetail: return "Acoustic Detail";
        default:                     return "Auto";
    }
}

} // namespace shine
