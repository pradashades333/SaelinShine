#pragma once

namespace shine {

// Two ML outputs from the Shine model
struct ShineParams {
    float presence = 0.0f;   // 0.0-6.0: Dual-band presence boost
    float air = 0.0f;        // 0.0-1.0: High shelf air boost
};

} // namespace shine
