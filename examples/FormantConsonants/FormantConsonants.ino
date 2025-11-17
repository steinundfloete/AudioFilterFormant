#include <Audio.h>
#include "AudioFilterFormant.h"

AudioSynthWaveform       osc1;
AudioFilterFormant       formant;
AudioOutputI2S           i2s1;

AudioConnection patchCord1(osc1,    0, formant, 0);
AudioConnection patchCord2(formant, 0, i2s1, 0);
AudioConnection patchCord3(formant, 0, i2s1, 1);

// AudioControlSGTL5000     sgtl5000_1;

void setup() {
  AudioMemory(30);
  // sgtl5000_1.enable();
  // sgtl5000_1.volume(0.5);

  osc1.begin(WAVEFORM_SAWTOOTH);
  osc1.frequency(165.0f);   // E3
  osc1.amplitude(0.55f);

  formant.setMix(1.0f);
  formant.setVowel(0.0f);         // "A"
  formant.setQ(7.0f);
  formant.setFormantMorph(0.4f);
  formant.setBrightness(2.0f);

  // Add realistic mouth noise:
  formant.setBreath(0.72f);                 // gentle whispered airflow
  formant.setConsonant(0.18f, 0.10f, 0.06f); // S, F, nasal
}

void loop() {
  static float t = 0.0f;
  t += 0.02f;

  float vowel = (sinf(t * 0.5f) * 0.5f + 0.5f) * 4.0f;
  formant.setVowel(vowel);

  delay(8);
}
