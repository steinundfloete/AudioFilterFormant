# 🎤 AudioFilterFormant

A smooth, morphable **formant filter** for the [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html).

This filter simulates human vowel resonances (A–E–I–O–U) using three parallel band-pass filters with continuous parameter morphing and vocal-inspired extras.

## ✨ Features

- Continuous vowel morphing (A → E → I → O → U)
- Continuous voice-type morphing (male → female → child)
- Resonance (Q) with RMS loudness compensation
- Brightness (vocal tract length) control in semitones
- Exponential formant scaling for realistic pitch behavior
- Smooth coefficient morphing (no zipper noise)
- Dry/Wet mix control
- Audio-rate modulation inputs for vowel and brightness (2 extra AudioStream inputs)
- Breath & consonant excitation (S, F, nasal-ish)
- Bypass (instant dry path, low CPU)
- Lightweight (~few % CPU on Teensy 4.1 per voice)

## 📦 Installation

1. Copy the `AudioFilterFormant` folder into your Arduino `libraries/` directory.
2. Restart the Arduino IDE.
3. Open **File → Examples → AudioFilterFormant → FormantDemo**.

## 🧪 Basic Example

```cpp
#include <Audio.h>
#include "AudioFilterFormant.h"

AudioSynthWaveform osc1;
AudioFilterFormant formant;
AudioOutputI2S i2s1;
AudioConnection patchCord1(osc1, formant);
AudioConnection patchCord2(formant, 0, i2s1, 0);
AudioConnection patchCord3(formant, 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;

void setup() {
  AudioMemory(30);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  osc1.begin(WAVEFORM_SAWTOOTH);
  osc1.frequency(110.0);
  osc1.amplitude(0.6);

  formant.setGain(1.0f);
  formant.setMix(1.0f);
  formant.setVowel(0.0f);           // "A"
  formant.setQ(6.0f);
  formant.setFormantMorph(0.3f);    // between male/female
  formant.setBrightness(4.0f);      // a bit brighter
  formant.setBreath(0.15f);
  formant.setConsonant(0.12f, 0.08f, 0.05f);
}

void loop() {
  static float t = 0.0f;
  t += 0.0025f;

  float vowel = (sinf(t * 0.7f) * 0.5f + 0.5f) * 4.0f;
  formant.setVowel(vowel);

  delay(10);
}
```

## 🎛 Controls

| Function                    | Range               | Description                                 |
|----------------------------|---------------------|---------------------------------------------|
| `setVowel(float)`          | 0.0 – 4.0           | Morphs between vowels (A–E–I–O–U)           |
| `setQ(float)`              | 0.1 – 20.0          | Resonance / sharpness                       |
| `setFormantMorph(float)`   | 0.0 – 1.0           | Morph male → female → child                  |
| `setBrightness(float)`     | −24 – +24 semitones | Adjusts vocal tract length / brightness     |
| `setMix(float)`            | 0.0 – 1.0           | Dry/wet mix (0=dry, 1=wet)                  |
| `setGain(float)`           | any                 | Output gain                                 |
| `setVowelModDepth(float)`  | 0.0 … 4.0           | Audio-rate vowel modulation depth           |
| `setBrightnessModDepth(float)` | semitones       | Audio-rate brightness modulation depth      |
| `setBreath(float)`         | 0.0 – 1.0           | Amount of whisper / breath noise            |
| `setConsonant(float,float,float)` | 0.0 – 1.0   | S, F, nasal noise levels                    |
| `setBypass(bool)`          | false/true          | Full bypass (or via `setMix(0.0f)`)         |

## 🔌 Modulation Inputs

`AudioFilterFormant` uses **3 inputs**:

- Input 0: audio signal to be filtered
- Input 1: vowel modulation (16-bit audio, −1…+1 scaled internally)
- Input 2: brightness modulation (16-bit audio, −1…+1 scaled internally)

Example wiring for modulation:

```cpp
AudioSynthWaveform lfoVowel;
AudioSynthWaveform lfoBright;
AudioFilterFormant formant;

AudioConnection c1(sourceSignal, 0, formant, 0);
AudioConnection c2(lfoVowel,    0, formant, 1);
AudioConnection c3(lfoBright,   0, formant, 2);

void setup() {
  formant.setVowelModDepth(1.5f);      // span ~1.5 vowel units
  formant.setBrightnessModDepth(6.0f); // ±6 semitones
}
```

## 📜 License

MIT License © 2025 ChatGPT
