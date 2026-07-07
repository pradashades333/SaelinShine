# Saelin Gloss — UI Specification
**Version 1.0 — companion to Saelin Daylight Design System v2.0**

This document specifies the visual layout and copy for the Saelin Gloss
plugin window. It references the **Saelin Daylight Design System v2.0**
for all shared visual tokens (colors, typography, components, knob assets)
and the **Ember UI Specification v1.0** as the architectural template —
Gloss follows the same layout pattern with one structural difference:
**no mode tabs.**

Read this document alongside the design system and the Ember UI spec.

---

## 1. PRODUCT SCOPE

When implementing Gloss, apply the product scope `.p-gloss` to the
plugin root container. This cascades the Gloss accent (`#2E5F66`) to all
accent-colored elements via the `--accent` token.

```html
<div class="plugin-window p-gloss">
  <!-- Gloss UI -->
</div>
```

```css
.p-gloss {
  --accent: #2E5F66;
  --accent-soft: rgba(46, 95, 102, 0.12);
  --accent-line: rgba(46, 95, 102, 0.3);
}
```

The cool sub-accent (`#5A8CA0`) used for the second knob's indicator
(Air) is constant across all products — do not override it per-product.

---

## 2. PLUGIN WINDOW DIMENSIONS

**Native 1× size:** 520 × 360 px (same as Ember and Shine).

All other window properties (resizable, scaling, background, corner
radius, border) are identical to Ember UI Spec §2.

---

## 3. LAYOUT STRUCTURE

Gloss's layout is **five** stacked sections, top to bottom — one fewer
than Ember and Shine because there are no mode tabs:

```
┌─────────────────────────────────────────────────────────────┐
│  1. Header        — brand mark + product label              │
├─────────────────────────────────────────────────────────────┤
│  2. Adaptive      — indicator with pulsing dot              │
│     indicator       "ADAPTIVE PROCESSING"                    │
├─────────────────────────────────────────────────────────────┤
│  3. Knobs         — Coat (left) + Air (right)               │
│                     Each with label + value (italic)         │
├─────────────────────────────────────────────────────────────┤
│  4. Adaptation    — single bar (Onset)                      │
│     bar             Title "ADAPTATION" above                 │
├─────────────────────────────────────────────────────────────┤
│  5. Meters        — Output meter (centered)                 │
└─────────────────────────────────────────────────────────────┘
```

**Vertical rhythm:** Identical to Ember UI Spec §3, with the mode tabs
section omitted. The space freed by the missing mode tabs can be
absorbed by slightly more generous padding between remaining sections
(e.g., 22px between sections instead of 18px), giving Gloss a more
spacious, settled feel appropriate for a finishing tool.

**Horizontal alignment:** Same as Ember — header left-aligned, all other
sections centered.

---

## 4. SECTION SPECIFICATIONS

### 4.1 Header

**Position:** Top of window, full width minus padding.

**Contents (left-aligned):**
- Brand mark: "SAELIN" — Cormorant Garamond 22px / 600 / tracking 0.18em
  / UPPER / color `var(--ink)`
- Product label: "GLOSS" — DM Sans 11px / 500 / tracking 0.16em / UPPER
  / color `var(--accent)` — placed to the right of brand mark, with 12px
  gap between them, vertically aligned to brand mark baseline.

**Contents (right-aligned):** Empty for Gloss (no FREE badge).

**Bottom border:** 1px solid `var(--line-soft)` separating header from
adaptive indicator section.

### 4.2 Mode tabs

**Not present.** Gloss does not have mode tabs.

This is the only structural difference from Ember and Shine's layout.
The vertical space normally occupied by mode tabs is removed; subsequent
sections shift up accordingly.

### 4.3 Adaptive indicator

**Position:** Below header (where mode tabs would be on other products).

**Identical to Ember UI Spec §4.3.** The pulsing dot is `var(--accent)`
(Gloss teal). Label text is "ADAPTIVE PROCESSING".

### 4.4 Knobs section

**Position:** Below adaptive indicator. The main visual focus of the
plugin.

**Two knobs side by side:**
- **Coat** (left) — Gloss accent indicator (teal)
- **Air** (right) — Cool sub-accent indicator (`#5A8CA0`)

**Knob asset:** Use `saelin-knob-gloss.svg` for Coat, and the template
SVG with cool sub-accent override for Air.

**Knob size, group layout, horizontal gap, group centering:** Identical
to Ember UI Spec §4.4.

**Value labels:** Pull from the parameter vocabulary in Design System
§07. The Gloss vocabularies are already specified there:

**Coat (0 → 1, amount of finish):**
- 0.0–0.2: Light
- 0.2–0.4: Touch
- 0.4–0.6: Polish
- 0.6–0.8: Glaze
- 0.8–1.0: Lacquer

**Air (0 → 1, warm ↔ open):**
- 0.0–0.2: Intimate
- 0.2–0.4: Warm
- 0.4–0.6: Balanced
- 0.6–0.8: Open
- 0.8–1.0: Lifted

Value text color: `var(--ink-muted)`.

### 4.5 Adaptation bar

**Position:** Below knobs section.

**Title:** "ADAPTATION" — same typography as Ember UI Spec §4.5.

**Single adaptation bar, centered:**

This is a structural difference from Ember and Shine, which both have
two adaptation bars (one per knob). Gloss's SAE has a different role —
it modulates a single internal parameter (`transientEase`) rather than
the user knobs (see Gloss DSP spec §3.4). The single adaptation bar
visualizes the SAE's transient-detection activity.

**Bar properties:**
- Width: 80px (wider than Ember/Shine's 60px since it stands alone)
- Height: 4px
- Border radius: 3px
- Background (unfilled): rgba(26,22,18,0.06)
- Fill: `var(--accent)` (Gloss teal)

**Behavior:** Bar fill width corresponds to the current SAE
`transientEase` modulation value (0.0 = no fill, 1.0 = full fill).
The bar will spike briefly during detected onsets and fall back between
gestures. Visually this produces a more rhythmic "pulse" pattern than
Ember and Shine's bars, which move more smoothly with performance
intent.

**Label below bar:** "ONSET" — DM Sans 8px / 500 / tracking 0.08em /
UPPER / color `var(--ink-muted)`.

### 4.6 Meters

**Position:** Below adaptation bar.

**Identical to Ember UI Spec §4.6.** Single output meter, centered,
six vertical bars ascending. Bar fill uses `var(--accent)` (Gloss teal).

---

## 5. INTERACTION DETAILS

All interaction details (knob drag, AE engagement, bypass behavior)
are identical to Ember UI Spec §5.

**Gloss-specific note on default values:**
- Coat default: 0.5 (= "Polish")
- Air default: 0.5 (= "Balanced")

---

## 6. RESPONSIVE BEHAVIOR

Identical to Ember UI Spec §6.

---

## 7. WHAT'S DELIBERATELY ABSENT

Same list as Ember UI Spec §7, with these differences:

**Gloss DOES NOT have:**
- Mode tabs (only product without them)
- A FREE badge (Shine only)

Everything else in the "deliberately absent" list still applies:
- No mode tab subtitles (n/a — no tabs at all)
- No descriptor text under knobs
- No "Modulating Coat & Air" textual descriptor
- No Adaptive Engine on/off toggle in v1
- No input meter
- No presets dropdown in UI
- No preset name display
- No info or help icons
- No version number display

---

## 8. COPY REFERENCE

All visible text in the Gloss UI, in one place:

| Element | Text |
|---|---|
| Brand mark | `SAELIN` |
| Product label | `GLOSS` |
| Adaptive indicator | `ADAPTIVE PROCESSING` |
| Knob 1 label | `COAT` |
| Knob 2 label | `AIR` |
| Knob 1 values | `Light / Touch / Polish / Glaze / Lacquer` |
| Knob 2 values | `Intimate / Warm / Balanced / Open / Lifted` |
| Adaptation title | `ADAPTATION` |
| Adaptation bar label | `ONSET` |
| Meter label | `OUTPUT` |

**Do not introduce any other text.**

---

## 9. ASSETS PROVIDED

- `saelin-knob-gloss.svg` — Gloss-accent knob, vector
- `saelin-knob-template.svg` — Re-colorable template (for Air using
  cool sub-accent)
- `saelin-knob-gloss-512.png` — 512px raster export
- `saelin-knob-gloss-1024.png` — 1024px raster export
- Newsreader, DM Sans, Cormorant Garamond, JetBrains Mono fonts (Google
  Fonts, freely available)

---

## 10. REVIEW PROCESS

Same as Ember UI Spec §10. Build to spec, send screenshot at 1× scale
for review, adjustments as needed.

---

## 11. NOTES ON GLOSS'S ADAPTIVE BEHAVIOR

Gloss's adaptive behavior differs architecturally from Ember and Shine.
While Ember and Shine use the SAE to modulate their user-facing knobs
(Bloom/Hue and Presence/Air respectively), Gloss uses the SAE to modulate
a single internal parameter — `transientEase` — which gently releases
the glue compressor's gain reduction during detected onsets (see Gloss
DSP Spec §3.4 and §3.5).

This is why the Gloss UI has only one adaptation bar (Onset) instead of
two: the SAE has only one job in Gloss, and the bar visualizes that one
job. The user knobs (Coat and Air) are not modulated by the SAE in Gloss.

This architectural difference is intentional. Finishing tools work best
when applied consistently — having the SAE modulate the user's chosen
finishing amount would fight the product's purpose. Instead, the SAE
preserves transient breath through the compression, which IS the right
adaptive job for a finishing tool.

---

*End of Gloss UI specification. Reference the Saelin Daylight Design
System v2.0 and Ember UI Specification v1.0 for shared visual tokens
and layout patterns.*
