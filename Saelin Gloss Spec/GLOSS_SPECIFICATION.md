# Saelin Gloss — Implementation Specification
**Version 1.0 — for C++/JUCE implementation**

This document specifies Saelin Gloss: an adaptive finishing/polish plugin
shipping for macOS and Windows in AU and VST3 formats.

Gloss consists of two parts:
1. **The Gloss character stage** (this document) — the five-element
   finishing DSP unique to Gloss
2. **The Saelin Adaptive Engine** (separate spec: SAE_IMPLEMENTATION_SPEC.md)
   — the shared analysis-and-modulation module used by all Saelin plugins

The character stage and the SAE are independent modules with a clean
interface. For Gloss specifically, the SAE has a narrower role than in
Ember — it does NOT modulate the user-facing knobs. Instead, it modulates
internal compression behavior in real time (see Section 5).

---

## 1. PRODUCT OVERVIEW

### What Gloss is
Saelin Gloss is an adaptive finishing/polish plugin. It applies a five-
element processing chain that takes a raw, untouched recording and makes
it sound finished — cohesive, sweetened, and intentional. The user shapes
the result with two knobs (Coat and Air); the Adaptive Engine works
underneath to keep transients breathing through the cohesion processing.

### Product specifications
- **Platforms:** macOS (Universal binary — Apple Silicon + Intel), Windows (64-bit)
- **Formats:** AU, VST3
- **Channels:** Mono and stereo (per-channel processing; SAE mono-sums for analysis)
- **Sample rates:** 44.1, 48, 88.2, 96, 176.4, 192 kHz
- **Latency:** Determined by oversampler — see Section 8
- **GUI:** Existing Saelin design system (cream/coral palette, two-knob layout,
  NO mode tabs, adaptation indicator bars driven by SAE)

### Controls
- **Coat knob:** [0, 1] — Amount of Gloss applied. Scales all five DSP
  elements together. 0 = minimal, 0.5 = default everyday setting, 1 = heavy.
- **Air knob:** [0, 1] — Character tilt of the finish. 0 = warm/intimate
  finish, 0.5 = balanced, 1 = open/airy finish.
- **Bypass:** Standard plugin bypass — bit-identical to dry input when
  bypassed (see Section 7).
- **Adaptation indicator bars:** Existing UI element, driven by the
  SAE module's onset detection. Animates visibly when the engine is
  actively easing compression during transients.

---

## 2. ARCHITECTURE

```
audio in ─┬──▶ ┌─────────────────────┐
          │    │ Saelin Adaptive     │
          │    │ Engine (SAE)        │
          │    │ — onset detection   │
          │    └──────────┬──────────┘
          │               │
          │       onset envelope (0..1)
          │               │
          ▼               ▼
┌─────────────────────────────────────────────────────────────────┐
│ GLOSS CHARACTER STAGE                                           │
│                                                                 │
│  ┌───────────┐  ┌────────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ Defined   │→ │ Harmonic   │→ │Sweetened │→ │ Glue Compress│  │
│  │ Bottom    │  │ Enrichment │  │   Top    │  │  ← AE eases  │  │
│  │ (HPF + LS)│  │ (saturator)│  │   (HS)   │  │ GR on onsets │  │
│  └───────────┘  └────────────┘  └──────────┘  └──────────────┘  │
│         ↑               ↑              ↑              ↑         │
│         └───────── Coat & Air knob mappings ──────────┘         │
└─────────────────────────────────────────────────────────────────┘
                                                          │
                                                       audio out
```

**Key architectural points:**
- The SAE does NOT modulate the user knobs (different from Ember).
- The SAE only modulates the glue compressor's gain-reduction amount
  during detected onsets.
- All other parameters are controlled by Coat and Air alone (static
  within a given knob position).
- The base signal output is preserved bit-identical when the SAE is
  disengaged.

### SAE registration
At plugin initialization, Gloss registers ONE parameter with the SAE
for the AE behavior:

```cpp
sae.registerParameter("transientEase", baseValue=0.0f, range={0.0f, 1.0f},
                      modulationDepth=1.0f);
```

This parameter ranges 0..1 where:
- 0.0 = no GR ease (full compression)
- 1.0 = maximum GR ease (during strongest onsets)

The SAE drives this parameter from its onset envelope. The character
stage uses the modulated value to scale glue compression GR amount.

When `setEngaged(false)`, the SAE returns 0.0 for transientEase —
guaranteeing the base signal is unchanged.

---

## 3. THE FIVE DSP ELEMENTS

All elements run in this order (signal chain):
```
input → defined bottom → harmonic enrichment → sweetened top shelf
      → glue compression (with AE-driven transient ease) → output
```

### 3.1 Defined Bottom

**Purpose:** Anchored bottom-end body without rumble or thickness.

**Implementation:**
- 2nd-order Butterworth high-pass at **35 Hz** (clears sub-mud)
- Low shelf, cookbook formulation:
  - Frequency: **150 Hz**
  - Gain (at Coat=0.5, Air=0.5): **+3.5 dB**
  - Q: **0.5** (wide, soft transition)

**Coat scaling:**
```
coat_factor = 0.3 + 1.4 * coat   // ranges 0.3..1.7
```

**Air scaling (low end):**
```
air_factor_low = 1.3 - 0.6 * air  // ranges 1.3 (warm) to 0.7 (open)
```

**Effective shelf gain:**
```
low_shelf_gain = 3.5 * coat_factor * air_factor_low
```

So at Coat=0, Air=1: gain = ~0.74 dB (almost off)
At Coat=1, Air=0: gain = ~7.74 dB (heavy warm body)

**State:** HPF biquad state (z1, z2) + shelf biquad state (z1, z2). All
state MUST persist across blocks.

---

### 3.2 Harmonic Enrichment

**Purpose:** Subtle saturation throughout the spectrum — the
"this has been through a chain" character.

**Implementation:**
- 2x oversampling with anti-aliasing
- Tanh waveshaper with slight asymmetric bias for even-harmonic dominance
- DC removal post-saturation
- RMS-matched output to preserve level

**Drive parameter:**
```
drive = clamp(0.12 * coat_factor, 0.02, 0.30)
```

Where `coat_factor` is the same as Section 3.1.
So at Coat=0.5: drive = 0.12 (locked default).
At Coat=1.0: drive = ~0.20.
At Coat=0.0: drive = 0.04.

**Algorithm pseudocode:**
```cpp
// upsample 2x
upsampled = oversample(input, 2);

// asymmetric bias
asym = 0.08;
biased = upsampled + asym * drive * 0.5;

// tanh waveshaper
g = 1.0 + 6.0 * drive;
saturated = tanh(biased * g) / g;

// DC removal — 1st-order Butterworth HP at 5 Hz (in oversampled domain)
saturated = dc_remove_filter.process(saturated);

// downsample back
output = downsample(saturated, 2);

// RMS match
in_rms = rms(input);
out_rms = rms(output);
if (out_rms > 1e-9) {
    output = output * (in_rms / out_rms);
}
```

**State:** Oversampler filter state + DC removal filter state. All state
must persist across blocks.

---

### 3.3 Sweetened Top Shelf

**Purpose:** Sweetened high-frequency presence — the "expensive sounding"
Pultec-style air lift.

**Implementation:**
- Cookbook high-shelf at **10,000 Hz** with **Q = 0.5**
- Base gain (at Coat=0.5, Air=0.5): **+3.5 dB**

**Air scaling (high end):**
```
air_factor_high = 0.5 + 1.0 * air  // ranges 0.5 (warm) to 1.5 (open)
```

**Effective shelf gain:**
```
high_shelf_gain = 3.5 * coat_factor * air_factor_high
```

So at Coat=1, Air=1: gain = ~8.93 dB (heavy open air)
At Coat=0, Air=0: gain = ~0.53 dB (almost off)
At Coat=0.5, Air=0.5: gain = 3.5 dB (default)

**Note:** Air's effect is asymmetric across the two shelves — increasing
Air boosts the high shelf but reduces the low shelf, producing a clear
warm↔open tilt without changing overall level (constant-energy by design).

**State:** Shelf biquad state, persistent across blocks.

---

### 3.4 Glue Compression (with AE integration)

**Purpose:** Cohesion and bonding without obvious compression. Single-band
SSL G / Slate VBC-style gentle bus compression.

**Static parameters:**
- Ratio: **1.7:1**
- Attack: **50 ms** (one-pole detector coefficient)
- Release: **90 ms** (one-pole detector coefficient)
- Soft knee: **6 dB** wide
- Detection: RMS-ish (squared signal, square-root after smoothing)

**Threshold (Coat + Air controlled):**
```
base_threshold = -18.0 dB
coat_offset = -4.0 * (coat - 0.5)   // lower threshold at high Coat
air_offset = 2.0 * (air - 0.5)      // less glue when Air is open
threshold = clamp(base_threshold + coat_offset + air_offset, -24, -12)
```

So at Coat=0.5, Air=0.5: threshold = -18 dB (locked default)
At Coat=1, Air=0: threshold = -22 dB (more glue, warmer)
At Coat=0, Air=1: threshold = -14 dB (less glue, more open)

**Compression curve (soft knee):**
```cpp
over = env_db - threshold;
if (over < -knee/2)             gr = 0;
else if (over > knee/2)         gr = over * (1 - 1/ratio);
else                            gr = ((over + knee/2)^2) / (2*knee) * (1 - 1/ratio);
```

**Make-up gain (always-on):**
After compression, RMS-match the output to the pre-compression input
to preserve perceived loudness:
```cpp
pre_rms = rms(signal_before_comp);
post_rms = rms(signal_after_comp);
if (post_rms > 1e-9) {
    output = output * (pre_rms / post_rms);
}
```

**AE-driven transient ease (this is the AE's role in Gloss):**

The SAE provides a `transientEase` modulation value (0..1) per sample.
This scales DOWN the gain reduction amount, allowing transients to
punch through:

```cpp
gr_amount_modulated = gr_amount * (1.0 - transientEase * 0.7);
gain = pow(10, -gr_amount_modulated / 20);
output_sample = input_sample * gain;
```

When `transientEase = 0.0` (no onset detected): full GR applied (locked behavior)
When `transientEase = 1.0` (strong onset): 70% GR reduction (transient passes through)

This is **non-destructive** — the AE can only REDUCE GR, never add. With
SAE disengaged (`transientEase` always 0), the compressor runs at full
locked behavior.

**State:** Envelope follower state (one float), pre-comp RMS state for
make-up gain (~50ms running estimate), GR smoothing if needed.

---

### 3.5 SAE Integration (the connective tissue)

The SAE module is registered with one parameter: `transientEase`.
The SAE drives this from its onset detection envelope (see SAE spec
Section 3.4 for onset detector implementation).

**Per-block flow:**
```cpp
void EmberProcessBlock(buffer) {
    sae.process(buffer);  // updates internal SAE state from input

    const float* ease_buffer = sae.getModulatedBuffer("transientEase");
    // ease_buffer contains per-sample modulation 0..1

    // run character stage with per-sample ease modulation
    process_character_stage(buffer, ease_buffer, coat_user, air_user);
}
```

**Critical:** The SAE analyzes the INPUT signal (pre-processing), not the
output. So feature extraction happens on raw audio entering the plugin,
not after the character stage has processed it.

---

## 4. KNOB MAPPING SUMMARY

| Parameter | Coat=0, Air=0 (warm/light) | Coat=0.5, Air=0.5 (default) | Coat=1, Air=1 (open/heavy) |
|---|---|---|---|
| Low shelf gain | +1.37 dB | +3.5 dB | +1.79 dB |
| Enrichment drive | 0.04 | 0.12 | 0.20 |
| High shelf gain | +0.53 dB | +3.5 dB | +8.93 dB |
| Glue threshold | -22 dB | -18 dB | -16 dB |

Note: the table values assume `coat_factor = 0.3 + 1.4 * coat` and
`air_factor_*` as defined in 3.1 and 3.3.

---

## 5. CRITICAL IMPLEMENTATION NOTES

### 5.1 Filter state persistence (MANDATORY)
**All internal filters MUST persist state across audio blocks.** This
includes:
- HPF at 35 Hz
- Low shelf biquad
- Oversampler anti-aliasing filters (both up- and down-sample)
- DC-removal filter in enrichment
- High shelf biquad
- Glue compressor envelope follower

Per-block state reset causes audible crackle. Verified during development
as a real bug. Declare all filter state as persistent member variables.

### 5.2 Oversampler (enrichment only)
- Internal sample rate: 2x the host sample rate
- Anti-aliasing filter: 4th-order Butterworth or polyphase FIR with
  cutoff at 0.45 * host sample rate (in normalized terms relative to
  oversampled rate)
- Latency: small but nonzero from filter group delay. Report to host via
  standard plugin latency mechanism if perceptible.

### 5.3 Make-up gain timing
The make-up gain calculation in glue compression uses RMS over a running
window. For stability, implement as a one-pole follower on the squared
signal (time constant ~50 ms), then take the square root. Buffer-by-buffer
RMS computation will jitter on small host buffers.

### 5.4 Per-sample parameter updates
The SAE's `transientEase` output is per-sample. The glue compressor
applies it per-sample to scale GR. All other parameters (filter gains,
threshold, drive) only update per-block (when Coat/Air change), since
the SAE doesn't modulate them.

### 5.5 Numerical safety
- `log10(max(x, 1e-12))` for all log calls
- `sqrt(max(x, 1e-12))` for all sqrt calls
- Guard denominators with `max(x, 1e-9)`
- Clamp parameter ranges as specified

### 5.6 Sample-rate independence
All time constants (attack, release, oversampler filter cutoff) must be
recomputed when sample rate changes:
```cpp
float coef = exp(-1.0f / (sampleRate * timeConstantSec));
```

### 5.7 Stereo handling
- Each channel processes through its own character stage instance
  (independent filter states, envelope states)
- SAE analyzes the mono sum (L+R)*0.5 and produces a single `transientEase`
  envelope applied to both channels' compression
- This keeps the stereo image coherent while sharing AE intelligence

---

## 6. THE BASELINE INTACT INVARIANT

**When the SAE is disengaged, Gloss output must be deterministic and
identical to a non-AE build.** Since the SAE returns `transientEase = 0.0`
when disengaged, the glue compressor receives 0 ease scaling, and the
character stage runs at its locked behavior.

**Test:** with SAE disengaged, render any audio file through Gloss. The
output must be bit-identical (or within float rounding, < -200 dB
difference) to a build that doesn't include the SAE module.

This invariant guarantees adding the SAE to Gloss cannot damage the
locked sound.

---

## 7. BYPASS BEHAVIOR

When the plugin's bypass button is engaged:
- Audio passes through unchanged (no processing)
- Output is bit-identical to input
- SAE state should remain active for smooth re-engagement (no click)
- UI indicator bars freeze at last position

---

## 8. PERFORMANCE / CPU NOTES

The 2x oversampling in harmonic enrichment is the most CPU-intensive
element. Optimization considerations:
- Polyphase FIR for the oversampler is usually more efficient than IIR
  for plugin use
- Tanh can be approximated with rational functions if measurement shows
  the standard tanh is a bottleneck
- Glue compressor envelope follower is per-sample but very lightweight

Target: Ember-comparable or better CPU usage on a typical i5/M1.

---

## 9. PRESET LIST (for v1 launch)

Factory presets at launch:
- **Default** — Coat 0.5, Air 0.5
- **Light Touch** — Coat 0.3, Air 0.5
- **Standard Finish** — Coat 0.5, Air 0.5 (same as default — could differ later)
- **Heavy Polish** — Coat 0.8, Air 0.5
- **Warm Vocal** — Coat 0.5, Air 0.25
- **Bright Mix** — Coat 0.5, Air 0.75
- **Intimate** — Coat 0.3, Air 0.2

(Final preset list to be tuned during pre-launch testing.)

---

## 10. REFERENCE PYTHON IMPLEMENTATION

The locked Gloss prototype is in:
- `gloss_static.py` (the static knob-controlled processor — to be exported)
- `gloss_with_ae.py` (the full version including SAE wiring — to be exported)

The C++ implementation should produce output that matches these references
to within numerical precision, on the same input audio at the same knob
positions.

**Test material:**
- `1779467452720_Nebraska.wav` — primary reference recording
- `gloss_knob_default.wav` — Gloss at Coat=0.5, Air=0.5 (SAE disengaged)
- `gloss_v4_AE_ON.wav` — Gloss at Coat=0.5, Air=0.5 (SAE engaged with
  transient ease)

---

## 11. TESTING / VALIDATION CHECKLIST

### 11.1 Per-knob sonic match
Render Nebraska through C++ at Coat=0.5, Air=0.5, SAE disengaged. Compare
to Python reference at same settings. Difference should be inaudible
(< 1 dB across the spectrum).

### 11.2 Baseline-intact null test
With SAE disengaged, render any file. Output must be bit-identical (or
within -200 dB) to a build without the SAE module.

### 11.3 SAE engaged check
With SAE engaged, render dynamic material. The output must:
- Preserve transients better than SAE-disengaged version (measurable
  via crest factor and peak level)
- Produce no clicks, zipper noise, or pumping
- Show indicator bar movement on the GUI corresponding to onset events

### 11.4 Knob behavior
- Coat from 0 to 1: sweeps musically from "barely there" to "heavy
  finishing." No dead zones, no abrupt transitions.
- Air from 0 to 1: tilts warm↔open without changing overall level.
- Both knobs at 0.5: equals the locked default sound.

### 11.5 Stress testing
- All sample rates (44.1 through 192 kHz)
- All buffer sizes (32, 64, 128, 256, 512, 1024 samples)
- Mono and stereo signal paths
- Multiple instance stability (10+ instances in a session)
- Automation of Coat/Air at audio rate (should be smooth)
- Bypass click test (engaging/disengaging should not click)

---

## 12. WHAT'S NOT IN THIS SPEC

- The SAE module itself (separate spec: SAE_IMPLEMENTATION_SPEC.md)
- GUI implementation details (existing Saelin design system; visual
  assets and layout handed off separately)
- Plugin chrome (matches existing Saelin plugin conventions)
- Marketing copy and brand messaging

---

## 13. VERSIONING

This is Gloss v1.0. Future versions may add additional elements
(dynamic 2-5 kHz tame, additional knob behavior, etc.) — see the
Gloss Design Document for parked features. Any future changes must
preserve the locked sonic identity established here.

---

*End of Gloss specification.*
