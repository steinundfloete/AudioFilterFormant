#include "AudioFilterFormant.h"
#include <math.h>

// ---------------------------------------------------------------------------
//  Formant frequency tables (Hz): Male / Female / Child
// ---------------------------------------------------------------------------
static const float maleFormants[5][3] = {
  {800,1150,2900},{400,2000,2600},{350,2200,2900},{450,800,2830},{325,700,2700}
};
static const float femaleFormants[5][3] = {
  {1000,1700,3200},{500,2500,3100},{400,2900,3350},{600,900,3200},{350,800,3000}
};
static const float childFormants[5][3] = {
  {1200,2300,3500},{700,3000,3400},{500,3500,3700},{700,1200,3300},{400,1100,3200}
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AudioFilterFormant::AudioFilterFormant()
: AudioStream(3, inputQueueArray)
{
  vowel          = targetVowel = 0.0f;
  qFactor        = targetQ     = 5.0f;
  gain           = 1.0f;
  compGain       = 1.0f;
  brightness     = 0.0f;
  formantSetMorph= 0.0f;
  mix            = 1.0f;

  pitchScale     = 1.0f;
  procVowel      = 0.0f;
  procBrightness = 0.0f;

  vowelModDepth      = 0.0f;
  brightnessModDepth = 0.0f;

  breath       = 0.0f;
  consonantS   = 0.0f;
  consonantF   = 0.0f;
  consonantN   = 0.0f;
  bypass       = false;

  noiseSeed    = 22222u;
  sState       = 0.0f;
  fState       = 0.0f;
  nasalState   = 0.0f;

  f1 = f2 = f3 = 0.0f;

  // Initialize coefficients to zero
  memset(&bq1, 0, sizeof(Biquad));
  memset(&bq2, 0, sizeof(Biquad));
  memset(&bq3, 0, sizeof(Biquad));

  calcFormants();
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void AudioFilterFormant::setVowel(float v){
  if(v < 0.0f) v = 0.0f;
  if(v > 4.0f) v = 4.0f;
  targetVowel = v;
}

void AudioFilterFormant::setQ(float q){
  if(q < 0.1f)  q = 0.1f;
  if(q > 20.0f) q = 20.0f;
  targetQ = q;
}

void AudioFilterFormant::setFormantMorph(float m){
  if(m < 0.0f) m = 0.0f;
  if(m > 1.0f) m = 1.0f;
  formantSetMorph = m;
  // Formants will be recalculated in update() based on procVowel / brightness
}

void AudioFilterFormant::setBrightness(float semi){
  if(semi < -24.0f) semi = -24.0f;
  if(semi >  24.0f) semi =  24.0f;
  brightness = semi;
  // procBrightness & pitchScale will be updated in update()
}

void AudioFilterFormant::setMix(float m){
  if(m < 0.0f) m = 0.0f;
  if(m > 1.0f) m = 1.0f;
  mix = m;
}

// ---------------------------------------------------------------------------
// Noise generator: very fast LCG, returns approx -1..+1
// ---------------------------------------------------------------------------
inline float AudioFilterFormant::randNoise() {
  noiseSeed = noiseSeed * 1664525u + 1013904223u;
  // Use upper bits for better distribution
  int32_t x = (int32_t)(noiseSeed >> 9);
  return (float)x * (1.0f / 16777216.0f); // ~[-1, 1]
}

// ---------------------------------------------------------------------------
// Bandpass coefficient calculation (RBJ type 2 bandpass)
// ---------------------------------------------------------------------------
void AudioFilterFormant::calcBandpass(Biquad &bq,float freq){
  float fs = AUDIO_SAMPLE_RATE_EXACT;
  float omega = 2.0f * PI * freq / fs;
  float sn = sinf(omega);
  float cs = cosf(omega);
  float alpha = sn / (2.0f * qFactor);

  float b0 =  alpha;
  float b1 =  0.0f;
  float b2 = -alpha;
  float a0 =  1.0f + alpha;
  float a1 = -2.0f * cs;
  float a2 =  1.0f - alpha;

  bq.b0t = b0 / a0;
  bq.b1t = b1 / a0;
  bq.b2t = b2 / a0;
  bq.a1t = a1 / a0;
  bq.a2t = a2 / a0;
}

// ---------------------------------------------------------------------------
// Smooth base vowel & Q (not including audio-rate modulation)
// ---------------------------------------------------------------------------
void AudioFilterFormant::smoothParams(){
  const float alphaV = 0.25f;
  const float alphaQ = 0.30f;

  vowel   += alphaV * (targetVowel - vowel);
  qFactor += alphaQ * (targetQ     - qFactor);

  // Q loudness compensation (normalize around Q = 5)
  compGain = sqrtf(qFactor / 5.0f);
}

// ---------------------------------------------------------------------------
// Calculate interpolated formant frequencies using procVowel & pitchScale
// ---------------------------------------------------------------------------
void AudioFilterFormant::calcFormants(){
  // Clamp effective vowel to 0..4
  float v = procVowel;
  if (v < 0.0f) v = 0.0f;
  if (v > 4.0f) v = 4.0f;

  int idx = (int)v;
  float frac = v - idx;
  if(idx >= 4) { idx = 3; frac = 1.0f; }

  // Interpolate between male→female→child sets based on formantSetMorph
  float m = formantSetMorph;
  float wMale, wFemale, wChild;
  if(m < 0.5f){          // male → female
    wMale   = 1.0f - m*2.0f;
    wFemale =        m*2.0f;
    wChild  = 0.0f;
  }else{                 // female → child
    wMale   = 0.0f;
    wFemale = 1.0f - (m-0.5f)*2.0f;
    wChild  =        (m-0.5f)*2.0f;
  }

  auto blend=[&](int i,int j){
    return maleFormants[i][j]*wMale
         + femaleFormants[i][j]*wFemale
         + childFormants[i][j]*wChild;
  };

  float f1a = blend(idx,0);
  float f2a = blend(idx,1);
  float f3a = blend(idx,2);
  float f1b = blend(idx+1,0);
  float f2b = blend(idx+1,1);
  float f3b = blend(idx+1,2);

  f1 = f1a + (f1b - f1a) * frac;
  f2 = f2a + (f2b - f2a) * frac;
  f3 = f3a + (f3b - f3a) * frac;

  // Apply brightness scaling (exponential in semitones)
  f1 *= pitchScale;
  f2 *= pitchScale;
  f3 *= pitchScale;

  calcBandpass(bq1,f1);
  calcBandpass(bq2,f2);
  calcBandpass(bq3,f3);
}

// ---------------------------------------------------------------------------
// Morph coefficients smoothly
// ---------------------------------------------------------------------------
void AudioFilterFormant::morphBiquadCoeffs(Biquad &bq){
  const float alpha = 0.15f;
  bq.b0 += alpha * (bq.b0t - bq.b0);
  bq.b1 += alpha * (bq.b1t - bq.b1);
  bq.b2 += alpha * (bq.b2t - bq.b2);
  bq.a1 += alpha * (bq.a1t - bq.a1);
  bq.a2 += alpha * (bq.a2t - bq.a2);
}

// ---------------------------------------------------------------------------
// Biquad sample processing (Transposed Direct Form II)
// ---------------------------------------------------------------------------
inline float AudioFilterFormant::processBiquad(Biquad &bq,float x){
  float y = bq.b0 * x + bq.z1;
  bq.z1 = bq.b1 * x - bq.a1 * y + bq.z2;
  bq.z2 = bq.b2 * x - bq.a2 * y;
  return y;
}

// ---------------------------------------------------------------------------
// Main audio update
// ---------------------------------------------------------------------------
void AudioFilterFormant::update(void){
  // Receive inputs: audio, vowel mod, brightness mod
  audio_block_t *in  = receiveReadOnly(0);
  audio_block_t *mV  = receiveReadOnly(1);
  audio_block_t *mBr = receiveReadOnly(2);

  if (!in) {
    if (mV)  release(mV);
    if (mBr) release(mBr);
    return;
  }

  // Fast bypass / dry-only path
  if (bypass || mix <= 0.00001f) {
    audio_block_t *out = allocate();
    if (!out) {
      release(in);
      if (mV)  release(mV);
      if (mBr) release(mBr);
      return;
    }
    memcpy(out->data, in->data, sizeof(out->data));
    transmit(out);
    release(out);
    release(in);
    if (mV)  release(mV);
    if (mBr) release(mBr);
    return;
  }

  // --- Block-averaged modulation (-1..+1) ---
  float mv = 0.0f;
  float mb = 0.0f;

  if (mV) {
    int32_t acc = 0;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) acc += mV->data[i];
    mv = (float)acc / (32768.0f * AUDIO_BLOCK_SAMPLES);
  }
  if (mBr) {
    int32_t acc = 0;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) acc += mBr->data[i];
    mb = (float)acc / (32768.0f * AUDIO_BLOCK_SAMPLES);
  }

  // --- Smooth base parameters (vowel & Q) ---
  smoothParams();

  // --- Effective (base + mod) vowel 0..4 ---
  procVowel = vowel + mv * vowelModDepth;
  if (procVowel < 0.0f) procVowel = 0.0f;
  if (procVowel > 4.0f) procVowel = 4.0f;

  // --- Effective (base + mod) brightness in semitones, smoothed ---
  float targetBr = brightness + mb * brightnessModDepth;
  if (targetBr < -24.0f) targetBr = -24.0f;
  if (targetBr >  24.0f) targetBr =  24.0f;

  // fixed-good smoothing factor for brightness (fast, but no zipper)
  const float alphaBr = 0.6f;
  procBrightness += alphaBr * (targetBr - procBrightness);

  // derive exponential pitch scaling from smoothed brightness
  pitchScale = powf(2.0f, procBrightness / 12.0f);

  // Recalculate formants for this block
  calcFormants();
  morphBiquadCoeffs(bq1);
  morphBiquadCoeffs(bq2);
  morphBiquadCoeffs(bq3);

  // Allocate output
  audio_block_t *out = allocate();
  if (!out) {
    release(in);
    if (mV)  release(mV);
    if (mBr) release(mBr);
    return;
  }

  // --- Main DSP loop ---
  for(int i = 0; i < AUDIO_BLOCK_SAMPLES; i++){
    float x = (float)in->data[i] / 32768.0f;

    // Breath & consonant excitation
    float n = randNoise();

    if (breath > 0.0001f) {
      x += n * breath * 0.25f;
    }

    if (consonantS > 0.0001f) {
      // Simple highpass-ish sibilance
      sState += 0.2f * (n - sState);
      float sHP = n - sState;
      x += sHP * consonantS * 0.20f;
    }

    if (consonantF > 0.0001f) {
      // Simple midband-ish fricative
      fState += 0.05f * (n - fState);
      x += fState * consonantF * 0.15f;
    }

    if (consonantN > 0.0001f) {
      // Very simple nasal-ish low-mid emphasis
      nasalState += 0.02f * ((n * 0.5f + x * 0.5f) - nasalState);
      x += nasalState * consonantN * 0.20f;
    }

    // Process through 3 parallel formant bandpasses
    float y1 = processBiquad(bq1,x);
    float y2 = processBiquad(bq2,x);
    float y3 = processBiquad(bq3,x);

    float wet = (y1 + y2 + y3) * (gain * compGain * 5.0f / 3.0f);

    // Dry/wet mix
    float y = x * (1.0f - mix) + wet * mix;

    if (y >  1.0f) y =  1.0f;
    if (y < -1.0f) y = -1.0f;

    out->data[i] = (int16_t)(y * 32767.0f);
  }

  transmit(out);
  release(out);
  release(in);
  if (mV)  release(mV);
  if (mBr) release(mBr);
}
