#!/usr/bin/env python3
"""The sounds and the music of Vellora, made here (no samples from anyone): soft and round, like a music box in a
quiet room. The same files for the phone (android/assets) and the desktop launcher (launcher/resources/sculk_theme).

UI sounds (22050 Hz, mono): marimba "tok"s and small bells with a little room echo.
  nav     switching tabs: two soft marimba notes
  open    a screen or a window opens: a rising chime
  close   it closes: a soft falling pair
  click   a quiet wooden tick
  toggle  a switch: two notes up
  wear    an item put on: a sparkling bell arpeggio
  launch  Play: a warm chord with a bell on top

Music: "Crimson Rain", a calm ambient loop in D minor (Dm - Bb - F - C) at 76 BPM: soft pads, a music-box melody, a
round bass and a hush of rain, with a long reverb; the end flows into the start, so it loops without a seam.
  android/assets/music/vellora_theme.ogg   (Vorbis, small)
  launcher/resources/sculk_theme/music/vellora_theme.wav   (22050 Hz mono PCM, for waveOut / paplay)

    python3 android/art/sounds.py      (needs numpy; ffmpeg for the .ogg)
"""
import os
import subprocess
import wave

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ANDROID = os.path.join(HERE, "..", "assets")
DESKTOP = os.path.join(HERE, "..", "..", "launcher", "resources", "sculk_theme")


def note(name):
    """'D4' -> Hz"""
    names = {"C": -9, "C#": -8, "Db": -8, "D": -7, "D#": -6, "Eb": -6, "E": -5, "F": -4, "F#": -3, "Gb": -3, "G": -2, "G#": -1, "Ab": -1,
             "A": 0, "A#": 1, "Bb": 1, "B": 2}
    pitch, octave = name[:-1], int(name[-1])
    return 440.0 * 2 ** ((names[pitch] + 12 * (octave - 4)) / 12)


# ---------------------------------------------------------------- building blocks

def env(n, sr, attack, decay):
    t = np.arange(n) / sr
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    return a * np.exp(-t / decay)


def marimba(f, sr, dur=0.5, vol=0.3):
    n = int(sr * dur)
    t = np.arange(n) / sr
    tone = np.sin(2 * np.pi * f * t) * env(n, sr, 0.002, 0.18)
    tone += 0.35 * np.sin(2 * np.pi * f * 3.93 * t) * env(n, sr, 0.001, 0.035)   # the wooden bar's bright partial
    tone += 0.12 * np.sin(2 * np.pi * f * 9.2 * t) * env(n, sr, 0.001, 0.012)
    return tone * vol


def bell(f, sr, dur=1.2, vol=0.25, ratio=3.5, index=2.2, decay=0.45):
    """an FM bell / music box tine"""
    n = int(sr * dur)
    t = np.arange(n) / sr
    idx = index * np.exp(-t / (decay * 0.5))
    tone = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * f * ratio * t))
    return tone * env(n, sr, 0.003, decay) * vol


def pad(freqs, sr, dur, vol=0.06, attack=1.4, release=1.6):
    """a warm pad: detuned sine + soft triangle voices, slow in and out"""
    n = int(sr * dur)
    t = np.arange(n) / sr
    out = np.zeros(n)
    for f in freqs:
        for detune in (-0.0035, 0.0, 0.0041):
            ph = 2 * np.pi * f * (1 + detune) * t
            out += np.sin(ph) + 0.18 * np.sin(2 * ph) + 0.06 * np.sin(3 * ph)
    shape = np.minimum(np.clip(t / attack, 0, 1), np.clip((dur - t) / release, 0, 1))
    shape = shape * shape * (3 - 2 * shape)  # smooth
    return out * shape * vol / (3 * len(freqs)) * 3


def smooth(x, taps):
    """a gentle low-pass: a moving average"""
    k = np.ones(taps) / taps
    return np.convolve(x, k, mode="same")


def comb(x, delay, g):
    y = np.copy(x)
    for start in range(delay, len(x), delay):
        end = min(start + delay, len(x))
        y[start:end] += g * y[start - delay:end - delay]
    return y


def allpass(x, delay, g):
    y = np.zeros_like(x)
    xd = np.concatenate([np.zeros(delay), x[:-delay]])
    y[:delay] = -g * x[:delay] + xd[:delay]
    for start in range(delay, len(x), delay):
        end = min(start + delay, len(x))
        y[start:end] = -g * x[start:end] + xd[start:end] + g * y[start - delay:end - delay]
    return y


def reverb(x, sr, wet=0.3, size=1.0, damp=7):
    """a small Schroeder room: four combs, two all-passes, the tail a little darker"""
    scale = sr / 44100 * size
    tail = np.zeros_like(x)
    for d, g in ((1557, 0.84), (1617, 0.83), (1491, 0.85), (1422, 0.84)):
        tail += comb(x, max(1, int(d * scale)), g)
    tail = smooth(tail / 4, damp)
    for d in (225, 556):
        tail = allpass(tail, max(1, int(d * scale)), 0.5)
    return x * (1 - wet) + tail * wet


def place(track, sound, at):
    end = min(len(track), at + len(sound))
    if at < len(track):
        track[at:end] += sound[:end - at]


def write_wav(path, x, sr):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = (np.clip(x, -1, 1) * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())


def finish(x, sr, peak):
    fade = int(sr * 0.01)
    x[-fade:] *= np.linspace(1, 0, fade)
    m = np.max(np.abs(x))
    return x * (peak / m) if m > 0 else x


# ---------------------------------------------------------------- the UI sounds

def ui_sounds():
    sr = 22050

    def track(seconds):
        return np.zeros(int(sr * seconds))

    s = {}
    t = track(0.45)
    place(t, marimba(note("A5"), sr, 0.4, 0.5), 0)
    place(t, marimba(note("E6"), sr, 0.4, 0.42), int(sr * 0.055))
    s["nav"] = finish(reverb(t, sr, 0.22, 0.7), sr, 0.42)

    t = track(0.8)
    for k, n in enumerate(("D5", "F#5", "A5", "D6")):
        place(t, bell(note(n), sr, 0.7, 0.4 - k * 0.04, ratio=2.0, index=1.2, decay=0.25), int(sr * 0.045 * k))
    s["open"] = finish(reverb(t, sr, 0.25, 0.8), sr, 0.4)

    t = track(0.55)
    place(t, marimba(note("A5"), sr, 0.45, 0.45), 0)
    place(t, marimba(note("D5"), sr, 0.5, 0.45), int(sr * 0.07))
    s["close"] = finish(reverb(t, sr, 0.22, 0.7), sr, 0.38)

    t = track(0.16)
    n = int(sr * 0.06)
    tick = np.sin(2 * np.pi * 1350 * np.arange(n) / sr) * env(n, sr, 0.0008, 0.012)
    noise = smooth(np.random.default_rng(3).standard_normal(n), 4) * env(n, sr, 0.0005, 0.006) * 0.25
    place(t, tick + noise, 0)
    s["click"] = finish(reverb(t, sr, 0.12, 0.5), sr, 0.3)

    t = track(0.45)
    place(t, marimba(note("C6"), sr, 0.35, 0.45), 0)
    place(t, marimba(note("G6"), sr, 0.4, 0.4), int(sr * 0.06))
    s["toggle"] = finish(reverb(t, sr, 0.2, 0.6), sr, 0.34)

    t = track(1.2)
    for k, n in enumerate(("D6", "F#6", "A6", "C#7", "D7")):
        place(t, bell(note(n), sr, 0.9, 0.3, ratio=2.0, index=0.9, decay=0.35), int(sr * 0.05 * k))
    shimmer = np.random.default_rng(5).standard_normal(len(t)) * env(len(t), sr, 0.05, 0.25) * 0.006
    s["wear"] = finish(reverb(t + smooth(shimmer, 2), sr, 0.3, 0.9), sr, 0.38)

    t = track(1.6)
    place(t, pad([note("D4"), note("F#4"), note("A4")], sr, 1.4, 0.5, attack=0.08, release=0.9), 0)
    place(t, pad([note("D3")], sr, 1.4, 0.4, attack=0.06, release=0.9), 0)
    place(t, bell(note("A5"), sr, 1.2, 0.3, ratio=2.0, index=1.0, decay=0.5), int(sr * 0.12))
    place(t, bell(note("D6"), sr, 1.2, 0.25, ratio=2.0, index=1.0, decay=0.5), int(sr * 0.24))
    s["launch"] = finish(reverb(t, sr, 0.3, 1.0), sr, 0.42)

    for name, x in s.items():
        write_wav(os.path.join(ANDROID, "sounds", name + ".wav"), x, sr)
        write_wav(os.path.join(DESKTOP, "sounds", name + ".wav"), x, sr)


# ---------------------------------------------------------------- the music

def music():
    sr = 32000
    bpm = 76
    beat = 60 / bpm
    bar = 4 * beat
    chords = [  # (pad notes, bass, arpeggio tones)
        (["D3", "F3", "A3", "D4"], "D2", ["D5", "F5", "A5", "C6", "D6", "E5"]),
        (["Bb2", "D3", "F3", "A3"], "Bb1", ["D5", "F5", "Bb5", "A5", "C6", "F5"]),
        (["F3", "A3", "C4", "E4"], "F2", ["C5", "F5", "A5", "E5", "G5", "C6"]),
        (["C3", "E3", "G3", "C4"], "C2", ["E5", "G5", "C6", "D5", "G5", "E5"]),
    ]
    rounds = 2
    length = int(sr * bar * 2 * len(chords) * rounds)
    tail = int(sr * 5)
    mix = np.zeros(length + tail)
    rng = np.random.default_rng(76)
    for r in range(rounds):
        for c, (padnotes, bass, arp) in enumerate(chords):
            start = int(sr * bar * 2 * (c + r * len(chords)))
            # the pad, a little longer than its two bars so the chords melt into each other
            place(mix, pad([note(n) for n in padnotes], sr, bar * 2 + 1.6, 0.11), max(0, start - int(sr * 0.4)))
            # the bass: soft, on the first and third beat
            for b in (0, 2, 4, 6):
                n = int(sr * beat * 1.8)
                t = np.arange(n) / sr
                f = note(bass)
                tone = (np.sin(2 * np.pi * f * t) + 0.25 * np.sin(4 * np.pi * f * t)) * env(n, sr, 0.02, 0.6)
                place(mix, tone * (0.16 if b % 4 == 0 else 0.1), start + int(sr * beat * b))
            # the music box: eighths over the chord, some left out, a little human in time and touch
            pattern = rng.permutation(len(arp))
            for k in range(16):
                if rng.random() < (0.32 if r == 0 else 0.22):
                    continue
                tone = arp[pattern[k % len(arp)]] if k % 4 else arp[0]
                if r == 1 and k % 8 == 6:
                    tone = tone[:-1] + str(int(tone[-1]) + 1)  # the second round answers an octave up now and then
                at = start + int(sr * (beat / 2 * k + rng.normal(0, 0.008)))
                vel = 0.12 + rng.random() * 0.05
                place(mix, bell(note(tone), sr, 1.6, vel, ratio=3.0, index=1.1, decay=0.55), max(0, at))
    # rain: a soft hush, slowly breathing
    n = len(mix)
    hush = smooth(rng.standard_normal(n), 9) - smooth(rng.standard_normal(n), 60) * 0.5
    hush *= 0.012 * (0.75 + 0.25 * np.sin(2 * np.pi * np.arange(n) / sr / 13))
    mix += hush
    mix = reverb(mix, sr, wet=0.38, size=1.6, damp=9)
    # the tail flows into the start: a seamless loop
    loop = mix[:length].copy()
    loop[:tail] += mix[length:length + tail]
    loop = loop / np.max(np.abs(loop)) * 0.85
    music_dir_a = os.path.join(ANDROID, "music")
    os.makedirs(music_dir_a, exist_ok=True)
    tmp = os.path.join(music_dir_a, "vellora_theme_tmp.wav")
    write_wav(tmp, loop, sr)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", tmp, "-c:a", "libvorbis", "-q:a", "3", os.path.join(music_dir_a, "vellora_theme.ogg")], check=True)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", tmp, "-ar", "22050", "-ac", "1", "-c:a", "pcm_s16le",
                    os.path.join(DESKTOP, "music", "vellora_theme.wav")] if os.makedirs(os.path.join(DESKTOP, "music"), exist_ok=True) is None else [],
                   check=True)
    os.remove(tmp)
    print("music: %.1f s" % (length / sr))


if __name__ == "__main__":
    ui_sounds()
    music()
