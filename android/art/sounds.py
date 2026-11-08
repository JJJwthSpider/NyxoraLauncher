#!/usr/bin/env python3
"""The interface sounds of Vellora, synthesized here (short, soft, 8-bit like the pixel look), the same files for the
phone (android/assets/sounds) and the desktop launcher (launcher/resources/sculk_theme/sounds):

  nav     switching tabs: two quick rising blips
  open    a screen or a window opens: a sweep up
  close   it closes: a sweep down
  click   a small tick
  toggle  a switch
  wear    an item put on in the wardrobe: a sparkle arpeggio
  launch  Play: a bright rising chord

    python3 android/art/sounds.py
"""
import math
import os
import random
import struct
import wave

RATE = 22050
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = [os.path.join(HERE, "..", "assets", "sounds"), os.path.join(HERE, "..", "..", "launcher", "resources", "sculk_theme", "sounds")]


def tone(freq_from, freq_to, ms, wave_kind="mix", volume=0.3, decay=6.0, attack_ms=2.0):
    """one note gliding from freq_from to freq_to, with a soft attack and an exponential fade"""
    n = int(RATE * ms / 1000)
    out = []
    phase = 0.0
    for i in range(n):
        t = i / n
        f = freq_from + (freq_to - freq_from) * t
        phase += f / RATE
        p = phase % 1.0
        sine = math.sin(2 * math.pi * p)
        square = 1.0 if p < 0.25 else -1.0
        triangle = 4 * abs(p - 0.5) - 1
        if wave_kind == "sine":
            s = sine
        elif wave_kind == "triangle":
            s = triangle
        else:  # a square softened by a sine: 8-bit, but not harsh
            s = 0.35 * square + 0.65 * sine
        env = min(1.0, (i / RATE * 1000) / attack_ms) * math.exp(-decay * t)
        out.append(s * env * volume)
    return out


def silence(ms):
    return [0.0] * int(RATE * ms / 1000)


def mix(*tracks):
    n = max(len(t) for t in tracks)
    return [sum(t[i] if i < len(t) else 0.0 for t in tracks) for i in range(n)]


def save(name, samples):
    # a short fade at the very end, so no click
    tail = min(len(samples), int(RATE * 0.004))
    for k in range(tail):
        samples[-1 - k] *= k / tail
    data = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples)
    for folder in OUT:
        os.makedirs(folder, exist_ok=True)
        with wave.open(os.path.join(folder, name + ".wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(RATE)
            w.writeframes(data)


def main():
    random.seed(7)
    save("nav", tone(660, 700, 38, volume=0.26, decay=3) + tone(990, 1040, 55, volume=0.24, decay=5))
    save("open", tone(420, 880, 120, "triangle", volume=0.32, decay=2.5))
    save("close", tone(860, 420, 105, "triangle", volume=0.3, decay=2.5))
    noise = [(random.random() * 2 - 1) * 0.08 * math.exp(-i / 40) for i in range(int(RATE * 0.012))]
    save("click", mix(tone(1800, 1700, 28, "sine", volume=0.22, decay=9), noise))
    save("toggle", tone(1200, 1200, 30, volume=0.2, decay=4) + tone(1600, 1600, 40, volume=0.2, decay=6))
    notes = [1046.5, 1318.5, 1568.0, 2093.0]
    wear = []
    for k, f in enumerate(notes):
        wear += tone(f, f, 70 if k == len(notes) - 1 else 42, "triangle", volume=0.2, decay=4 if k == len(notes) - 1 else 2)
    save("wear", wear)
    save("launch", mix(tone(523, 1046, 260, volume=0.22, decay=2.2), silence(40) + tone(659, 1318, 220, "triangle", volume=0.16, decay=2.5),
                       silence(80) + tone(784, 1568, 200, "sine", volume=0.14, decay=3)))


if __name__ == "__main__":
    main()
