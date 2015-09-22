#include "PngContext.h"

#include "quant_util.h"

#include "CalcError.h"

#include <unordered_map>
#include <vector>

#include <iostream>

#include <assert.h>

using namespace std;

// Scan input values to make sure there are no duplicate pixels

#if defined(DEBUG)

void checkForDuplicates(vector<uint32_t> &inPixelsVec, uint32_t allowedDup)
{
  // In DEBUG mode, use unordered_map to check for duplicate pixels in the input.
  // Note that BGRA pixels will be ignored since different alpha values will be
  // treated as the same point as fully opaque. The caller must take care to
  // implement this alpha value filtering before invoking this method.
  
  unordered_map<uint32_t, uint32_t> uniquePixelMap;
  
  for ( uint32_t pixel : inPixelsVec ) {
    if (pixel != allowedDup && uniquePixelMap.count(pixel) > 0) {
      fprintf(stdout, "input contains duplicate BGR pixel 0x%08X\n", pixel);
      exit(1);
    }
    
    uniquePixelMap[pixel] = 0;
  }
  
  return;
}

#endif // DEBUG

// Given a vector of pixels and a pixel that may or may not be in the vector, return
// the pixel in the vector that is closest to the indicated pixel.

uint32_t closestToPixel(const vector<uint32_t> &pixels, const uint32_t closeToPixel) {
  const bool debug = false;
  
#if defined(DEBUG)
  assert(pixels.size() > 0);
#endif // DEBUG
  
  unsigned int minDist = (~0);
  uint32_t closestToPixel = 0;
  
  uint32_t cB = closeToPixel & 0xFF;
  uint32_t cG = (closeToPixel >> 8) & 0xFF;
  uint32_t cR = (closeToPixel >> 16) & 0xFF;
  
  for ( uint32_t pixel : pixels ) {
    uint32_t B = pixel & 0xFF;
    uint32_t G = (pixel >> 8) & 0xFF;
    uint32_t R = (pixel >> 16) & 0xFF;
    
    int dB = (int)cB - (int)B;
    int dG = (int)cG - (int)G;
    int dR = (int)cR - (int)R;
    
    unsigned int d3 = (unsigned int) ((dB * dB) + (dG * dG) + (dR * dR));
    
    if (debug) {
      cout << "d3 from (" << B << "," << G << "," << R << ") to closeToPixel (" << cB << "," << cG << "," << cR << ") is (" << dB << "," << dG << "," << dR << ") = " << d3 << endl;
    }
    
    if (d3 < minDist) {
      closestToPixel = pixel;
      minDist = d3;
      
      if ((debug)) {
        fprintf(stdout, "new    closestToPixel 0x%08X\n", closestToPixel);
      }
      
      if (minDist == 0) {
        // Quit the loop once a zero distance has been found
        break;
      }
    }
  }
  
  if ((debug)) {
    fprintf(stdout, "return closestToPixel 0x%08X\n", closestToPixel);
  }
  
  return closestToPixel;
}

// Given a vector of cluster center pixels, determine a cluster to cluster walk order based on 3D
// distance from one cluster center to the next. This method returns a vector of offsets into
// the cluster table with the assumption that the number of clusters fits into a 16 bit offset.

vector<uint32_t> generate_cluster_walk_on_center_dist(const vector<uint32_t> &clusterCenterPixels)
{
  const bool debugDumpClusterWalk = false;
  
  int numClusters = (int) clusterCenterPixels.size();
  
  unordered_map<uint32_t, uint32_t> clusterCenterToClusterOffsetMap;
  
  int clusteri = 0;
  
  for ( clusteri = 0; clusteri < numClusters; clusteri++) {
    uint32_t closestToCenterOfMassPixel = clusterCenterPixels[clusteri];
    clusterCenterToClusterOffsetMap[closestToCenterOfMassPixel] = clusteri;
  }
  
  // Reorder the clusters so that the first cluster contains the pixel with the value
  // that is closest to zero. Then, the next cluster is determined by checking
  // the distance to the first pixel in the next cluster. The clusters are already
  // ordered in terms of density so this reordering logic basically jumps from one
  // cluster to the next in terms of the shortest distance to the next clsuter.
  
  vector<uint32_t> closestSortedClusterOrder;
  
  closestSortedClusterOrder.reserve(numClusters);
  
  if ((1)) {
    // Choose cluster that is closest to (0,0,0)
    
    uint32_t closestToZeroCenter = closestToPixel(clusterCenterPixels, 0x0);
    
    int closestToZeroClusteri = -1;
    
    clusteri = 0;
    
    for ( uint32_t clusterCenter : clusterCenterPixels ) {
      if (closestToZeroCenter == clusterCenter) {
        closestToZeroClusteri = clusteri;
        break;
      }
      
      clusteri += 1;
    }
    
    assert(closestToZeroClusteri != -1);
    
    if (debugDumpClusterWalk) {
      fprintf(stdout, "closestToZero 0x%08X is in clusteri %d\n", closestToZeroCenter, closestToZeroClusteri);
    }
    
    closestSortedClusterOrder.push_back(closestToZeroClusteri);
    
    // Calculate the distance from the cluster center to the next closest cluster center.
    
    {
      uint32_t closestToCenterOfMassPixel = clusterCenterPixels[closestToZeroClusteri];
      
      auto it = clusterCenterToClusterOffsetMap.find(closestToCenterOfMassPixel);
      
#if defined(DEBUG)
      assert(it != end(clusterCenterToClusterOffsetMap));
#endif // DEBUG
      
      clusterCenterToClusterOffsetMap.erase(it);
    }
    
    // Each remaining cluster is represented by an entry in clusterCenterToClusterOffsetMap.
    // Get the center coord and use the center to lookup the cluster index. Then find
    // the next closest cluster in terms of distance in 3D space.
    
    uint32_t closestCenterPixel = clusterCenterPixels[closestToZeroClusteri];
    
    for ( ; 1 ; ) {
      if (clusterCenterToClusterOffsetMap.size() == 0) {
        break;
      }
      
      vector<uint32_t> remainingClustersCenterPoints;
      
      for ( auto it = begin(clusterCenterToClusterOffsetMap); it != end(clusterCenterToClusterOffsetMap); it++) {
        //uint32_t clusterCenterPixel = it->first;
        uint32_t clusterOffset = it->second;
        
        uint32_t clusterStartPixel = clusterCenterPixels[clusterOffset];
        remainingClustersCenterPoints.push_back(clusterStartPixel);
      }
      
      if (debugDumpClusterWalk) {
        fprintf(stdout, "there are %d remaining center pixel clusters\n", (int)remainingClustersCenterPoints.size());
        
        for ( uint32_t pixel : remainingClustersCenterPoints ) {
          fprintf(stdout, "0x%08X\n", pixel);
        }
      }
      
      uint32_t nextClosestClusterCenterPixel = closestToPixel(remainingClustersCenterPoints, closestCenterPixel);
      
      if (debugDumpClusterWalk) {
        fprintf(stdout, "nextClosestClusterPixel is 0x%08X from current clusterEndPixel 0x%08X\n", nextClosestClusterCenterPixel, closestCenterPixel);
      }
      
      assert(nextClosestClusterCenterPixel != closestCenterPixel);
      
#if defined(DEBUG)
      assert(clusterCenterToClusterOffsetMap.count(nextClosestClusterCenterPixel) > 0);
#endif // DEBUG
      
      uint32_t nextClosestClusteri = clusterCenterToClusterOffsetMap[nextClosestClusterCenterPixel];
      
      closestSortedClusterOrder.push_back(nextClosestClusteri);
      
      {
        // Find index from next closest and then lookup cluster center
        
        uint32_t nextClosestCenterPixel = clusterCenterPixels[nextClosestClusteri];
        
        auto it = clusterCenterToClusterOffsetMap.find(nextClosestCenterPixel);
#if defined(DEBUG)
        assert(it != end(clusterCenterToClusterOffsetMap));
#endif // DEBUG
        clusterCenterToClusterOffsetMap.erase(it);
      }
      
      closestCenterPixel = nextClosestClusterCenterPixel;
    }
    
    assert(closestSortedClusterOrder.size() == clusterCenterPixels.size());
  }
  
  return closestSortedClusterOrder;
}

void process_file(PngContext *cxt)
{
  // Input contains all pixels from image, dedup pixels using
  // an unordered_map and then sort after the dedup.
  
  unordered_map<uint32_t, uint32_t> uniquePixelMap;
  
  int inputImageNumPixels = cxt->width * cxt->height;
  
  printf("read  %d pixels from input image\n", inputImageNumPixels);
  
  for ( int i = 0; i < inputImageNumPixels; i++) {
    uint32_t pixel = cxt->pixels[i];
    uniquePixelMap[pixel] = 0;
  }
  
  vector<uint32_t> allSortedUniquePixels;
  
  for ( auto it = begin(uniquePixelMap); it != end(uniquePixelMap); it++ ) {
    uint32_t pixel = it->first;
    allSortedUniquePixels.push_back(pixel);
  }
  
  // Release possibly large amount of memory used for hashtable
  uniquePixelMap = unordered_map<uint32_t, uint32_t>();
  
  // Sort pixels into int order (this can take some time for large N)
  
  sort(begin(allSortedUniquePixels), end(allSortedUniquePixels));
  
#if defined(DEBUG)
  checkForDuplicates(allSortedUniquePixels, 0);
#endif // DEBUG
  
  // Allocate input and output buffers of uint32_t and pass to quant method
  
  printf("found %d unique pixels in input image\n", (int)allSortedUniquePixels.size());
  
  int numPixels = (int) allSortedUniquePixels.size();
  uint32_t numClusters = 256;
  
  uint32_t *inUniquePixels = new uint32_t[numPixels];
  uint32_t *outUniquePixels = new uint32_t[numPixels];
  uint32_t *outColortableOffsets = new uint32_t[numPixels];
  uint32_t *outColortablePixels = new uint32_t[numClusters];

  {
    int i = 0;
    for ( uint32_t pixel : allSortedUniquePixels ) {
      inUniquePixels[i++] = pixel;
    }
  }
  
  // Deallocate allSortedUniquePixels since the input buffer can be quite large
  allSortedUniquePixels = vector<uint32_t>();
  
  // Note that this method writes numClusters with a possibly smaller N in the case
  // where not enough points exist to split into N clusters.
  
  quant_recurse(numPixels, inUniquePixels, outUniquePixels, &numClusters, outColortablePixels);
  
  // Print absolute mean and squared mean error metrics that indicate cluster quality
  
  if ((1)) {
    double cMAE = calc_combined_mean_abs_error(numPixels, inUniquePixels, outUniquePixels);
    
    fprintf(stdout, "combined MAE %0.8f\n", cMAE);
    
    double cMSE = calc_combined_mean_sqr_error(numPixels, inUniquePixels, outUniquePixels);
    
    fprintf(stdout, "combined MSE %0.8f\n", cMSE);
  }
  
  // Resort cluster in terms of shortest distance from one cluster center to the next
  // to implement a cluster sort order.
  
  vector<uint32_t> clusterCenterPixels;
  
  for ( int i = 0; i < numClusters; i++) {
    clusterCenterPixels.push_back(outColortablePixels[i]);
  }
  
  if ((1)) {
    fprintf(stdout, "numClusters %5d\n", numClusters);
    
    unordered_map<uint32_t, uint32_t> seen;
    
    for ( int i = 0; i < numClusters; i++ ) {
      uint32_t pixel;
      pixel = outColortablePixels[i];
      
      if (seen.count(pixel) > 0) {
        fprintf(stdout, "cmap[%3d] = 0x%08X (DUP of %d)\n", i, pixel, seen[pixel]);
      } else {
        fprintf(stdout, "cmap[%3d] = 0x%08X\n", i, pixel);
        
        // Note that only the first seen index is retained, this means that a repeated
        // pixel value is treated as a dup.
        
        seen[pixel] = i;
      }
    }
    
    fprintf(stdout, "cmap contains %3d unique entries\n", (int)seen.size());
    
    int numQuantUnique = (int)seen.size();

    assert(numQuantUnique == numClusters);
  }
  
  // Library returns quant pixel, need to quickly map the quant pixel to
  // colortable offset and store that to buffer.
  
  unordered_map<uint32_t,uint32_t> colorTableMap;
  
  for ( int i = 0; i < numClusters; i++ ) {
    uint32_t pixel = outColortablePixels[i];
    if (colorTableMap.count(pixel) == 0) {
      colorTableMap[pixel] = i;
    }
  }
  
  // Colormap should not contain duplicates
  assert(numClusters == colorTableMap.size());
  
  // Use colorTableMap to write cluster offset for each output pixel
  
  for ( int i = 0; i < numPixels; i++ ) {
    uint32_t pixel = outUniquePixels[i];
#if defined(DEBUG)
    assert(colorTableMap.count(pixel) > 0);
#endif // DEBUG
    uint32_t offset = colorTableMap[pixel];
    outColortableOffsets[i] = offset;
  }
  
  // Foreach cluster, generate a list of original pixels that quant to the
  // given cluster center pixel.
  
  unordered_map<uint32_t, vector<uint32_t> > dedupQuantPixelCollection;
  unordered_map<uint32_t,uint32_t> inPixelToQuantPixel;
  
  for ( int i = 0; i < numPixels; i++ ) {
    uint32_t origPixel = inUniquePixels[i];
    uint32_t quantPixel = outUniquePixels[i];
    uint32_t offset = outColortableOffsets[i];
    
    // Dedup and filter into cluster specific list of pixels
    // ordered by integer order.
    
    vector<uint32_t> &vec = dedupQuantPixelCollection[quantPixel];
    vec.push_back(origPixel);
    
    if ((0)) {
      fprintf(stdout, "orig -> offset -> quant : 0x%08X -> %3d -> 0x%08X\n", origPixel, offset, quantPixel);
    }

    // Copy the alpha channel from the unique input pixel to the quant output pixel
    
//    if ((1) && ((origPixel >> 24) != 0 && (origPixel >> 24) != 0xFF)) {
//      fprintf(stdout, "orig -> offset -> quant : 0x%08X -> %3d -> 0x%08X\n", origPixel, offset, quantPixel);
//    }
    
    quantPixel &= 0x00FFFFFF;
    quantPixel |= (origPixel & 0xFF000000);

//    if ((1) && ((origPixel >> 24) != 0 && (origPixel >> 24) != 0xFF)) {
//      fprintf(stdout, "orig -> offset -> quant : 0x%08X -> %3d -> 0x%08X\n", origPixel, offset, quantPixel);
//    }
    
    inPixelToQuantPixel[origPixel] = quantPixel;
  }
  
  // Generate cluster to cluster walk (sort) order
  
  vector<uint32_t> sortedOffsets = generate_cluster_walk_on_center_dist(clusterCenterPixels);
  
  // Once cluster centers have been sorted by 3D color cube distance, emit "centers.png"
  
  if ((1)) {
    PngContext centersCxt;
    PngContext_init(&centersCxt);
    PngContext_copy_settngs(&centersCxt, cxt);
    
    PngContext_alloc_pixels(&centersCxt, numClusters, 1);
    
    uint32_t *outPixels = centersCxt.pixels;
    
    for (int i = 0; i < numClusters; i++) {
      int si = (int) sortedOffsets[i];
      uint32_t quantPixel = clusterCenterPixels[si];
      outPixels[i] = quantPixel;
    }
    
    char *outSortedClusterCentersFilename = (char*)"centers.png";
    
    write_png_file((char*)outSortedClusterCentersFilename, &centersCxt);
    
    printf("wrote %d sorted cluster center pixels to %s\n", numClusters, outSortedClusterCentersFilename);
    
    PngContext_dealloc(&centersCxt);
  }
  
  // Write clustered pixels and PNG image with N pixels in rows of at least 256
  
  int totalPixelsWritten = 0;
  
  // Write image that contains pixels clustered into N clusters where each row in
  // the image corresponds to a cluster
  
  PngContext cxt2;
  PngContext_init(&cxt2);
  PngContext_copy_settngs(&cxt2, cxt);
  
  // Count num rows needed to represent pixels with max width 256
  
  const int numCols = 256;
  int numRows = 0;
  
  {
    for (int i = 0; i < numClusters; i++) {
      int si = (int) sortedOffsets[i];
      
      uint32_t quantPixel = clusterCenterPixels[si];
      vector<uint32_t> &vec = dedupQuantPixelCollection[quantPixel];
      int N = (int)vec.size();
      
      printf("cluster[%3d]: contains %5d pixels\n", i, N);
      
      if (N == 0) {
        // Emit empty row in this case
        numRows += 1;
      } else if (N < numCols) {
        numRows += 1;
      } else {
        while (N > 0) {
          numRows++;
          if (N == numCols) {
            numRows++;
          }
          N -= numCols;
        }
      }
    }
  }
  
  // Allocate columns x height pixels
  
  PngContext_alloc_pixels(&cxt2, numCols, numRows);
  
  uint32_t *outPixels = cxt2.pixels;
  uint32_t outPixelsi = 0;
  
  for (int i = 0; i < numClusters; i++) {
    int si = (int) sortedOffsets[i];
    
    uint32_t quantPixel = clusterCenterPixels[si];
    vector<uint32_t> &vec = dedupQuantPixelCollection[quantPixel];
    
    int pixelsWritten = 0;
    
    for ( uint32_t pixel : vec ) {
      outPixels[outPixelsi++] = pixel;
      pixelsWritten++;
    }
    
    totalPixelsWritten += pixelsWritten;
    
    if (true) {
      // Pad each cluster out to 256
      
      int numPointsInCluster = pixelsWritten;
      
      int over = numPointsInCluster % 256;
      int under = 0;
      
      if (over == 0) {
        // Cluster of exactly 256 pixels, emit 256 zeros to indicate this case.
        under = 256;
      } else {
        under = 256 - over;
      }
      
      for (int j = 0; j < under; j++) {
        uint32_t pixel = 0;
        outPixels[outPixelsi++] = pixel;
        totalPixelsWritten++;
      }
    }
  }
  
  char *outClustersFilename = (char*)"clusters.png";
  
  write_png_file((char*)outClustersFilename, &cxt2);
  
  printf("wrote %d cluster pixels to %s\n", totalPixelsWritten, outClustersFilename);
  
  int totalRowsOf256 = totalPixelsWritten/256;
  if ((totalPixelsWritten % 256) != 0) {
    assert(0);
  }
  assert(totalRowsOf256 == numRows);
  
  PngContext_dealloc(&cxt2);
  
  // Combine sorted pixels into a flat array of pixels and emit as image with 256 columns
  
  PngContext cxt3;
  PngContext_init(&cxt3);
  PngContext_copy_settngs(&cxt3, cxt);
  
  allSortedUniquePixels.clear();
  
  for (int i = 0; i < numClusters; i++) {
    int si = (int) sortedOffsets[i];
    
    uint32_t quantPixel = clusterCenterPixels[si];
    vector<uint32_t> &vec = dedupQuantPixelCollection[quantPixel];
    
    for ( uint32_t pixel : vec ) {
      allSortedUniquePixels.push_back(pixel);
    }
  }
  
#if defined(DEBUG)
  checkForDuplicates(allSortedUniquePixels, 0);
#endif // DEBUG
  
  numRows = (int)allSortedUniquePixels.size() / 256;
  if (((int)allSortedUniquePixels.size() % 256) != 0) {
    numRows++;
  }
  
  PngContext_alloc_pixels(&cxt3, 256, numRows);
  
  int pixeli = 0;
  uint32_t *outPixelsPtr = (uint32_t*)cxt3.pixels;
  
  for ( uint32_t pixel : allSortedUniquePixels ) {
    outPixelsPtr[pixeli++] = pixel;
  }
  
  char *outSortedClustersFilename = (char*)"sorted.png";
  
  write_png_file(outSortedClustersFilename, &cxt3);
  
  printf("wrote %d total sorted pixels to %s\n", (int)allSortedUniquePixels.size(), outSortedClustersFilename);
  
  PngContext_dealloc(&cxt3);
  
  // Finally, generate a version of the original image where each original pixel is replaced
  // by the cluster center the is closest to the pixel. Note that only fully opaque images
  // can be processed in this way to output only pixels with N clusters. Images with partially
  // opaque pixels retain the original partial alpha value from the input pixels so the output
  // number of pixels could be larger than the number of clusters in that case.
  
  if ((1)) {
    PngContext quantCxt;
    PngContext_init(&quantCxt);
    PngContext_copy_settngs(&quantCxt, cxt);
    PngContext_alloc_pixels(&quantCxt, cxt->width, cxt->height);
    
    assert(inputImageNumPixels == (cxt->width * cxt->height));

    uint32_t *inOriginalPixels = cxt->pixels;
    uint32_t *outQuantPixels = quantCxt.pixels;
    
    for (int i = 0; i < inputImageNumPixels; i++) {
      uint32_t inPixel = inOriginalPixels[i];
#if defined(DEBUG)
      assert(inPixelToQuantPixel.count(inPixel) > 0);
#endif // DEBUG
      uint32_t quantPixel = inPixelToQuantPixel[inPixel];
      outQuantPixels[i] = quantPixel;
    }
    
    char *outQuantFilename = (char*)"quant.png";
    
    write_png_file((char*)outQuantFilename, &quantCxt);
    
    printf("wrote quant replaced pixels to %s\n", outQuantFilename);
    
    PngContext_dealloc(&quantCxt);
  }
  
  return;
}

// deallocate memory

void cleanup(PngContext *cxt)
{
  PngContext_dealloc(cxt);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage divquantcluster PNG\n");
    exit(1);
  }
  PngContext cxt;
  read_png_file(argv[1], &cxt);
  
  if ((0)) {
    // Write input data just read back out to a PNG image to make sure read/write logic
    // is dealing correctly with wacky issues like grayscale and palette images
    
    PngContext copyCxt;
    PngContext_init(&copyCxt);
    PngContext_copy_settngs(&copyCxt, &cxt);
    PngContext_alloc_pixels(&copyCxt, cxt.width, cxt.height);
    
    uint32_t *inOriginalPixels = cxt.pixels;
    uint32_t *outPixels = copyCxt.pixels;
    
    int numPixels = cxt.width * cxt.height;
    
    for (int i = 0; i < numPixels; i++) {
      uint32_t inPixel = inOriginalPixels[i];
      outPixels[i] = inPixel;
    }
    
    char *inoutFilename = (char*)"in_out.png";
    write_png_file((char*)inoutFilename, &copyCxt);
    printf("wrote input copy to %s\n", inoutFilename);
    PngContext_dealloc(&copyCxt);
  }

  printf("processing %d pixels from image of dimensions %d x %d\n", cxt.width*cxt.height, cxt.width, cxt.height);
  
  process_file(&cxt);
  
  cleanup(&cxt);
  return 0;
}
