#include <Arduino.h>
#include "AudioFilterFormant.h"
#include <math.h>

// ---------------------------------------------------------------------------
//  Formant frequency tables (Hz): Male / Female / Child
// ---------------------------------------------------------------------------
static const float male[5][3] = {
  {800,1150,2900},{400,2000,2600},{350,2200,2900},{450,800,2830},{325,700,2700}
};
static const float female[5][3] = {
  {1000,1700,3200},{500,2500,3100},{400,2900,3350},{600,900,3200},{350,800,3000}
};
static const float child[5][3] = {
  {1200,2300,3500},{700,3000,3400},{500,3500,3700},{700,1200,3300},{400,1100,3200}
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AudioFilterFormant::AudioFilterFormant()
: AudioStream(1,inputQueueArray)
{
  vowel = targetVowel = 0.0f;
  qFactor = targetQ = 8.0f;
  gain = 1.0f;
  compGain = 1.0f;
  formantSetMorph = 0.0f;
  brightness = 0.0f;
  pitchScale = 1.0f;
  mix = 1.0f; // default = full effect
  calcFormants();
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void AudioFilterFormant::setVowel(float v){
  if(v<0)v=0; 
  else if(v>4)v=4;
  targetVowel = v;
}

void AudioFilterFormant::setQ(float q){
  if(q<0.1f)q=0.1f; 
  else if(q>30.0f)q=30.0f;
  targetQ = q;
}

void AudioFilterFormant::setFormantMorph(float m){
  if(m<0)m=0; 
  else if(m>1)m=1;
  formantSetMorph = m;
  calcFormants();
}

void AudioFilterFormant::setBrightness(float semi){
  if(semi<-24.0f) semi = -24.0f;
  if(semi> 24.0f) semi =  24.0f;
  brightness = semi;
  pitchScale = powf(2.0f, brightness / 12.0f);
  calcFormants();
}

void AudioFilterFormant::setMix(float m) {
  if (m < 0.0f) m = 0.0f;
  if (m > 1.0f) m = 1.0f;
  mix = m;
}

// ---------------------------------------------------------------------------
// Bandpass coefficient calculation (RBJ type 2)
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
// Smooth parameter changes (no zipper noise)
// ---------------------------------------------------------------------------
void AudioFilterFormant::smoothParams(){
  const float alphaV = 0.25f;
  const float alphaQ = 0.3f;

  vowel   += alphaV * (targetVowel - vowel);
  qFactor += alphaQ * (targetQ - qFactor);

  // Q loudness compensation (normalize around Q = 5)
  compGain = sqrtf(qFactor / 2.0f);

  static float lastV = -1, lastQ = -1;
  if(fabsf(vowel-lastV)>0.005f || fabsf(qFactor-lastQ)>0.02f){
    calcFormants();
    lastV = vowel; lastQ = qFactor;
  }
}

// ---------------------------------------------------------------------------
// Calculate interpolated formant frequencies
// ---------------------------------------------------------------------------
void AudioFilterFormant::calcFormants(){
  int idx = (int)vowel;
  float frac = vowel - idx;
  if(idx>=4){ idx=3; frac=1.0f; }

  // Interpolate between male→female→child sets
  float m = formantSetMorph;
  float wMale,wFemale,wChild;
  if(m<0.5f){          // male → female
    wMale   = 1.0f - m*2.0f;
    wFemale = m*2.0f;
    wChild  = 0.0f;
  }else{               // female → child
    wMale   = 0.0f;
    wFemale = 1.0f - (m-0.5f)*2.0f;
    wChild  = (m-0.5f)*2.0f;
  }

  auto blend=[&](int i,int j){
    return male[i][j]*wMale + female[i][j]*wFemale + child[i][j]*wChild;
  };

  float f1a=blend(idx,0); float f2a=blend(idx,1); float f3a=blend(idx,2);
  float f1b=blend(idx+1,0); float f2b=blend(idx+1,1); float f3b=blend(idx+1,2);

  f1 = (f1a + (f1b - f1a) * frac) * pitchScale;
  f2 = (f2a + (f2b - f2a) * frac) * pitchScale;
  f3 = (f3a + (f3b - f3a) * frac) * pitchScale;

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
// Biquad sample processing
// ---------------------------------------------------------------------------
inline float AudioFilterFormant::processBiquad(Biquad &bq,float x){
  float y = bq.b0*x + bq.z1;
  bq.z1 = bq.b1*x - bq.a1*y + bq.z2;
  bq.z2 = bq.b2*x - bq.a2*y;
  return y;
}

// ---------------------------------------------------------------------------
// Main audio update loop
// ---------------------------------------------------------------------------
void AudioFilterFormant::update(void){
  smoothParams();
  morphBiquadCoeffs(bq1);
  morphBiquadCoeffs(bq2);
  morphBiquadCoeffs(bq3);

  audio_block_t *in = receiveReadOnly(0);
  if(!in) return;
  if(mix == 0.0f) {
    transmit(in);
    release(in);
	return; 
  }
  audio_block_t *out = allocate();
  if(!out){ 
	release(in); 
	return; 
  }

  for(int i=0;i<AUDIO_BLOCK_SAMPLES;i++){
    float x = (float)in->data[i] / 32768.0f;
    float y1 = processBiquad(bq1,x);
    float y2 = processBiquad(bq2,x);
    float y3 = processBiquad(bq3,x);
	float wet = (y1 + y2 + y3) * (gain * compGain * 5.0f / 3.0f);
	float y = (x * (1.0f - mix)) + (wet * mix);    
	if(y>1) 
		y=1; 
	else if(y<-1) 
		y=-1;
    out->data[i] = (int16_t)(y * 32767.0f);
  }

  transmit(out);
  release(out);
  release(in);
}
