/*
  ---------------------------------------------------------------------------
  Teensy Formant Filter Demo
  ---------------------------------------------------------------------------
  Demonstrates:
    - Continuous vowel morphing (A–E–I–O–U)
    - Resonance (Q) modulation
    - Brightness (vocal tract length) morph
    - Voice type morph (male -> female -> child)
  ---------------------------------------------------------------------------
*/
#include <Audio.h>
#include "AudioFilterFormant.h"


AudioSynthWaveform       osc1;
AudioFilterFormant       formant;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(osc1, formant);
AudioConnection          patchCord2(formant, 0, i2s1, 0);
AudioConnection          patchCord3(formant, 0, i2s1, 1);
// enable for teensy audio shield
//AudioControlSGTL5000     sgtl5000_1;

void setup() {
  AudioMemory(30);
  
// enable for teensy audio shield
//  sgtl5000_1.enable();
//  sgtl5000_1.volume(0.5);

  osc1.begin(WAVEFORM_SAWTOOTH);
  osc1.frequency(110.0);
  osc1.amplitude(0.6);

  formant.setGain(1.0f);
  formant.setQ(6.0f);
}

void loop() {
  static float t = 0.0f;
  t += 0.0025f;

  float vowel = (sinf(t * 0.7f) * 0.5f + 0.5f) * 4.0f;
  float q = 0.1f + (sinf(t * 0.5f) * 0.5f + 0.5f) * 19.9f;
  float bright = sinf(t * 0.3f) * 12.0f;
  float morph = (sinf(t * 0.25f) * 0.5f + 0.5f);

  formant.setVowel(vowel);
  formant.setQ(q);
  formant.setBrightness(bright);
  formant.setFormantMorph(morph);

  delay(10);
}
