/*
 
 Copyright (c) 2015, M. Emre Celebi
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#include "DivQuantHeader.h"

#include <assert.h>

#define L2_SQR( X1, Y1, Z1, X2, Y2, Z2 )\
temp = ( X1 ) - ( X2 );\
dist = temp * temp;\
temp = ( Y1 ) - ( Y2 );\
dist += temp * temp;\
temp = ( Z1 ) - ( Z2 );\
dist += temp * temp\

static
int
L2_sqr_int ( const int x1, const int y1, const int z1,
            const int x2, const int y2, const int z2 )
{
  int temp;
  int dist;
  
  L2_SQR ( x1, y1, z1, x2, y2, z2 );
  
  return dist;
}

#undef L2_SQR

void
check_mem ( const int x )
{
  if ( x != 0 )
  {
    fprintf ( stderr, "Insufficient memory !\n" );
    abort ( );
  }
}

/* dec_factor: decimation factor */

#define HASH_SIZE 20023

/* TODO: No need for the bitmasking in HASH */

#define HASH(R, G, B) ( ( ( ( long ) ( R ) * 33023 + \
( long ) ( G ) * 30013 + \
( long ) ( B ) * 27011 ) \
& 0x7fffffff ) % HASH_SIZE )

typedef struct Bucket_Entry* Bucket;

struct Bucket_Entry
{
  int red;
  int green;
  int blue;
  int value;
  Bucket next;
};

typedef Bucket* Hash_Table;

// This method will dedup unique pixels and subsample pixels
// based on dec_factor. When dec_factor is 1 then this method
// would not do anything if the input is already unique, use
// unique_colors_as_doubles() in that case.

double *
calc_color_table ( const uint32_t *inPixels,
                  const uint32_t numPixels,
                  uint32_t *outPixels,
                  const uint32_t numRows,
                  const uint32_t numCols,
                  const int dec_factor,
                  int *num_colors )
{
  int ih;
  int ir, ic;
  int hash;
  int index;
  int red, green, blue;
  double norm_factor;
  Bucket bucket;
  Bucket temp_bucket;
  Hash_Table hash_table;
  double *weights;
  uint32_t pixel;
  
  if ( dec_factor <= 0 )
  {
    fprintf ( stderr, "Decimation factor ( %d ) should be positive !\n", dec_factor );
    
    return NULL;
  }
  
  hash_table = ( Hash_Table ) malloc ( HASH_SIZE * sizeof ( Bucket ) );
  check_mem ( hash_table == NULL );
  
  for ( ih = 0; ih < HASH_SIZE; ih++ )
  {
    hash_table[ih] = NULL;
  }
  
  *num_colors = 0;
  
  for ( ir = 0; ir < numRows; ir += dec_factor )
  {
    for ( ic = 0; ic < numCols; ic += dec_factor )
    {
      pixel = inPixels[ic + (ir * numRows)];
      blue = pixel & 0xFF;
      green = (pixel >> 8) & 0xFF;
      red = (pixel >> 16) & 0xFF;
      
      /* Determine the bucket */
      hash = HASH ( red, green, blue );
      
      /* Search for the color in the bucket chain */
      for ( bucket = hash_table[hash]; bucket != NULL; bucket = bucket->next )
      {
        if ( bucket->red == red && bucket->green == green && bucket->blue == blue )
        {
          /* This color exists in the hash table */
          break;
        }
      }
      
      if ( bucket != NULL )
      {
        /* This color exists in the hash table */
        bucket->value++;
      }
      else
      {
        (*num_colors)++;
        
        /* Create a new bucket entry for this color */
        bucket = ( Bucket ) malloc ( sizeof ( struct Bucket_Entry ) );
        check_mem ( bucket == NULL );
        
        bucket->red = red;
        bucket->green = green;
        bucket->blue = blue;
        bucket->value = 1;
        bucket->next = hash_table[hash];
        hash_table[hash] = bucket;
      }
    }
  }
  
  // printf ( "# colors = %d\n", num_colors );
  
  weights = new double[*num_colors];
  check_mem ( weights == NULL );
  
  /* Normalization factor to obtain color frequencies to color probabilities */
  /* norm_factor = ( dec_factor * dec_factor ) / ( double ) num_pixels; */
  norm_factor =  1.0 / ( ceil ( numRows / ( double ) dec_factor ) * ceil ( numCols / ( double ) dec_factor ) );
  
  index = 0;
  for ( hash = 0; hash < HASH_SIZE; hash++ )
  {
    for ( bucket = hash_table[hash]; bucket != NULL; )
    {
      uint32_t B = bucket->blue;
      uint32_t G = bucket->green;
      uint32_t R = bucket->red;
      uint32_t pixel = (R << 16) | (G << 8) | B;
      outPixels[index] = pixel;
      
      weights[index] = norm_factor * bucket->value;
      
      index++;
      
      // Save the current bucket pointer
      temp_bucket = bucket;
      
      // Advance to the next bucket
      bucket = bucket->next;
      
      // Free the current bucket
      free ( temp_bucket );
    }
  }
  
  free ( hash_table );
  
  return weights;
}

double
get_double_scale(
                 const uint32_t *inPixels,
                 const uint32_t numPixels)
{
  int numRows = 1;
  int numCols = numPixels;
  int dec_factor = 1;
  
  //double weight =  0.25;
  
  //  double weight =  1.0;
  double weight =  1.0 / ( ceil ( numRows / ( double ) dec_factor ) * ceil ( numCols / ( double ) dec_factor ) );
  
  return weight;
}

static inline
bool asc_weighted_pixel(const Pixel_Int & a, const Pixel_Int & b) {
  return a.weight < b.weight;
}

static void
sort_color ( Pixel_Int *cmap, const int num_colors )
{
  std::vector<Pixel_Int> pixelVec(num_colors);
  for ( int i = 0; i < num_colors; i++ ) {
    pixelVec[i] = cmap[i];
  }
  std::sort(begin(pixelVec), end(pixelVec), asc_weighted_pixel);
  for ( int i = 0; i < num_colors; i++ ) {
    cmap[i] = pixelVec[i];
  }
}

//#define SEARCH_DEBUG
//#define SEARCH_DEBUG_SORT

void
map_colors_mps ( const uint32_t *inPixelsPtr, uint32_t numPixels, uint32_t *outPixelsPtr, uint32_t *outColortablePtr, int colormapSize )
{
  int ik, ic;
  int index;
  int low, high;
  int m, n;
  int dist;
  int min_dist;
  int red, green, blue;
  int sum;
  int size_lut_init = 3 * MAX_RGB + 1;
  int max_sum = 3 * MAX_RGB;
  int *lut_init;
  Pixel_Int *cmap;
  int up, down;
  uint32_t B, G, R, pixel;
  double *lut_ssd_buffer;
  double *lut_ssd;
  int size_lut_ssd;
  
  int num_colors = colormapSize;
  assert(num_colors > 0);
  
  lut_init = ( int * ) malloc ( size_lut_init * sizeof ( int ) );
  check_mem ( lut_init == NULL );
  
  cmap = ( Pixel_Int * ) malloc ( num_colors * sizeof ( Pixel_Int ) );
  for (int i = 0; i < num_colors; i++) {
    uint32_t pixel = outColortablePtr[i];
    Pixel_Int *pi = &cmap[i];
    pi->blue = pixel & 0xFF;
    pi->green = (pixel >> 8) & 0xFF;
    pi->red = (pixel >> 16) & 0xFF;
    pi->weight = 0;
  }
  
#if defined(SEARCH_DEBUG_SORT)
  for ( int si = 0; si < num_colors; si++ ) {
    printf("table[%3d] = (%3d %3d %3d) 0x%02X%02X%02X\n", si, cmap[si].red, cmap[si].green, cmap[si].blue, cmap[si].red, cmap[si].green, cmap[si].blue);
  }
#endif // SEARCH_DEBUG_SORT
  
  size_lut_ssd = 2 * max_sum + 1;
  lut_ssd_buffer = ( double * ) malloc ( size_lut_ssd * sizeof ( double ) );
  check_mem ( lut_ssd_buffer == NULL );
  
  lut_ssd = lut_ssd_buffer + max_sum;
  lut_ssd[0] = 0;
  
  // Premultiply the LUT entries by (1/3) -- see below
  for ( ik = 1; ik <= max_sum; ik++ )
  {
#if defined(DEBUG)
    // Translate ik to offset in terms of the buffer start
    int neg_offset_from_start = max_sum + -ik;
    int pos_offset_from_start = max_sum + ik;
    
    assert(neg_offset_from_start >= 0);
    assert(neg_offset_from_start < size_lut_ssd);
    
    assert(pos_offset_from_start >= 0);
    assert(pos_offset_from_start < size_lut_ssd);
#endif // DEBUG
    
    //    printf("lut_ssd[%d] = %0.8f\n", -ik, lut_ssd[-ik]);
    //    printf("lut_ssd[%d] = %0.8f\n", ik, lut_ssd[ik]);
    
    lut_ssd[-ik] = lut_ssd[ik] = ( ik * ik ) / 3.0;
  }
  
  // Sort the palette by the sum of color components.
  for ( ic = 0; ic < num_colors; ic++ )
  {
#if defined(DEBUG)
    assert(ic >= 0 && ic < num_colors);
#endif // DEBUG
    
    cmap[ic].weight = cmap[ic].red + cmap[ic].green + cmap[ic].blue;
  }
  
  sort_color ( cmap, num_colors );
  
#if defined(SEARCH_DEBUG_SORT)
  for ( int si = 0; si < num_colors; si++ ) {
    printf("table[%3d] = (%3d %3d %3d) 0x%02X%02X%02X : w %3d\n", si, cmap[si].red, cmap[si].green, cmap[si].blue, cmap[si].red, cmap[si].green, cmap[si].blue, cmap[si].weight);
  }
#endif // SEARCH_DEBUG_SORT
  
  // Calculate the LUT
  if (num_colors >= 2) {
    // Average first 2 weights
    low = ( int ) ( 0.5 * ( cmap[0].weight + cmap[1].weight ) + 0.5 ); // round
  } else {
    low = 1;
  }
  for ( ik = 0; ik < low; ik++ )
  {
#if defined(DEBUG)
    assert(ik >= 0);
    assert(ik < size_lut_init);
#endif // DEBUG
    
    //    printf("lut_init[%d] (low) = %d\n", ik, lut_init[ik]);
    
    lut_init[ik] = 0;
  }
  
  if (num_colors >= 2) {
#if defined(DEBUG)
    assert((num_colors-2) >= 0);
#endif // DEBUG
    high = ( int ) ( 0.5 * ( cmap[num_colors - 2].weight + cmap[num_colors - 1].weight ) + 0.5 ); // round
  } else {
    high = 1;
  }
  
  for ( ik = high; ik < size_lut_init; ik++ )
  {
    lut_init[ik] = num_colors - 1;
    //   printf("lut_init[%3d] (high) = %d\n", ik, lut_init[ik]);
  }
  
  for ( ic = 1; ic < num_colors - 1; ic++ )
  {
#if defined(DEBUG)
    assert(((ic - 1) >= 0) && ((ic - 1) < num_colors));
    assert((ic >= 0) && (ic < num_colors));
    assert(((ic + 1) >= 0) && ((ic + 1) < num_colors));
#endif // DEBUG
    
    low = ( int ) ( 0.5 * ( cmap[ic - 1].weight + cmap[ic].weight ) + 0.5 );  // round
    high = ( int ) ( 0.5 * ( cmap[ic].weight + cmap[ic + 1].weight ) + 0.5 ); // round
    
    for ( ik = low; ik < high; ik++ )
    {
#if defined(DEBUG)
      assert((ik >= 0) && (ik < size_lut_init));
#endif // DEBUG
      lut_init[ik] = ic;
    }
  }
  
  for ( ik = 0; ik < numPixels; ik++ )
  {
    pixel = inPixelsPtr[ik];
    blue = pixel & 0xFF;
    green = (pixel >> 8) & 0xFF;
    red = (pixel >> 16) & 0xFF;
    sum = red + green + blue;
    
    //    for ( int si = 0; si < 256; si++ ) {
    //      printf("table[%3d] = (%3d %3d %3d) 0x%02X%02X%02X : w = %d\n", si, cmap[si].red, cmap[si].green, cmap[si].blue, cmap[si].red, cmap[si].green, cmap[si].blue, cmap[si].weight);
    //    }
    
    // Determine the initial searched colour cinit in the palette for cp.
#if defined(DEBUG)
    assert(sum >= 0);
    assert(sum < size_lut_init);
#endif // DEBUG
    index = lut_init[sum];
    
#if defined(SEARCH_DEBUG)
    printf("L2 search start at index %d for pixel 0x%08X : (%3d %3d %3d)\n", index, pixel, red, green, blue);
#endif // SEARCH_DEBUG
    
    // Calculate the squared Euclidean distance between cp and cinit
    min_dist = L2_sqr_int ( red, green, blue,
                           cmap[index].red, cmap[index].green, cmap[index].blue );
    
#if defined(SEARCH_DEBUG)
    {
      B = ( uint8_t ) cmap[index].blue;
      G = ( uint8_t ) cmap[index].green;
      R = ( uint8_t ) cmap[index].red;
      uint32_t tablePixel = (R << 16) | (G << 8) | B;
      printf("min_dist from 0x%08X (%3d %3d %3d) to 0x%08X (%3d %3d %3d) (at index %d) is %d\n", pixel, red, green, blue, index, tablePixel, cmap[index].red, cmap[index].green, cmap[index].blue, min_dist);
    }
#endif // SEARCH_DEBUG
    
    m = n = index;
    up = down = 1;
    while ( up || down )
    {
      if ( down )
      {
        m++;
        
        if ( ( m > ( num_colors - 1 ) ) || ( lut_ssd[sum - cmap[m].weight] > min_dist ) )
        {
          // Terminate the search in DOWN direction
          down = 0;
          
#if defined(SEARCH_DEBUG)
          printf("terminate down search at low index %3d\n", m);
#endif // SEARCH_DEBUG
        }
        else
        {
          dist = L2_sqr_int ( red, green, blue,
                             cmap[m].red, cmap[m].green, cmap[m].blue );
          
#if defined(SEARCH_DEBUG)
          printf("L2 (down) at %3d from (%d %d %d) to 0x%02X%02X%02X (%d %d %d) = %d\n", m, red, green, blue, cmap[m].red, cmap[m].green, cmap[m].blue, cmap[m].red, cmap[m].green, cmap[m].blue, dist);
#endif // SEARCH_DEBUG
          
          if ( dist < min_dist )
          {
            min_dist = dist;
            index = m;
            
#if defined(SEARCH_DEBUG)
            printf("L2 (down) new min at %d\n", index);
#endif // SEARCH_DEBUG
          }
        }
      }
      
      if ( up )
      {
        n--;
        
        if ( ( n < 0 ) || ( lut_ssd[sum - cmap[n].weight] > min_dist ) )
        {
          // Terminate the search in UP direction
          up = 0;
          
#if defined(SEARCH_DEBUG)
          printf("terminate up search at high index %3d\n", n);
#endif // SEARCH_DEBUG
        }
        else
        {
          dist = L2_sqr_int ( red, green, blue,
                             cmap[n].red, cmap[n].green, cmap[n].blue );
          
#if defined(SEARCH_DEBUG)
          printf("L2 (up  ) at %3d from (%d %d %d) to 0x%02X%02X%02X (%d %d %d) = %d\n", n, red, green, blue, cmap[n].red, cmap[n].green, cmap[n].blue, cmap[n].red, cmap[n].green, cmap[n].blue, dist);
#endif // SEARCH_DEBUG
          
          if ( dist < min_dist ) 
          { 
            min_dist = dist; 
            index = n;
            
#if defined(SEARCH_DEBUG)
            printf("L2 (up) new min at %d\n", index);
#endif // SEARCH_DEBUG
          }
        }
      }
    }
    
    B = ( uint8_t ) cmap[index].blue;
    G = ( uint8_t ) cmap[index].green;
    R = ( uint8_t ) cmap[index].red;
    pixel = (R << 16) | (G << 8) | B;
    outPixelsPtr[ik] = pixel;
    
#if defined(SEARCH_DEBUG)
    printf("L2 search finished on index %3d : pixel 0x%08X : (%d %d %d) and min_dist %d\n", index, pixel, R, G, B, min_dist);
#endif // SEARCH_DEBUG
  }
  
  free ( lut_init );
  free ( lut_ssd_buffer );
  free ( cmap );
  
  return;
}
