/* (C) 2007 Jean-Marc Valin, CSIRO
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "psy.h"
#include <math.h>

/* The Vorbis freq<->Bark mapping */
#define toBARK(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))
#define fromBARK(z) (102.f*(z)-2.f*pow(z,2.f)+.4f*pow(z,3.f)+pow(1.46f,z)-1.f)

/* Psychoacoustic spreading function. The idea here is compute a first order 
   recursive filter. The filter coefficient is frequency dependent and 
   chosen such that we have a -10dB/Bark slope on the right side and a -25dB/Bark
   slope on the left side. */
static void spreading_func(float *psd, float *mask, int len, int Fs)
{
   int i;
   float decayL[len], decayR[len];
   float mem;
   //for (i=0;i<len;i++) printf ("%f ", psd[i]);
   /* This can easily be tabulated, which makes the function very fast. */
   for (i=0;i<len;i++)
   {
      float f;
      float deriv;
      /* Real frequency (in Hz) */
      f = Fs*i*(1/(2.f*len));
      /* This is the derivative of the Vorbis freq->Bark function (see above) */
      deriv = (8.288e-8 * f)/(3.4225e-16 *f*f*f*f + 1) +  .009694/(5.476e-7 *f*f + 1) + 1e-4;
      /* Back to FFT bin units */
      deriv *= Fs*(1/(2.f*len));
      /* decay corresponding to -10dB/Bark */
      decayR[i] = pow(.1f, deriv);
      /* decay corresponding to -25dB/Bark */
      decayL[i] = pow(0.0031623f, deriv);
   }
   /* Compute right slope (-10 dB/Bark) */
   mem=psd[0];
   for (i=0;i<len;i++)
   {
      mask[i] = (1-decayR[i])*psd[i] + decayR[i]*mem;
      mem = mask[i];
   }
   /* Compute left slope (-25 dB/Bark) */
   mem=mask[len-1];
   for (i=len-1;i>=0;i--)
   {
      mask[i] = (1-decayR[i])*mask[i] + decayL[i]*mem;
      mem = mask[i];
   }
   //for (i=0;i<len;i++) printf ("%f ", mask[i]); printf ("\n");
#if 0 /* Prints signal and mask energy per critical band */
   for (i=0;i<25;i++)
   {
      int start,end;
      int j;
      float Esig=0, Emask=0;
      start = (int)floor(fromBARK((float)i)*(2*len)/Fs);
      if (start<0)
         start = 0;
      end = (int)ceil(fromBARK((float)(i+1))*(2*len)/Fs);
      if (end<=start)
         end = start+1;
      if (end>len-1)
         end = len-1;
      for (j=start;j<end;j++)
      {
         Esig += psd[j];
         Emask += mask[j];
      }
      printf ("%f %f ", Esig, Emask);
   }
   printf ("\n");
#endif
}

/* Compute a marking threshold from the spectrum X. */
void compute_masking(float *X, float *mask, int len, int Fs)
{
   int i;
   int N=len/2;
   float psd[N];
   psd[0] = X[0]*X[0];
   for (i=1;i<N;i++)
      psd[i] = X[i*2-1]*X[i*2-1] + X[i*2]*X[i*2];
   /* TODO: Do tone masking */
   /* Noise masking */
   spreading_func(psd, mask, N, Fs);
   
}
