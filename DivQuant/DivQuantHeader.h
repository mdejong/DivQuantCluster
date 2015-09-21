/*
 
 Copyright (c) 2015, M. Emre Celebi
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#ifndef DivQuantHeader_h
#define DivQuantHeader_h

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <vector>

#define MAX_RGB     ( 255 )
#define MAX_RGB_SQR ( 65025 ) /* 255 * 255 */
#define MAX_COLORS  ( 256 )

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef struct
{
 int red, green, blue;
 int weight;
} Pixel_Int; /**< Integer Pixel */

typedef struct
{
 double red, green, blue;
 double weight;
} Pixel_Double; /**< (Double) Pixel */

clock_t start_timer ( void );
double stop_timer ( const clock_t );

void check_mem ( const int );

double
get_double_scale(const uint32_t *inPixels,
                 const uint32_t numPixels);

long timediff(clock_t t1, clock_t t2);

void
map_colors_mps ( const uint32_t *inPixelsPtr, uint32_t numPixels, uint32_t *outPixelsPtr, const std::vector<uint32_t> &colormap );

double *
calc_color_table ( const uint32_t *inPixels,
                  const uint32_t numPixels,
                  uint32_t *outPixels,
                  const uint32_t numRows,
                  const uint32_t numCols,
                  const int dec_factor,
                  int *num_colors );

void
cut_bits ( const uint32_t *inPixels,
          const uint32_t numPixels,
          uint32_t *outPixels,
          const uchar num_bits_red,
          const uchar num_bits_green,
          const uchar num_bits_blue );

std::vector<uint32_t>
quant_varpart_fast (
                    const uint32_t numPixels,
                    const uint32_t *inPixels,
                    uint32_t *tmpPixels,
                    const uint32_t numRows,
                    const uint32_t numCols,
                    const int num_colors,
                    const int num_bits,
                    const int dec_factor,
                    const int max_iters,
                    const bool allPixelsUnique);

int validate_num_bits ( const uchar );

#endif // DivQuantHeader_h
