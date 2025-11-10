#ifndef _audio_filter_formant_h_
#define _audio_filter_formant_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <arm_math.h>

/*
  ---------------------------------------------------------------------------
   Teensy Formant Filter  (AudioFilterFormant)
  ---------------------------------------------------------------------------
   Copyright (c) 2025 by Uli Schmidt steinundfloete@online.de
   V1.0 11/10/2025 
  ---------------------------------------------------------------------------
   Description:
     A smooth, morphable 3-band formant filter simulating vowel resonances.

   Features:
     • Continuous morphing between vowels  (A -> E -> I -> O -> U)
     • Continuous morphing between voice types (male -> female -> child)
     • Resonance (Q) with RMS loudness compensation
     • Smooth coefficient morphing -> zero zipper noise
     • Brightness control (+/- 24 semitones vocal-tract length)
     • Lightweight (~ 3 % CPU on Teensy 4.1)

   Typical use:
     AudioFilterFormant formant;
     formant.setVowel(2.3f);        // between I and O
     formant.setQ(8.0f);            // resonance
     formant.setFormantMorph(0.4f); // between male/female/child
     formant.setBrightness(+6.0f);  // brighter voice
     formant.setGain(1.0f);
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
  virtual void update(void);

  /** Set vowel position (0.0 = A … 4.0 = U, fractional = morph) */
  void setVowel(float v);

  /** Set resonance (Q-factor, typical 0.1 – 20.0) */
  void setQ(float q);

  /** Overall gain multiplier (default = 1.0) */
  void setGain(float g) { gain = g; }

  /** Continuous morph between voice models:
      0.0 = male -> 0.5 = female -> 1.0 = child */
  void setFormantMorph(float m);

  /** Global brightness / vocal-tract length offset in semitones
      (−24 … +24, default = 0).  Positive = brighter / shorter tract. */
  void setBrightness(float semi);

private:
  audio_block_t *inputQueueArray[1];

  struct Biquad {
    float b0,b1,b2,a1,a2;        // current coefficients
    float b0t,b1t,b2t,a1t,a2t;   // target coefficients
    float z1,z2;                 // state memory
  };

  float vowel, targetVowel;
  float qFactor, targetQ;
  float gain;
  float compGain;
  float formantSetMorph;   // 0…1 voice type morph
  float brightness;        // semitone offset
  float pitchScale;        // derived frequency scale
  float f1,f2,f3;
  Biquad bq1,bq2,bq3;

  void calcFormants();
  void calcBandpass(Biquad &bq, float freq);
  inline float processBiquad(Biquad &bq, float x);
  void morphBiquadCoeffs(Biquad &bq);
  void smoothParams();
};

#endif // #ifndef _audio_filter_formant_h_
