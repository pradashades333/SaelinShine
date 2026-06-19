# Saelin Shine — SAE Integration Specification
**Version 1.0 — for Shine v2.0 implementation**

This document specifies how the Saelin Adaptive Engine (SAE) replaces
the ML-driven adaptation in Shine v1.

It depends on:
- `SAE_IMPLEMENTATION_SPEC.md` (the main SAE module spec)
- `SAE_ADDENDUM_v1.1.md` (the inverted-flag addition)
- Shine v1's existing DSP processor (unchanged in v2)

The static DSP in Shine v1 — dual-band presence shelves + air shelf
with gain staging — is preserved exactly. Only the adaptation engine
changes: the trained ML model (`shine.pt`) is removed and replaced
with the SAE module, configured to produce equivalent adaptation
behavior.

---

## 1. WHAT CHANGES FROM v1

### Stays the same
- Static DSP processor (dual-band presence + air shelf, all gain staging)
- Two mode tabs (Vocal Clarity, Acoustic Detail) — see Section 7
- Two-knob UI (Presence, Air)
- Output peak limiter (must be preserved from v1)
- All existing v1 behavior at output

### Changes
- ML model (`shine.pt`) is removed from the production build
- ML inference code path is removed
- SAE module replaces ML model as the adaptation source
- Knob ranges normalized to [0, 1] internally (see Section 2)

### Net effect on the customer
- Sonically equivalent to v1 at default settings
- Same "magic" feel of inverted adaptation (quiet → more lift)
- Cleaner codebase, unified adaptive architecture across Saelin catalog

---

## 2. KNOB RANGE NORMALIZATION

Shine v1 used internal ranges of:
- Presence: 0–4.5
- Air: 0–0.45

Shine v2 uses normalized 0–1 ranges for both knobs (standard plugin
convention). The static DSP gains are scaled by the user knob value
internally.

### Knob-to-scaler mapping
```
knob 0.0  → scaler 0.00 (effective bypass for that band)
knob 0.5  → scaler 1.00 (Shine v1 default behavior)
knob 1.0  → scaler 1.15 (15% more lift than v1 default)
```

Piecewise linear interpolation between these points:
```cpp
float knob_to_scaler(float knob) {
    if (knob <= 0.5f) {
        return knob * 2.0f;  // 0..0.5 → 0..1.0
    } else {
        return 1.0f + (knob - 0.5f) * 0.3f;  // 0.5..1.0 → 1.0..1.15
    }
}
```

The scaler then multiplies the static DSP gain values in dB:
```cpp
low_presence_gain_db = 4.5f * presence_scaler;   // Peak at 2.5 kHz, Q=1.0
high_presence_gain_db = 2.2f * presence_scaler;  // Peak at 4.5 kHz, Q=1.0
air_shelf_gain_db = 10.0f * air_scaler;          // High shelf at 6 kHz, Q=0.5
air_peak_gain_db = 7.0f * air_scaler;            // Peak at 8.5 kHz, Q=1.4
```

These values produce the same spectral curve as Shine v1 at user knob
position 0.5 (verified against v1 reference renders — match within
~1-2 dB across all bands).

---

## 3. SAE REGISTRATION

At plugin initialization, Shine registers two parameters with the SAE:

```cpp
sae.registerParameter(
    "presence",
    /* baseValue */ userPresenceKnob,    // 0..1 from UI
    /* range */ {0.0f, 1.0f},
    /* modulationDepth */ 0.10f,
    /* inverted */ true
);

sae.registerParameter(
    "air",
    /* baseValue */ userAirKnob,         // 0..1 from UI
    /* range */ {0.0f, 1.0f},
    /* modulationDepth */ 0.10f,
    /* inverted */ true
);
```

The `inverted = true` flag is critical (see SAE Addendum v1.1). It
flips the sign of `intent_lvl` so that:
- Quiet performance → SAE increases knob value → more lift
- Loud performance → SAE decreases knob value → less lift

This is the inverted curve that gives Shine its character.

### Modulation depth: 0.10
This is small for a reason. Our analysis of Shine v1 showed:
- The static EQ does ~95% of the audible work (the aggressive HF lift)
- The adaptation contributes only ~0.3–0.5 dB of variation in
  spectral lift between quiet and loud sections
- Larger depths would over-modulate and break v1's character

Depth 0.10 produces ±~1 dB swing in spectral lift, which approximates
v1's measured inverted-curve behavior.

---

## 4. PROCESSING FLOW

```
audio in ─┬──▶ ┌─────────────────────┐
          │    │ SAE module          │
          │    │ — extracts level    │
          │    │   intent, applies   │
          │    │   inverted curve    │
          │    └──────────┬──────────┘
          │               │
          │  per-sample modulated knob values
          │  (presence_modulated, air_modulated)
          │               │
          ▼               ▼
┌─────────────────────────────────────────────────────────────────┐
│ SHINE STATIC DSP (unchanged from v1)                            │
│                                                                 │
│  ┌─────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐  ┌─────────┐  │
│  │ HPF │→ │ Peak EQ  │→ │ Peak EQ  │→ │ Air     │→ │ Peak EQ │  │
│  │60 Hz│  │ 2.5 kHz  │  │ 4.5 kHz  │  │ Shelf   │  │ 8.5 kHz │  │
│  │     │  │ scaled   │  │ scaled   │  │ 6 kHz   │  │ scaled  │  │
│  │     │  │ by pres  │  │ by pres  │  │ scaled  │  │ by air  │  │
│  │     │  │          │  │          │  │ by air  │  │         │  │
│  └─────┘  └──────────┘  └──────────┘  └─────────┘  └─────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                                          │
                                                          ▼
                                                ┌─────────────────┐
                                                │ Output Limiter  │
                                                │ (preserved from │
                                                │  Shine v1)      │
                                                └────────┬────────┘
                                                         │
                                                      audio out
```

### Per-block processing pseudocode
```cpp
void Shine::processBlock(buffer, numSamples) {
    // 1. Feed input to SAE for analysis
    sae.process(buffer, numSamples);
    
    // 2. Get per-sample modulated knob values
    const float* presence_mod = sae.getModulatedBuffer("presence");
    const float* air_mod = sae.getModulatedBuffer("air");
    
    // 3. Process each sample
    for (int i = 0; i < numSamples; i++) {
        float pres_scaler = knob_to_scaler(presence_mod[i]);
        float air_scaler = knob_to_scaler(air_mod[i]);
        
        // Update biquad coefficients if scaler changed (with smoothing)
        updateCoefficients(pres_scaler, air_scaler);
        
        // Apply filter chain
        float sample = buffer[i];
        sample = hpf.process(sample);
        sample = presence_low_eq.process(sample);
        sample = presence_high_eq.process(sample);
        sample = air_shelf.process(sample);
        sample = air_peak_eq.process(sample);
        
        buffer[i] = sample;
    }
    
    // 4. Apply output limiter (existing v1 code)
    output_limiter.process(buffer, numSamples);
}
```

### Coefficient update smoothing
Because the SAE modulates per-sample, raw application of new
coefficients each sample would cause zipper noise. Apply coefficient
smoothing with a one-pole filter (time constant ~10ms) on the gain
values before computing biquad coefficients. This is standard practice
for parameter-modulated EQ.

---

## 5. STATIC DSP REFERENCE VALUES

Exact gain values at scaler = 1.0 (= user knob at 0.5, = Shine v1 default):

| Filter | Type | Frequency | Q | Gain at scaler=1.0 |
|---|---|---|---|---|
| 1. HPF | 2nd-order Butterworth HP | 60 Hz | — | (no gain — filter only) |
| 2. Low Presence | Peaking EQ | 2,500 Hz | 1.0 | +4.5 dB |
| 3. High Presence | Peaking EQ | 4,500 Hz | 1.0 | +2.2 dB |
| 4. Air Shelf | High Shelf | 6,000 Hz | 0.5 | +10.0 dB |
| 5. Air Peak | Peaking EQ | 8,500 Hz | 1.4 | +7.0 dB |

All gains scale linearly with the scaler value (0 = no boost, 1.15 = max).

---

## 6. SAE TIME CONSTANTS FOR SHINE

The SAE main spec defines several time constants for feature extraction
and modulation smoothing. For Shine, use the default values from the
main SAE spec — they produce appropriate behavior for the inverted
curve:

| Parameter | Value |
|---|---|
| Slow level envelope time constant | 300 ms |
| Output modulation smoothing time constant | 280 ms |
| Activity gate fade time | 50 ms |

These do not need to be customized for Shine. The default SAE behavior
produces ~1 dB of inverted-curve modulation in spectral lift, which
matches Shine v1's measured behavior.

---

## 7. MODE TABS

Shine has two mode tabs: **Vocal Clarity** and **Acoustic Detail**.

**Critical clarification:** Unlike Ember's mode tabs (which select
distinct character voicings via different DSP), Shine's mode tabs
are **knob preset selectors only**. The DSP processing is identical
between modes. Selecting a mode auto-sets the Presence and Air knobs
to recommended starting points for that material type.

### Mode defaults

| Mode | Presence (internal) | Presence (displayed) | Air (internal) | Air (displayed) |
|---|---|---|---|---|
| Vocal Clarity | 0.67 | *Lifted* | 0.89 | *Airy* |
| Acoustic Detail | 0.89 | *Crisp* | 0.44 | *Balanced* |

**The internal float values (0.67, 0.89, 0.44) are not shown to users.**
They are the underlying knob values stored in plugin state and passed
to the SAE/DSP. What the user sees:
- The knob's visual rotation (set to the position corresponding to the
  internal value)
- The italic value word below the knob, pulled from the parameter
  vocabulary defined in `SHINE_UI_SPEC.md` §4.4

The parameter vocabulary mapping (knob value → displayed word):
- **Presence:** 0.0–0.2 *Settled*, 0.2–0.4 *Clear*, 0.4–0.6 *Forward*,
  0.6–0.8 *Lifted*, 0.8–1.0 *Crisp*
- **Air:** 0.0–0.2 *Close*, 0.2–0.4 *Natural*, 0.4–0.6 *Balanced*,
  0.6–0.8 *Open*, 0.8–1.0 *Airy*

So Vocal Clarity selects "Lifted / Airy" defaults; Acoustic Detail
selects "Crisp / Balanced" defaults.

(These translate from v1's internal ranges: Vocal Clarity was P=3.0/4.5,
Air=0.40/0.45; Acoustic Detail was P=4.0/4.5, Air=0.20/0.45.)

### Mode tab interaction
- Clicking a mode tab sets the Presence and Air knobs to that mode's defaults
- After the mode sets the knobs, the user can adjust freely
- The active mode is stored as plugin state (persists with the session/preset)
- Switching modes again will re-apply the defaults (overwriting user adjustments)

### Note for future products
This "mode-as-preset" architecture is specific to Shine. Future Saelin
products (including Ember and Gloss) use distinct DSP paths per mode
where the mode tabs are meaningful character voicings. Don't generalize
Shine's pattern.

---

## 8. WHAT TO REMOVE FROM v1

When converting Shine v1 to v2:

1. **Remove `models/shine.pt`** — the trained ML model file
2. **Remove ML inference code** in the processing path
3. **Remove `create_shine_dataset_v2.py` and `train_shine.py`** from the
   production codebase (move to archive if useful for future training,
   but they should not ship with the plugin)

Files to keep:
- `shine_dsp.py` / equivalent C++ — the static DSP processor
- `saelin_multi_mic.py` / equivalent — the routing logic (will need
  updating to use SAE instead of ML model)

---

## 9. BASELINE INTACT INVARIANT

When the SAE is disengaged, Shine v2 must produce output equivalent to
the static DSP at the user knob positions, with no adaptation
modulation.

Test: With SAE disengaged, render Nebraska through Shine v2 at
Presence=0.5, Air=0.5. The result should match Shine v1's output at
default knob positions within ~1-2 dB across all bands (measurement
noise from filter topology differences).

When SAE is engaged, the output should be similar to v1 but with the
characteristic inverted-curve modulation (quieter passages get
slightly more lift; louder passages get slightly less).

---

## 10. OUTPUT LIMITER

The output peak limiter from Shine v1 must be preserved in v2 with
identical behavior. Without this limiter, the aggressive HF boost
will clip on dynamic material.

The limiter is the final stage in the processing chain and operates
independently of the SAE — it just ensures peaks stay below 0 dBFS.

If the limiter is buried inside v1's combined processing block and
hard to extract cleanly, document the implementation when porting to
v2 so it can be verified.

---

## 11. VERIFICATION CHECKLIST

After implementation, verify:

1. **Sonic match at default settings**: Render reference material
   (e.g., Nebraska vocal at Presence=0.5, Air=0.5) through Shine v2.
   Compare to Shine v1 reference render at same knob positions.
   Should sound equivalent. Spectral match within ~1-2 dB per band
   is acceptable.

2. **Mode tab behavior**: Clicking Vocal Clarity sets knobs to
   internal values 0.67/0.89, which display as "Lifted / Airy".
   Clicking Acoustic Detail sets knobs to 0.89/0.44, displaying as
   "Crisp / Balanced". Knob rotations and italic value words update
   visibly in the UI.

3. **Knob range usability**: Sweep both knobs from 0 to 1. Knob at 0
   should be effective bypass (just HPF). Knob at 1.0 should be a
   modest increase above default — not harsh.

4. **No clipping**: Render dynamic material at all knob positions. The
   output peak limiter should keep all peaks below 0 dBFS regardless
   of knob position.

5. **SAE responsiveness**: With SAE engaged, the adaptation indicator
   bars should show movement during signal — more activity on quieter
   passages (since presence/air modulate up when quiet).

6. **Baseline-intact**: With SAE disengaged, the output is deterministic
   and depends only on knob positions.

---

## 12. UI

UI implementation is specified separately in `SHINE_UI_SPEC.md`.
Notable points relevant to this DSP spec:

- The two knobs use normalized 0–1 ranges
- Parameter value vocabulary words display the knob's current setting
- The adaptation bars below the knobs visualize the SAE's per-parameter
  modulation envelope (Presence bar and Air bar, both showing the
  inverted curve behavior)

---

## 13. SUMMARY

Shine v2 = Shine v1's static DSP + the Saelin Adaptive Engine module
with inverted-flag parameter registration, replacing the v1 ML model.
Static DSP is unchanged. UI is updated. Output limiter is preserved.

No new DSP work needed. The bulk of v2 is wiring the SAE in place of
the ML model and removing the ML model from the build.

---

*End of Shine SAE integration specification.*
