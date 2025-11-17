#include <Audio.h>
#include "AudioFilterFormant.h"
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// Uncomment if you use the SGTL5000 audio shield
// AudioControlSGTL5000 sgtl5000_1;

const int NUM_VOICES = 4;

AudioSynthWaveform   osc[NUM_VOICES];
AudioEffectEnvelope  env[NUM_VOICES];
AudioFilterFormant   formant[NUM_VOICES];
AudioMixer4          mixL, mixR;
AudioOutputI2S       i2s1;

AudioConnection* pc[NUM_VOICES * 3];
AudioConnection    cOutL(mixL, 0, i2s1, 0);
AudioConnection    cOutR(mixR, 0, i2s1, 1);

struct Voice {
  bool    active;
  uint8_t note;
} voices[NUM_VOICES];

int findFreeVoice() {
  for (int i = 0; i < NUM_VOICES; ++i)
    if (!voices[i].active) return i;
  return 0; // very simple steal
}

int findVoice(uint8_t note) {
  for (int i = 0; i < NUM_VOICES; ++i)
    if (voices[i].active && voices[i].note == note) return i;
  return -1;
}

void applyGlobalCCToAll(uint8_t cc, uint8_t val);

// -------- MIDI handling --------
void noteOn(uint8_t note, uint8_t vel) {
  int v = findFreeVoice();
  voices[v].active = true;
  voices[v].note   = note;

  float freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);
  osc[v].frequency(freq);
  osc[v].amplitude(0.8f * (vel / 127.0f));
  env[v].noteOn();

  // simple brightness keytrack example
  float semi = (note - 60) * 0.25f;
  formant[v].setBrightness(semi);
}

void noteOff(uint8_t note) {
  int v = findVoice(note);
  if (v >= 0) {
    env[v].noteOff();
    voices[v].active = false;
  }
}

void handleCC(uint8_t cc, uint8_t val) {
  applyGlobalCCToAll(cc, val);
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

// -------- CC → Filter mappings (global for all voices) --------
void applyGlobalCCToAll(uint8_t cc, uint8_t val) {
  float x = val / 127.0f;
  for (int i = 0; i < NUM_VOICES; ++i) {
    switch (cc) {
      case 21: { // Vowel
        formant[i].setVowel(x * 4.0f);
        break;
      }
      case 22: { // Brightness
        float semi = x * 48.0f - 24.0f;
        formant[i].setBrightness(semi);
        break;
      }
      case 23: { // Formant morph
        formant[i].setFormantMorph(x);
        break;
      }
      case 24: { // Q
        float q = 0.1f + x * 19.9f;
        formant[i].setQ(q);
        break;
      }
      case 25: { // Mix
        formant[i].setMix(x);
        break;
      }
      case 26: { // Breath
        formant[i].setBreath(x);
        break;
      }
      case 27: { // Consonant S
        formant[i].setConsonant(x, 0.0f, 0.0f);
        break;
      }
      case 28: { // Consonant F
        // keep previous S and N; only change F:
        formant[i].setConsonant(0.0f, x, 0.0f); // simple demo, adjust as needed
        break;
      }
      // second row
      case 70: { // Vowel Mod Depth
        formant[i].setVowelModDepth(x * 4.0f);
        break;
      }
      case 71: { // Brightness Mod Depth
        formant[i].setBrightnessModDepth(x * 24.0f);
        break;
      }
      case 72: { // Gain
        float g = 0.2f + x * 1.8f;
        formant[i].setGain(g);
        break;
      }
      case 73: { // Nasal
        formant[i].setConsonant(0.0f, 0.0f, x);
        break;
      }
      case 91: { // another gain trim (for experiments)
        float g = 0.5f + x * 1.5f;
        formant[i].setGain(g);
        break;
      }
      case 93: { // extra mix control
        formant[i].setMix(x);
        break;
      }
      case 82: // reserved / TODO
      case 83:
      default:
        break;
    }
  }
}

elapsedMillis cpuTimer;

void setup() {
  AudioMemory(80);
  Serial.begin(115200);
  Serial1.begin(31250);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // sgtl5000_1.enable();
  // sgtl5000_1.volume(0.5f);

  int k = 0;
  for (int i = 0; i < NUM_VOICES; ++i) {
    voices[i].active = false;
    voices[i].note   = 0;

    pc[k++] = new AudioConnection(osc[i], env[i]);
    pc[k++] = new AudioConnection(env[i], formant[i]);
    if (i < 2) pc[k++] = new AudioConnection(formant[i], 0, mixL, i);
    else       pc[k++] = new AudioConnection(formant[i], 0, mixR, i - 2);

    osc[i].begin(WAVEFORM_SAWTOOTH);
    osc[i].amplitude(0.0f);

    env[i].attack(5);
    env[i].decay(80);
    env[i].sustain(0.7f);
    env[i].release(120);

    formant[i].setMix(1.0f);
    formant[i].setGain(1.0f);
    formant[i].setVowel(1.0f);
    formant[i].setQ(7.0f);
    formant[i].setFormantMorph(0.3f);
    formant[i].setBrightness(0.0f);
    formant[i].setBreath(0.12f);
    formant[i].setConsonant(0.08f, 0.05f, 0.04f);
  }

  mixL.gain(0, 0.5f); mixL.gain(1, 0.5f);
  mixR.gain(0, 0.5f); mixR.gain(1, 0.5f);
}

void loop() {
  handleMidi();
  if (cpuTimer > 1000) {
    cpuTimer = 0;
    Serial.printf("Poly CPU: %.2f%% max %.2f%%, Mem: %u\n",
                  AudioProcessorUsage(), AudioProcessorUsageMax(),
                  AudioMemoryUsageMax());
  }
}
