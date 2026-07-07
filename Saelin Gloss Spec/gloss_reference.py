"""
Saelin Gloss — Reference Python implementation.

This is the locked sound reference for the C++ implementation.
The C++ build should produce output matching this code's output
on the same input audio at the same knob positions.

Usage:
    python gloss_reference.py input.wav output.wav [coat] [air] [ae_engaged]

Default knob positions: coat=0.5, air=0.5, ae_engaged=True
"""
import sys
import numpy as np
import scipy.signal as sig
import soundfile as sf


def detect_onsets(x, sr):
    """SAE onset detector — returns 0..1 envelope of onset activity."""
    # Ultra-fast level for onset detection
    aV = np.exp(-1.0 / (sr * 0.020))
    vfast = 10 * np.log10(np.maximum(
        sig.lfilter([1 - aV], [1, -aV], x**2), 1e-12))
    # Derivative — positive when rising
    onset_raw = np.diff(vfast, prepend=vfast[0])
    onset_pos = np.maximum(onset_raw, 0)
    # Hold-and-decay
    aOn = np.exp(-1.0 / (sr * 0.080))  # 80ms hold
    onset_held = np.zeros_like(onset_pos)
    prev = 0.0
    for i in range(len(onset_pos)):
        prev = max(onset_pos[i] * 30, prev * aOn)
        onset_held[i] = prev
    # Normalize
    p95 = np.percentile(onset_held, 95)
    return np.clip(onset_held / (p95 + 1e-9), 0, 1.0)


def gloss_process(x, sr, coat=0.5, air=0.5, ae_engaged=True):
    """
    Single-channel Gloss processing.

    Args:
        x: input audio (1D numpy array)
        sr: sample rate
        coat: 0..1, Amount knob
        air: 0..1, Character knob
        ae_engaged: if True, AE eases compression on detected onsets

    Returns:
        output audio (1D numpy array)
    """
    # === Knob mappings ===
    coat_factor = 0.3 + 1.4 * coat
    air_factor_low = 1.3 - 0.6 * air
    air_factor_high = 0.5 + 1.0 * air

    # === Element 1: Defined Bottom ===
    # 2nd-order Butterworth HPF at 35 Hz
    sos_hp = sig.butter(2, 35, btype='high', fs=sr, output='sos')
    out = sig.sosfilt(sos_hp, x)

    # Low shelf at 150 Hz
    low_shelf_gain = 3.5 * coat_factor * air_factor_low
    A = 10**(low_shelf_gain / 40)
    w0 = 2 * np.pi * 150 / sr
    alpha = np.sin(w0) / (2 * 0.5)  # Q = 0.5
    cw = np.cos(w0)
    sA = np.sqrt(A)
    b0 = A * ((A + 1) - (A - 1) * cw + 2 * sA * alpha)
    b1 = 2 * A * ((A - 1) - (A + 1) * cw)
    b2 = A * ((A + 1) - (A - 1) * cw - 2 * sA * alpha)
    a0 = (A + 1) + (A - 1) * cw + 2 * sA * alpha
    a1 = -2 * ((A - 1) + (A + 1) * cw)
    a2 = (A + 1) + (A - 1) * cw - 2 * sA * alpha
    b = np.array([b0, b1, b2]) / a0
    a = np.array([1, a1 / a0, a2 / a0])
    out = sig.lfilter(b, a, out)

    # === Element 2: Harmonic Enrichment ===
    drive = np.clip(0.12 * coat_factor, 0.02, 0.30)
    # 2x oversample
    up = sig.resample_poly(out, 2, 1)
    # Asymmetric bias
    asym = 0.08
    biased = up + asym * drive * 0.5
    # Tanh waveshaper
    g = 1.0 + 6.0 * drive
    sat = np.tanh(biased * g) / g
    # DC removal at 5 Hz (oversampled domain)
    sos_dc = sig.butter(1, 5, btype='high', fs=sr * 2, output='sos')
    sat = sig.sosfilt(sos_dc, sat)
    # Downsample
    out = sig.resample_poly(sat, 1, 2)
    # RMS match to preserve level
    in_rms = np.sqrt(np.mean(x**2) + 1e-12)
    out_rms = np.sqrt(np.mean(out**2) + 1e-12)
    if out_rms > 1e-9:
        out = out * (in_rms / out_rms)

    # === Element 3: Sweetened Top Shelf ===
    high_shelf_gain = 3.5 * coat_factor * air_factor_high
    A = 10**(high_shelf_gain / 40)
    w0 = 2 * np.pi * 10000 / sr
    alpha = np.sin(w0) / (2 * 0.5)  # Q = 0.5
    cw = np.cos(w0)
    sA = np.sqrt(A)
    b0 = A * ((A + 1) + (A - 1) * cw + 2 * sA * alpha)
    b1 = -2 * A * ((A - 1) + (A + 1) * cw)
    b2 = A * ((A + 1) + (A - 1) * cw - 2 * sA * alpha)
    a0 = (A + 1) - (A - 1) * cw + 2 * sA * alpha
    a1 = 2 * ((A - 1) - (A + 1) * cw)
    a2 = (A + 1) - (A - 1) * cw - 2 * sA * alpha
    b = np.array([b0, b1, b2]) / a0
    a = np.array([1, a1 / a0, a2 / a0])
    out = sig.lfilter(b, a, out)

    # === Element 4: Glue Compression (with optional AE transient ease) ===
    # Threshold
    base_threshold = -18.0
    coat_threshold_offset = -4.0 * (coat - 0.5)
    air_threshold_offset = 2.0 * (air - 0.5)
    threshold = np.clip(
        base_threshold + coat_threshold_offset + air_threshold_offset,
        -24, -12
    )

    # Envelope follower (RMS-ish via squared signal)
    att_coef = np.exp(-1.0 / (sr * 0.050))   # 50ms attack
    rel_coef = np.exp(-1.0 / (sr * 0.090))   # 90ms release
    sq = out**2
    rms_state = 0.0
    env = np.zeros_like(out)
    for i in range(len(out)):
        s = sq[i]
        if s > rms_state:
            rms_state = att_coef * rms_state + (1 - att_coef) * s
        else:
            rms_state = rel_coef * rms_state + (1 - rel_coef) * s
        env[i] = np.sqrt(max(rms_state, 1e-12))
    env_db = 20 * np.log10(env + 1e-9)

    # Soft-knee gain reduction
    over = env_db - threshold
    knee = 6
    ratio = 1.7
    gr_base = np.where(
        over < -knee / 2, 0,
        np.where(
            over > knee / 2,
            over * (1 - 1 / ratio),
            (over + knee / 2)**2 / (2 * knee) * (1 - 1 / ratio)
        )
    )

    # AE: ease GR on detected onsets
    if ae_engaged:
        onset_env = detect_onsets(x, sr)  # AE analyzes INPUT signal
        gr_factor = 1.0 - onset_env * 0.7  # max 70% ease during onsets
        gr = gr_base * gr_factor
    else:
        gr = gr_base

    # Apply gain reduction
    pre_rms = np.sqrt(np.mean(out**2) + 1e-12)
    out = out * 10**(-gr / 20)
    post_rms = np.sqrt(np.mean(out**2) + 1e-12)
    # Make-up gain
    if post_rms > 1e-9:
        out = out * (pre_rms / post_rms)

    return out.astype(np.float32)


def process_file(input_path, output_path, coat=0.5, air=0.5, ae_engaged=True):
    """Process a WAV file through Gloss."""
    raw, sr = sf.read(input_path)

    if raw.ndim == 1:
        out = gloss_process(raw, sr, coat, air, ae_engaged)
    else:
        # Process each channel independently
        out = np.zeros_like(raw, dtype=np.float32)
        for ch in range(raw.shape[1]):
            out[:, ch] = gloss_process(raw[:, ch], sr, coat, air, ae_engaged)

    sf.write(output_path, out, sr, subtype='PCM_24')
    print(f"Processed {input_path} → {output_path}")
    print(f"  Coat: {coat}, Air: {air}, AE engaged: {ae_engaged}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python gloss_reference.py input.wav output.wav "
              "[coat=0.5] [air=0.5] [ae_engaged=1]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    coat = float(sys.argv[3]) if len(sys.argv) > 3 else 0.5
    air = float(sys.argv[4]) if len(sys.argv) > 4 else 0.5
    ae = bool(int(sys.argv[5])) if len(sys.argv) > 5 else True

    process_file(input_file, output_file, coat, air, ae)
