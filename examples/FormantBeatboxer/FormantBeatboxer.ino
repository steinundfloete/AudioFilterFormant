#include <Audio.h>
#include "AudioFilterFormant.h"
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
// AudioControlSGTL5000 sgtl5000_1;

AudioSynthWaveform   osc;
AudioEffectEnvelope  env;
AudioFilterFormant   formant;
AudioOutputI2S       i2s1;

AudioConnection c1(osc, env);
AudioConnection c2(env, 0, formant, 0);
AudioConnection c3(formant, 0, i2s1, 0);
AudioConnection c4(formant, 0, i2s1, 1);

const int NUM_STEPS = 16;
uint8_t pattern[NUM_STEPS] = {
  1,0,2,0,  1,0,3,0,  1,0,2,0,  1,3,2,0
}; // 1 kick, 2 snare-ish, 3 hat-ish
int stepIndex = 0;
elapsedMillis stepTimer, cpuTimer;

void triggerDrum(uint8_t type) {
  if (type == 0) return;
  if (type == 1) { // kick
    osc.frequency(60.0f);
    formant.setVowel(0.0f);
    formant.setBrightness(-8.0f);
    formant.setQ(8.0f);
    formant.setBreath(0.20f);
    formant.setConsonant(0.15f, 0.05f, 0.05f);
  } else if (type == 2) { // snare
    osc.frequency(180.0f);
    formant.setVowel(2.5f);
    formant.setBrightness(4.0f);
    formant.setQ(12.0f);
    formant.setBreath(0.35f);
    formant.setConsonant(0.30f, 0.22f, 0.05f);
  } else if (type == 3) { // hat
    osc.frequency(400.0f);
    formant.setVowel(3.8f);
    formant.setBrightness(10.0f);
    formant.setQ(15.0f);
    formant.setBreath(0.45f);
    formant.setConsonant(0.40f, 0.25f, 0.02f);
  }
  osc.amplitude(1.0f);
  env.noteOn();
}

void noteOn(uint8_t note, uint8_t vel) {
  // trigger by MIDI note: map any note to one of 3 drums
  uint8_t t = (note % 3) + 1;
  triggerDrum(t);
}
void noteOff(uint8_t note) {(void)note;}

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

void setup() {
  AudioMemory(40);
  Serial.begin(115200);
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // sgtl5000_1.enable();
  // sgtl5000_1.volume(0.5f);

  osc.begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
  osc.amplitude(0.0f);
  env.attack(2); env.decay(50); env.sustain(0.0f); env.release(30);

  formant.setMix(1.0f);
  formant.setQ(10.0f);
  formant.setFormantMorph(0.3f);
  formant.setBrightness(0.0f);
  formant.setBreath(0.20f);
  formant.setConsonant(0.25f, 0.15f, 0.08f);
}

void loop() {
  handleMidi();

  // 16tel bei ~140 BPM ≈ 107 ms
  if (stepTimer > 107) {
    stepTimer = 0;
    uint8_t t = pattern[stepIndex];
    triggerDrum(t);
    stepIndex++;
    if (stepIndex >= NUM_STEPS) stepIndex = 0;
  }

  if (cpuTimer > 1000) {
    cpuTimer = 0;
    Serial.printf("Beatbox CPU: %.2f%% max %.2f%%, Mem: %u\n",
                  AudioProcessorUsage(), AudioProcessorUsageMax(),
                  AudioMemoryUsageMax());
  }
}
