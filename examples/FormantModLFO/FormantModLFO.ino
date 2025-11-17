#include <Audio.h>
#include "AudioFilterFormant.h"

// Signal Sources
AudioSynthWaveform       osc1;        // main synth source
AudioSynthWaveformSine   lfoVowel;    // vowel LFO
AudioSynthWaveformSine   lfoBright;   // brightness LFO

// Formant Filter and Output
AudioFilterFormant       formant;
AudioOutputI2S           i2s1;

// Patch routing: audio + 2 modulation inputs
AudioConnection patchCord1(osc1,      0, formant, 0);
AudioConnection patchCord2(lfoVowel,  0, formant, 1);
AudioConnection patchCord3(lfoBright, 0, formant, 2);
AudioConnection patchCord4(formant,   0, i2s1, 0);
AudioConnection patchCord5(formant,   0, i2s1, 1);

//AudioControlSGTL5000     sgtl5000_1;

void setup() {
  AudioMemory(40);
//  sgtl5000_1.enable();
//  sgtl5000_1.volume(0.5);

  // Main oscillator
  osc1.begin(WAVEFORM_SAWTOOTH);
  osc1.frequency(110.0f);
  osc1.amplitude(0.6f);

  // LFOs: very slow sine modulations
  lfoVowel.frequency(0.25f);   // cycles vowel every ~4s
  lfoVowel.amplitude(1.0f);

  lfoBright.frequency(0.13f);  // independent brightness cycle
  lfoBright.amplitude(1.0f);

  // Formant filter setup
  formant.setMix(1.0f);
  formant.setVowel(2.0f);         // center around "I"
  formant.setQ(6.0f);
  formant.setFormantMorph(0.35f); // between male/female
  formant.setBrightness(0.0f);

  // Modulation depths:
  formant.setVowelModDepth(1.8f);      // ~ ±1.8 vowel range
  formant.setBrightnessModDepth(6.0f); // ±6 semitone brightness
}

void loop() {
  // Nothing needed. Audio runs in background.
}
