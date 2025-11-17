#include <Audio.h>
#include "AudioFilterFormant.h"
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// AudioControlSGTL5000 sgtl5000_1;

AudioSynthWaveform     osc;
AudioSynthWaveformSine lfoVowel, lfoBright;
AudioFilterFormant     formant;
AudioOutputI2S         i2s1;

AudioConnection pc1(osc,      0, formant, 0);
AudioConnection pc2(lfoVowel, 0, formant, 1);
AudioConnection pc3(lfoBright,0, formant, 2);
AudioConnection pc4(formant,  0, i2s1, 0);
AudioConnection pc5(formant,  0, i2s1, 1);

void noteOn(uint8_t note, uint8_t vel) {
  float freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);
  osc.frequency(freq);
  osc.amplitude(0.6f * (vel / 127.0f));
}
void noteOff(uint8_t note) {
  (void)note;
}

void handleCC(uint8_t cc, uint8_t val) {
  float x = val / 127.0f;
  switch (cc) {
    case 21: formant.setVowel(x * 4.0f); break;
    case 22: formant.setBrightness(x * 48.0f - 24.0f); break;
    case 23: formant.setFormantMorph(x); break;
    case 24: formant.setQ(0.1f + x * 19.9f); break;
    case 25: formant.setMix(x); break;
    case 26: formant.setBreath(x); break;
    case 27: formant.setConsonant(x, 0.0f, 0.0f); break;
    case 28: formant.setConsonant(0.0f, x, 0.0f); break;
    case 70: formant.setVowelModDepth(x * 4.0f); break;
    case 71: formant.setBrightnessModDepth(x * 24.0f); break;
    case 72: formant.setGain(0.2f + x * 1.8f); break;
    case 73: formant.setConsonant(0.0f, 0.0f, x); break;
    case 91: formant.setGain(0.5f + x * 1.5f); break;
    case 93: formant.setMix(x); break;
    default: break;
  }
}

void handleMidi() {
  while (usbMIDI.read()) {
    switch (usbMIDI.getType()) {
      case usbMIDI.NoteOn:
        if (usbMIDI.getData2() > 0)
          noteOn(usbMIDI.getData1(), usbMIDI.getData2());
        else
          noteOff(usbMIDI.getData1());
        break;
      case usbMIDI.NoteOff:
        noteOff(usbMIDI.getData1());
        break;
      case usbMIDI.ControlChange:
        handleCC(usbMIDI.getData1(), usbMIDI.getData2());
        break;
    }
  }
  if (MIDI.read()) {
    byte type = MIDI.getType();
    byte d1   = MIDI.getData1();
    byte d2   = MIDI.getData2();
    if (type == midi::NoteOn && d2 > 0) noteOn(d1, d2);
    if (type == midi::NoteOff || (type == midi::NoteOn && d2 == 0)) noteOff(d1);
    if (type == midi::ControlChange) handleCC(d1, d2);
  }
}

elapsedMillis cpuTimer;

void setup() {
  AudioMemory(40);
  Serial.begin(115200);
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // sgtl5000_1.enable();
  // sgtl5000_1.volume(0.5f);

  osc.begin(WAVEFORM_SAWTOOTH);
  osc.frequency(55.0f);
  osc.amplitude(0.0f);

  lfoVowel.frequency(2.0f);
  lfoVowel.amplitude(1.0f);
  lfoBright.frequency(0.5f);
  lfoBright.amplitude(1.0f);

  formant.setMix(1.0f);
  formant.setVowel(0.8f);
  formant.setQ(12.0f);
  formant.setFormantMorph(0.2f);
  formant.setBrightness(-4.0f);
  formant.setBreath(0.08f);
  formant.setConsonant(0.08f, 0.06f, 0.02f);
  formant.setVowelModDepth(2.0f);
  formant.setBrightnessModDepth(6.0f);
}

void loop() {
  handleMidi();
  if (cpuTimer > 1000) {
    cpuTimer = 0;
    Serial.printf("TalkingBass CPU: %.2f%% max %.2f%%, Mem: %u\n",
                  AudioProcessorUsage(), AudioProcessorUsageMax(),
                  AudioMemoryUsageMax());
  }
}
