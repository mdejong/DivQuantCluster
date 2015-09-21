/*
 
 Copyright (c) 2015, M. Emre Celebi
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#include "DivQuantHeader.h"

#include <time.h>
#include <assert.h>

/* TODO: What if num_bits == 0 */

// This method will reduce the precision of each component of each pixel by setting
// the number of bits on the right side of the value to zero. Note that this method
// works properly when inPixels and outPixels are the same buffer to support in
// place processing.

void
cut_bits ( const uint32_t *inPixels,
          const uint32_t numPixels,
          uint32_t *outPixels,
          const uchar num_bits_red,
          const uchar num_bits_green,
          const uchar num_bits_blue )
{
  uchar shift_red, shift_green, shift_blue;
  int i;
  uint32_t pixel;
  uint32_t B, G, R;
  
  if ( !validate_num_bits ( num_bits_red ) ||
      !validate_num_bits ( num_bits_green ) ||
      !validate_num_bits ( num_bits_blue ) )
  {
    return;
  }
  
  shift_red = 8 - num_bits_red;
  shift_green = 8 - num_bits_green;
  shift_blue = 8 - num_bits_blue;
  
  const int displayTimings = 0;
  
  clock_t t1, t2;
  
  if (displayTimings) {
    t1 = clock();
  }
  
  if (shift_red == shift_green && shift_red == shift_blue) {
    // Shift and mask pixels as whole words when the shift amount
    // for all 3 channel is the same.
    
    const uint32_t shift = shift_red;
    const uint32_t byteMask = ((0xFF >> shift) << shift);
    const uint32_t wordMask = (byteMask << 16) | (byteMask << 8) | byteMask;
    
    for ( i = 0; i < numPixels; i++ )
    {
      pixel = inPixels[i];
      pixel &= wordMask;
      pixel >>= shift;
      outPixels[i] = pixel;
    }
  } else {
    for ( i = 0; i < numPixels; i++ )
    {
      pixel = inPixels[i];
      
      B = pixel & 0xFF;
      G = (pixel >> 8) & 0xFF;
      R = (pixel >> 16) & 0xFF;
      
      B = B >> shift_blue;
      G = G >> shift_green;
      R = R >> shift_red;
      
      pixel = (R << 16) | (G << 8) | B;
      outPixels[i] = pixel;
    }
  }
  
  if (displayTimings) {
    t2 = clock();
    long elapsed = timediff(t1, t2);
    printf("cut_bits() elapsed: %ld ms aka %0.2f s\n", elapsed, elapsed/1000.0f);
  }
  
  return;
}
