// Header cannot be included into C++ currently so this util method will read an image into
// a format that can be processed by the library

#include "DivQuantHeader.h"

#include "assert.h"

#include <zlib.h>

#include <time.h>

#include "quant_util.h"

#include <unordered_map>

using namespace std;

// Each cluster is represented by an exact floating point cluster center and the variance.

void quant_recurse ( uint32_t numPixels, const uint32_t *inPixelsPtr, uint32_t *outPixelsPtr, uint32_t *numClustersPtr, uint32_t *outColortablePtr, int allPixelsUnique )
{
  const int displayTimings = 1;
  
  const bool dumpDedupCmap = false;
  
  clock_t t1, t2;
  long elapsed;
  
  //int num_colors = 256;
  
  int max_iters = 10;
  //int max_iters = 5;
  //int max_iters = 0; // no local kmeans
  
  int dec_factor = 1; // 1.0 means no decimation
  int num_bits = 8; // note that width = numPixels and height = 1 is passed with uni quant disabled
  
  //  int dec_factor = 1; // 1.0 means no decimation
  //  int num_bits = 7; // note that width = numPixels and height = 1 is passed with uni quant disabled
  
  // half the pixels and uniform quant is very fast
  //  int dec_factor = 2;
  //  int num_bits = 5;
  
  //  int dec_factor = 1;
  //  int num_bits = 6;
  
  if (displayTimings) {
    t1 = clock();
  }
  
  if ((0)) {
    // Determine adler32 for input pixels
    
    uLong adlerSig = adler32(0L, (const Bytef*)inPixelsPtr, (uInt)(numPixels * sizeof(uint32_t)));
    
    fprintf(stdout, "quant_varpart_fast() input pixels adler 0x%08X\n", (int)adlerSig);
  }
  
  quant_varpart_fast( numPixels, inPixelsPtr, outPixelsPtr, 1, numPixels, numClustersPtr, outColortablePtr, num_bits, dec_factor, max_iters, allPixelsUnique);
  
  if (displayTimings) {
    t2 = clock();
    elapsed = timediff(t1, t2);
    printf("quant_varpart_fast() elapsed: %ld ms aka %0.2f s\n", elapsed, elapsed/1000.0f);
  }
  
  int act_num_colors = *numClustersPtr;
  
  if (displayTimings) {
    t1 = clock();
  }
  
  // Dump cmap entries and dedup cmap in case of repeated values that resolve to same RGB entry.
  
  if (dumpDedupCmap) {
    for ( int i = 0; i < act_num_colors; i++ ) {
      // Note that red, green, blue already converted to int at this point
      
      uint32_t colortablePixel = outColortablePtr[i];
      
      uint32_t B = colortablePixel & 0xFF;
      uint32_t G = (colortablePixel >> 8) & 0xFF;
      uint32_t R = (colortablePixel >> 16) & 0xFF;
      uint32_t pixel = (R << 16) | (G << 8) | B;
      
      if ((1)) {
        fprintf(stdout, "cmap[%3d] = 0x%08X = R G B ( %3d %3d %3d )\n", i, pixel, R, G, B);
      }
    }
  }
  
  unordered_map<uint32_t, uint32_t> seen;
  vector<uint32_t> dedupOrder;
  dedupOrder.reserve(act_num_colors);

  for ( int i = 0; i < act_num_colors; i++) {
    uint32_t pixel = outColortablePtr[i];
    if (seen.count(pixel) > 0) {
      continue;
    }
    seen[pixel] = i;
    dedupOrder.push_back(pixel);
  }
  
  if ((int)dedupOrder.size() < act_num_colors) {
    if (dumpDedupCmap) {
      fprintf(stdout, "DEDUP cmap from %d to %d entries\n", act_num_colors, (int)dedupOrder.size());
    }
    
    act_num_colors = (int)dedupOrder.size();
    *numClustersPtr = act_num_colors;
    
    for ( int i = 0; i < act_num_colors; i++) {
      uint32_t pixel = dedupOrder[i];
      outColortablePtr[i] = pixel;
    }
  }
  
  if (dumpDedupCmap) {
    for ( int i = 0; i < act_num_colors; i++ ) {
      // Note that red, green, blue already converted to int at this point
      
      uint32_t colortablePixel = outColortablePtr[i];
      
      uint32_t B = colortablePixel & 0xFF;
      uint32_t G = (colortablePixel >> 8) & 0xFF;
      uint32_t R = (colortablePixel >> 16) & 0xFF;
      uint32_t pixel = (R << 16) | (G << 8) | B;
      
      if ((1)) {
        fprintf(stdout, "cmap[%3d] = 0x%08X = R G B ( %3d %3d %3d )\n", i, pixel, R, G, B);
      }
    }
  }

  // Map input pixels through the colortable
  
  map_colors_mps ( inPixelsPtr, numPixels, outPixelsPtr, outColortablePtr, act_num_colors );
  
  if (displayTimings) {
    t2 = clock();
    elapsed = timediff(t1, t2);
    printf("map_colors_mps() elapsed: %ld ms aka %0.2f s\n", elapsed, elapsed/1000.0f);
  }
  
  if ((0)) {
    // Check adler for quant table output as words
    
    uLong adlerSig = adler32(0L, (const Bytef*)outColortablePtr, (uInt)(act_num_colors * sizeof(uint32_t)));
    
    if ((1)) {
      fprintf(stdout, "quant_varpart_fast() output table adler 0x%08X\n", (int)adlerSig);
    }
  }
  
  return;
}

