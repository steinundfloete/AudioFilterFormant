/*
  ---------------------------------------------------------------------------
  Teensy Formant Filter Demo
  ---------------------------------------------------------------------------
  Demonstrates 
  - Usage of AudioFilterFormant with MIDI controllers
  - Creates a chord with bass and sends it through AudioFilterFormant 
  - Continuous vowel morphing (A–E–I–O–U)
  - Resonance (Q) modulation
  - Brightness (vocal tract length) morph
  - Voice type morph (male -> female -> child)
  - Use MIDI keys to change chord (see handleNoteOn for examples)
  - Use Pitchbend  
  ---------------------------------------------------------------------------
*/

#include <Audio.h>
#include <AudioFilterFormant.h>

// GUItool: begin automatically generated code
AudioSynthWaveformSine           lfo1;                //xy=670.000000,450.000000
AudioSynthWaveformDc             dc1;                 //xy=675.000000,405.000000
AudioSynthWaveformModulated      osc4;                //xy=845.000000,538.000000
AudioSynthWaveformModulated      osc3;                //xy=845.000000,502.000000
AudioSynthWaveformModulated      osc2;                //xy=845.000000,466.000000
AudioSynthWaveformModulated      osc1;                //xy=845.000000,425.000000
AudioMixer4                      mixer1;              //xy=995.000000,540.000000
AudioAnalyzeRMS                  rms2;                //xy=1185.000000,575.000000
AudioFilterFormant               formant1;            //xy=1190.000000,530.000000
AudioAnalyzeRMS                  rms1;                //xy=1305.000000,560.000000
AudioOutputI2S                   i2s1;                //xy=1350.000000,525.000000
AudioConnection                  patchCord1(lfo1, 0, osc4, 0);
AudioConnection                  patchCord2(dc1, 0, osc4, 1);
AudioConnection                  patchCord3(lfo1, 0, osc3, 0);
AudioConnection                  patchCord4(dc1, 0, osc3, 1);
AudioConnection                  patchCord5(lfo1, 0, osc2, 0);
AudioConnection                  patchCord6(dc1, 0, osc2, 1);
AudioConnection                  patchCord7(lfo1, 0, osc1, 0);
AudioConnection                  patchCord8(dc1, 0, osc1, 1);
AudioConnection                  patchCord9(osc1, 0, mixer1, 0);
AudioConnection                  patchCord10(osc2, 0, mixer1, 1);
AudioConnection                  patchCord11(osc3, 0, mixer1, 2);
AudioConnection                  patchCord12(osc4, 0, mixer1, 3);
AudioConnection                  patchCord13(mixer1, rms2);
AudioConnection                  patchCord14(mixer1, formant1);
AudioConnection                  patchCord15(formant1, rms1);
AudioConnection                  patchCord16(formant1, 0, i2s1, 0);
AudioConnection                  patchCord17(formant1, 0, i2s1, 1);
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

const float MIDI_PITCHBEND_SCALER_NEG = 1.0f / 8192.0f;
const float MIDI_PITCHBEND_SCALER_POS = 1.0f / 8191.0f;
const float MIDI_SCALER = 1.0f / 127.0f;

const uint8_t NUM_OSC_WAVES = 5;
const uint8_t OSC_WAVES[NUM_OSC_WAVES] = { WAVEFORM_SINE, WAVEFORM_TRIANGLE_VARIABLE, WAVEFORM_BANDLIMIT_SQUARE, WAVEFORM_BANDLIMIT_PULSE, WAVEFORM_BANDLIMIT_SAWTOOTH };
const char* OSC_WAVE_NAMES[NUM_OSC_WAVES] = { "SINE", "TRIANGLE (shape)", "SQUARE", "PULSE (shape)", "SAWTOOTH" };

uint8_t oscWave = 4;
uint32_t hwTimesTimer = millis();
float pitchBendSemitones = 0.0f;
float pitchBendFactor = 1.0f;

void onControlChange(byte channel, byte number, byte value);
void onPitchBend(byte channel, int value);
void handleNoteOn(byte channel, byte pitch, byte velocity);

float OSC_BASE_FREQ[4];

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
  MIDI.begin(MIDI_CHANNEL_OMNI);

  usbMIDI.setHandleControlChange(onControlChange);
  usbMIDI.setHandlePitchChange(onPitchBend);
  usbMIDI.setHandleNoteOn(handleNoteOn);

  OSC_BASE_FREQ[0] = noteToFreq(36);
  OSC_BASE_FREQ[1] = noteToFreq(48);
  OSC_BASE_FREQ[2] = noteToFreq(51);
  OSC_BASE_FREQ[3] = noteToFreq(55);

  Serial.println("Init Audio modules");
  AudioNoInterrupts();
  formant1.setQ(15.0f);
  lfo1.frequency(5.0f);
  lfo1.amplitude(0.0f);
  dc1.amplitude(0.0f);
  osc1.begin(0.2f, OSC_BASE_FREQ[0], WAVEFORM_BANDLIMIT_SAWTOOTH);
  osc2.begin(0.2f, OSC_BASE_FREQ[1], WAVEFORM_BANDLIMIT_SAWTOOTH);
  osc3.begin(0.2f, OSC_BASE_FREQ[2], WAVEFORM_BANDLIMIT_SAWTOOTH);
  osc4.begin(0.2f, OSC_BASE_FREQ[3], WAVEFORM_BANDLIMIT_SAWTOOTH);

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

    if (rms2.available()) {
      Serial.printf("RMS IN:  %f\n", rms2.read());
    }
    if (rms1.available()) {
      Serial.printf("RMS OUT: %f\n", rms1.read());
    }
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  uint8_t note = pitch % 12;
  Serial.printf("Received note %d\n", note);
  switch (note) {
    case 0:
      OSC_BASE_FREQ[1] = noteToFreq(48);
      break;
    case 1:
      break;
    case 2:
      OSC_BASE_FREQ[2] = noteToFreq(50);
      break;
    case 3:
      OSC_BASE_FREQ[2] = noteToFreq(51);
      break;
    case 4:
      OSC_BASE_FREQ[2] = noteToFreq(52);
      break;
    case 5:
      OSC_BASE_FREQ[2] = noteToFreq(53);
      break;
    case 6:
      break;
    case 7:
      break;
    case 8:
      break;
    case 9:
      break;
    case 10:
      OSC_BASE_FREQ[1] = noteToFreq(46);
      break;
    case 11:
      OSC_BASE_FREQ[1] = noteToFreq(47);
      break;
    default:
      break;
  }
  AudioNoInterrupts();
  osc1.frequency(OSC_BASE_FREQ[0] * pitchBendFactor);
  osc2.frequency(OSC_BASE_FREQ[1] * pitchBendFactor);
  osc3.frequency(OSC_BASE_FREQ[2] * pitchBendFactor);
  osc4.frequency(OSC_BASE_FREQ[3] * pitchBendFactor);
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
  float pitchBendAmt = 24.0f;
  pitchBendSemitones = v * pitchBendAmt;

  // Convert semitones to frequency multiplier
  pitchBendFactor = pow(2.0f, pitchBendSemitones / 12.0f);

  AudioNoInterrupts();
  formant1.setBrightness(pitchBendSemitones);
  osc1.frequency(OSC_BASE_FREQ[0] * pitchBendFactor);
  osc2.frequency(OSC_BASE_FREQ[1] * pitchBendFactor);
  osc3.frequency(OSC_BASE_FREQ[2] * pitchBendFactor);
  osc4.frequency(OSC_BASE_FREQ[3] * pitchBendFactor);
  AudioInterrupts();
}

void onControlChange(byte channel, byte number, byte value) {
  float fvalue = (float)(uint8_t)value * MIDI_SCALER;
  bool waveChanged = false;

  AudioNoInterrupts();

  switch (number) {
    case 1:  // modulation wheel
      fvalue *= 0.01f;
      lfo1.amplitude(fvalue);
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
        if(oscWave >= NUM_OSC_WAVES) {
          oscWave = NUM_OSC_WAVES-1;
        }
        osc1.begin(OSC_WAVES[oscWave]);
        osc2.begin(OSC_WAVES[oscWave]);
        osc3.begin(OSC_WAVES[oscWave]);
        osc4.begin(OSC_WAVES[oscWave]);
      }
      break;
    case MIDI_CC_OSC_SHAPE:
      dc1.amplitude(fvalue);
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