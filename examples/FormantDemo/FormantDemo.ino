#include <Audio.h>
#include "AudioFilterFormant.h"

// Basic demo: saw oscillator into formant filter, with automatic morphing

AudioSynthWaveform       osc1;
AudioFilterFormant       formant;
AudioOutputI2S           i2s1;

AudioConnection          patchCord1(osc1, 0, formant, 0);
AudioConnection          patchCord2(formant, 0, i2s1, 0);
AudioConnection          patchCord3(formant, 0, i2s1, 1);

// AudioControlSGTL5000     sgtl5000_1;

void setup() {
  AudioMemory(30);
  // sgtl5000_1.enable();
  // sgtl5000_1.volume(0.5);

  osc1.begin(WAVEFORM_SAWTOOTH);
  osc1.frequency(110.0);
  osc1.amplitude(0.6);

  formant.setGain(1.0f);
  formant.setMix(1.0f);

  formant.setVowel(0.0f);            // start at "A"
  formant.setQ(6.0f);
  formant.setFormantMorph(0.3f);     // between male/female
  formant.setBrightness(0.0f);

  formant.setBreath(0.15f);
  formant.setConsonant(0.12f, 0.08f, 0.05f);

  formant.setVowelModDepth(0.0f);
  formant.setBrightnessModDepth(0.0f);
}

void loop() {
  static float t = 0.0f;
  t += 0.025f;

  float vowel = (sinf(t * 0.7f) * 0.5f + 0.5f) * 4.0f;
  float q = 0.1f + (sinf(t * 0.5f) * 0.5f + 0.5f) * 19.9f;
  float bright = sinf(t * 0.3f) * 8.0f;
  float morph = (sinf(t * 0.25f) * 0.5f + 0.5f);

  formant.setVowel(vowel);
  formant.setQ(q);
  formant.setBrightness(bright);
  formant.setFormantMorph(morph);

  delay(10);
}
