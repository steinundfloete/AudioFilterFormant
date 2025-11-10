# 🎤 AudioFilterFormant

A smooth, morphable **formant filter** for the [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html).

This filter simulates human vowel resonances (A–E–I–O–U) using three parallel band-pass filters with continuous parameter morphing.

---

## ✨ Features

- Continuous vowel morphing (A → E → I → O → U)
- Continuous voice-type morphing (male → female → child)
- Resonance (Q) with loudness compensation
- Brightness control (vocal tract length ±24 semitones)
- Smooth coefficient morphing (no zipper noise)
- Low CPU (~3 % on Teensy 4.1)

---

### Controls
| Function | Range | Description |
|-----------|--------|-------------|
| `setVowel(float)` | 0.0 – 4.0 | Morphs between vowels (A–E–I–O–U) |
| `setQ(float)` | 0.1 – 20.0 | Resonance / sharpness |
| `setFormantMorph(float)` | 0.0 – 1.0 | Morph between male → female → child voice |
| `setBrightness(float)` | −24 – +24 semitones | Adjusts vocal tract length |
| `setMix(float)` | 0.0 – 1.0 | Dry/wet mix (0=dry, 1=wet) |
| `setGain(float)` | any | Output gain |

---

## 📦 Installation

1. Download or clone this repository into your Arduino `libraries/` folder.
2. Restart the Arduino IDE.
3. Find the example under **File → Examples → AudioFilterFormant → FormantDemo**.

---

## 🧪 Example

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

  formant.setVowel(0.0f);
  formant.setQ(8.0f);
  formant.setFormantMorph(0.4f);
  formant.setBrightness(6.0f);
}

void loop() {
  static float v = 0.0f;
  v += 0.002f;
  if (v > 4.0f) v = 0.0f;
  formant.setVowel(v);
  delay(10);
}
