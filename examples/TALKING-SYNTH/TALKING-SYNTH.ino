/*
  ---------------------------------------------------------------------------
  Teensy Formant Filter Demo
  ---------------------------------------------------------------------------
  Demonstrates 
  - Usage of AudioFilterFormant with MIDI Keyboard and controllers
  - Playable monophonic voice
  - Velocity changes vowel morphing (A–E–I–O–U)
  - Played key changes Brightness (vocal tract length) morph
  - Pitchbend, Modwheel + Aftertouch enabled  
  ---------------------------------------------------------------------------
*/

#include <Audio.h>
#include <AudioFilterFormant.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine lfo1;       //xy=90.000000,80.000000
AudioSynthWaveformDc dc1;          //xy=95.000000,35.000000
AudioSynthWaveformModulated osc2;  //xy=265.000000,95.000000
AudioSynthWaveformModulated osc1;  //xy=265.000000,55.000000
AudioMixer4 mixer1;                //xy=390.000000,95.000000
AudioFilterFormant formant1;       //xy=510.000000,55.000000
AudioEffectFreeverb freeverb1;     //xy=635.000000,110.000000
AudioMixer4 mixer2;                //xy=755.000000,90.000000
AudioOutputI2S i2s1;               //xy=910.000000,65.000000
AudioConnection patchCord1(lfo1, 0, osc2, 0);
AudioConnection patchCord2(dc1, 0, osc2, 1);
AudioConnection patchCord3(lfo1, 0, osc1, 0);
AudioConnection patchCord4(dc1, 0, osc1, 1);
AudioConnection patchCord5(osc1, 0, mixer1, 0);
AudioConnection patchCord6(osc2, 0, mixer1, 1);
AudioConnection patchCord7(mixer1, formant1);
AudioConnection patchCord8(formant1, freeverb1);
AudioConnection patchCord9(formant1, 0, mixer2, 0);
AudioConnection patchCord10(freeverb1, 0, mixer2, 1);
AudioConnection patchCord11(mixer2, 0, i2s1, 0);
AudioConnection patchCord12(mixer2, 0, i2s1, 1);
// GUItool: end automatically generated code

#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// change control change numbers here. These ones are predefined for usage with EMU XBoard
#define MIDI_CC_VOWEL 21
#define MIDI_CC_Q 22
#define MIDI_CC_GAIN 23
#define MIDI_CC_FORMANT_MORPH 24
#define MIDI_CC_BRIGHTNESS 25
#define MIDI_CC_OSC_WAVE 26
#define MIDI_CC_OSC_SHAPE 27
#define MIDI_CC_MIX 28
#define MIDI_CC_SUBOSC 70
#define MIDI_CC_SUBOSC_TUNE 71
#define MIDI_CC_REVERB 72

const float MIDI_PITCHBEND_SCALER_NEG = 1.0f / 8192.0f;
const float MIDI_PITCHBEND_SCALER_POS = 1.0f / 8191.0f;
const float MIDI_SCALER = 1.0f / 127.0f;

const uint8_t NUM_OSC_WAVES = 5;
const uint8_t OSC_WAVES[NUM_OSC_WAVES] = { WAVEFORM_SINE, WAVEFORM_TRIANGLE_VARIABLE, WAVEFORM_BANDLIMIT_SQUARE, WAVEFORM_BANDLIMIT_PULSE, WAVEFORM_BANDLIMIT_SAWTOOTH };
const char* OSC_WAVE_NAMES[NUM_OSC_WAVES] = { "SINE", "TRIANGLE (shape)", "SQUARE", "PULSE (shape)", "SAWTOOTH" };

uint8_t oscWave = 4;
uint8_t playedNote = 0;
uint32_t hwTimesTimer = millis();
float pitchBendSemitones = 0.0f;
float pitchBendFactor = 1.0f;
float modWheel = 0, modAT = 0;
float subOscLevel = 0;
float subOscTune = 0.5f;
float fxMix = 0.15f;

void onControlChange(byte channel, byte number, byte value);
void onPitchBend(byte channel, int value);
void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
void handleAfterTouch(byte channel, byte value);

float OSC_BASE_FREQ;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Start");

  Serial.println("Init Audio");
  AudioMemory(30);

  Serial.println("Init MIDI");
  MIDI.setHandleControlChange(onControlChange);
  MIDI.setHandlePitchBend(onPitchBend);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleAfterTouchChannel(handleAfterTouch);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  usbMIDI.setHandleControlChange(onControlChange);
  usbMIDI.setHandlePitchChange(onPitchBend);
  usbMIDI.setHandleNoteOn(handleNoteOn);
  usbMIDI.setHandleNoteOff(handleNoteOff);
  usbMIDI.setHandleAfterTouchChannel(handleAfterTouch);

  OSC_BASE_FREQ = noteToFreq(36);

  Serial.println("Init Audio modules");
  AudioNoInterrupts();
  formant1.setQ(15.0f);
  lfo1.frequency(5.0f);
  lfo1.amplitude(0.0f);
  dc1.amplitude(0.0f);
  osc1.begin(0.0f, OSC_BASE_FREQ, OSC_WAVES[oscWave]);
  osc2.begin(0.0f, OSC_BASE_FREQ, OSC_WAVES[oscWave]);

  mixer2.gain(0, 1 - fxMix);
  mixer2.gain(1, fxMix);

  AudioInterrupts();
}

void loop() {
  MIDI.read();
  usbMIDI.read();
  if (millis() - hwTimesTimer > 5000) {
    hwTimesTimer = millis();
    Serial.println("-----------------------------------------------------");
    Serial.print("Audio CPU: ");
    Serial.print(AudioProcessorUsageMax());
    Serial.print("% Audio MEM: ");
    Serial.print(AudioMemoryUsageMax());
    Serial.println();
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
  }
}
void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if (playedNote == pitch) {
    Serial.println("Note off");
    osc1.amplitude(0.0f);
    osc2.amplitude(0.0f);
    playedNote = 0;
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  Serial.printf("Note on %d\n", pitch);

  playedNote = pitch;

  float semis = pitch - 48;

  float vowel = (float)(uint8_t)velocity * MIDI_SCALER;
  OSC_BASE_FREQ = noteToFreq(pitch);
  float totalFreq = OSC_BASE_FREQ * pitchBendFactor;

  AudioNoInterrupts();
  osc1.frequency(totalFreq);
  osc2.frequency(totalFreq * subOscTune);
  osc1.amplitude(1);
  osc2.amplitude(subOscLevel);
  formant1.setVowel(vowel * vowel * 4.0f);
  formant1.setBrightness(semis);
  AudioInterrupts();
}

void onPitchBend(byte channel, int value) {
  float v;
  if (value == 0) {
    v = 0;
  } else if (value < 0) {
    v = value * MIDI_PITCHBEND_SCALER_NEG;
  } else {
    v = value * MIDI_PITCHBEND_SCALER_POS;
  }

  // Calculate semitone offset
  float pitchBendAmt = 2.0f;
  pitchBendSemitones = v * pitchBendAmt;

  // Convert semitones to frequency multiplier
  pitchBendFactor = pow(2.0f, pitchBendSemitones / 12.0f);

  float totalFreq = OSC_BASE_FREQ * pitchBendFactor;

  AudioNoInterrupts();
  // formant1.setBrightness(pitchBendSemitones);
  osc1.frequency(totalFreq);
  osc2.frequency(totalFreq * subOscTune);
  AudioInterrupts();
}

void handleAfterTouch(byte channel, byte value) {
  float fvalue = (float)(uint8_t)value * MIDI_SCALER;
  fvalue *= 0.01f;
  modAT = fvalue;
  lfo1.amplitude(modWheel + modAT);
}

void onControlChange(byte channel, byte number, byte value) {
  float fvalue = (float)(uint8_t)value * MIDI_SCALER;
  bool waveChanged = false;

  AudioNoInterrupts();

  switch (number) {
    case 1:  // modulation wheel
      fvalue *= 0.01f;
      modWheel = fvalue;
      lfo1.amplitude(modWheel + modAT);
      break;
    case MIDI_CC_VOWEL:
      fvalue *= 4.0f;
      formant1.setVowel(fvalue);
      break;
    case MIDI_CC_Q:
      fvalue *= 30.0f + 1.0f;
      formant1.setQ(fvalue);
      break;
    case MIDI_CC_GAIN:
      formant1.setGain(fvalue);
      break;
    case MIDI_CC_FORMANT_MORPH:
      formant1.setFormantMorph(fvalue);
      break;
    case MIDI_CC_BRIGHTNESS:
      fvalue = fvalue * 48.0f - 24.0f;
      formant1.setBrightness(fvalue);
      break;
    case MIDI_CC_OSC_WAVE:
      fvalue *= (NUM_OSC_WAVES - 1);
      if (round(fvalue) != oscWave) {
        waveChanged = true;
        oscWave = round(fvalue);
        if (oscWave >= NUM_OSC_WAVES) {
          oscWave = NUM_OSC_WAVES - 1;
        }
        osc1.begin(OSC_WAVES[oscWave]);
        osc2.begin(OSC_WAVES[oscWave]);
      }
      break;
    case MIDI_CC_OSC_SHAPE:
      dc1.amplitude(fvalue);
      break;
    case MIDI_CC_MIX:
      formant1.setMix(fvalue);
      break;
    case MIDI_CC_SUBOSC:
      subOscLevel = fvalue * 0.9f;
      if (playedNote != 0) {
        osc2.amplitude(subOscLevel);
      }
      break;
    case MIDI_CC_SUBOSC_TUNE:
      subOscTune = 0.5f + fvalue * fvalue * 0.01f;
      osc2.frequency(OSC_BASE_FREQ * pitchBendFactor * subOscTune);
      break;
    case MIDI_CC_REVERB:
      fxMix = fvalue;
      mixer2.gain(0, 1 - fxMix);
      mixer2.gain(1, fxMix);
      break;
    default:
      break;
  }

  AudioInterrupts();

  switch (number) {
    case 1:
      Serial.printf("modwheel: %f\n", fvalue);
      break;
    case MIDI_CC_VOWEL:
      Serial.printf("vowel: %f\n", fvalue);
      break;
    case MIDI_CC_Q:
      Serial.printf("q: %f\n", fvalue);
      break;
    case MIDI_CC_GAIN:
      Serial.printf("gain: %f\n", fvalue);
      break;
    case MIDI_CC_FORMANT_MORPH:
      Serial.printf("formantMorph: %f\n", fvalue);
      break;
    case MIDI_CC_BRIGHTNESS:
      Serial.printf("Brightness: %f\n", fvalue);
      break;
    case MIDI_CC_OSC_WAVE:
      if (waveChanged) {
        Serial.printf("Osc Wave: %s\n", OSC_WAVE_NAMES[oscWave]);
      }
      break;
    case MIDI_CC_OSC_SHAPE:
      Serial.printf("Osc Shape: %f\n", fvalue);
      break;
    case MIDI_CC_MIX:
      Serial.printf("Formant Mix: %f\n", fvalue);
      break;
    case MIDI_CC_SUBOSC:
      Serial.printf("Sub Osc Level: %f\n", fvalue);
      break;
    case MIDI_CC_SUBOSC_TUNE:
      Serial.printf("Sub Osc Tune: %f\n", subOscTune);
      break;
    case MIDI_CC_REVERB:
      Serial.printf("Reverb Mix: %f\n", fxMix);
      break;
    default:
      Serial.printf("UNUSED MIDI CC#%d: %d \n", number, value);
      break;
  }
}



// get the frequency from a MIDI note number
static float noteToFreq(int note) {
  float a = 440;  //frequency of A (common value is 440Hz)
  return (a / 32) * pow(2, ((note - 9) / 12.0));
}