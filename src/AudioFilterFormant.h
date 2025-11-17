#ifndef AudioFilterFormant_h_
#define AudioFilterFormant_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <arm_math.h>

/*
  ---------------------------------------------------------------------------
   AudioFilterFormant
  ---------------------------------------------------------------------------
   Author: ChatGPT 2025 – Optimized for Teensy 4.x + Audio Library

   Description:
     A smooth, morphable 3-band formant filter simulating human vowel
     resonances, designed for use with the Teensy Audio Library.

   Features:
     • Continuous vowel morphing (A → E → I → O → U)
     • Continuous morph between voice types (male → female → child)
     • Resonance (Q) with RMS loudness compensation
     • Brightness (vocal tract length) control in semitones
     • Smooth coefficient morphing (no zipper noise)
     • Dry/Wet mix control
     • Audio-rate modulation inputs for vowel and brightness
     • Breath & consonant noise excitation (S, F, nasal-ish color)
     • Bypass mode with low-CPU passthrough

   Typical use:
     AudioFilterFormant formant;
     formant.setVowel(2.3f);        // between I and O
     formant.setQ(8.0f);            // resonance
     formant.setFormantMorph(0.4f); // between male/female/child
     formant.setBrightness(6.0f);   // brighter voice (+6 semitones)
     formant.setGain(1.0f);
     formant.setMix(1.0f);          // fully wet
  ---------------------------------------------------------------------------
  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.    
*/

class AudioFilterFormant : public AudioStream {
public:
  AudioFilterFormant();

  // 3 inputs: 0=audio in, 1=vowel mod, 2=brightness mod
  virtual void update(void);

  /** Set vowel position (0.0 = A … 4.0 = U, fractional = morph) */
  void setVowel(float v);

  /** Set resonance (Q-factor, typical 0.1 – 20.0) */
  void setQ(float q);

  /** Overall gain multiplier (default = 1.0) */
  void setGain(float g) { gain = g; }

  /** Continuous morph between voice models:
      0.0 = male → 0.5 = female → 1.0 = child */
  void setFormantMorph(float m);

  /** Global brightness / vocal-tract length offset in semitones
      (−24 … +24, default = 0).  Positive = brighter / shorter tract. */
  void setBrightness(float semi);

  /** Dry/wet mix balance.
      0.0 = 100% dry (original input)
      1.0 = 100% wet (formant filter only)
      default = 1.0 */
  void setMix(float m);

  /** Modulation depths:
      vowel depth in vowel units (0..4), brightness in semitones. */
  void setVowelModDepth(float d)      { vowelModDepth = d; }
  void setBrightnessModDepth(float d) { brightnessModDepth = d; }

  /** Additional breath / turbulence mix (0..1) */
  void setBreath(float level) { breath = level; }

  /** Consonant noise levels:
      sLevel  = sibilance  ("S")
      fLevel  = fricative  ("F")
      nasal   = nasal-like coloration */
  void setConsonant(float sLevel, float fLevel, float nasal) {
    consonantS = sLevel;
    consonantF = fLevel;
    consonantN = nasal;
  }

  /** Bypass processing: when true, or when mix == 0, the input is passed
      through unprocessed with minimal CPU load. */
  void setBypass(bool b) { bypass = b; }

private:
  audio_block_t *inputQueueArray[3];

  struct Biquad {
    float b0, b1, b2, a1, a2;        // current coefficients
    float b0t, b1t, b2t, a1t, a2t;   // target coefficients
    float z1, z2;                    // state memory
  };

  // Base parameters (setters write these)
  float vowel, targetVowel;     // 0..4
  float qFactor, targetQ;       // resonance
  float gain;
  float brightness;             // semitones (-24..+24)
  float formantSetMorph;        // 0..1 (male→female→child)
  float mix;                    // 0..1 dry/wet

  // Derived & smoothed values
  float compGain;               // RMS compensation vs Q
  float pitchScale;             // formant scaling factor
  float procVowel;              // effective vowel (base + mod)
  float procBrightness;         // effective brightness (base + mod, smoothed)

  // Modulation depths
  float vowelModDepth;          // in vowel units
  float brightnessModDepth;     // in semitones

  // Breath & consonants
  float breath;                 // 0..1
  float consonantS;             // "S" sibilance
  float consonantF;             // "F" fricative mid-noise
  float consonantN;             // nasal-ish color

  bool bypass;

  // Noise generator & consonant states
  uint32_t noiseSeed;
  float sState;
  float fState;
  float nasalState;

  // Formant frequencies
  float f1, f2, f3;
  Biquad bq1, bq2, bq3;

  // Internal helpers
  void calcFormants();
  void calcBandpass(Biquad &bq, float freq);
  inline float processBiquad(Biquad &bq, float x);
  void morphBiquadCoeffs(Biquad &bq);
  void smoothParams();
  inline float randNoise();
};

#endif
