/*
 
 Copyright (c) 2015, M. Emre Celebi
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

// M. E. Celebi, Q. Wen, S. Hwang, An Effective Real-Time Color Quantization Method Based on Divisive Hierarchical Clustering, Journal of Real-Time Image Processing, 10(2): 329-344, 2015

#include "DivQuantHeader.h"

#include <vector>

#include "assert.h"

using namespace std;

#define RESET_PIXEL( P ) ( ( P )->red = ( P )->green = ( P )->blue = 0.0 )

#define SQR( X ) ( ( X ) * ( X ) )

//#define VERBOSE

// This class defines a clustering method that divides the input into
// roughly equally sized clusters until N clusters is reached or the
// clusters can be divided no more.

// UW  : true if a uniform weight applies to each pixel evenly
// MT  : type of the member attribute, either uint8_t uint32_t
// KM  : true if 1 or more kmeans iterations will be applied

template <bool UW, typename MT, bool KM>
class DivQuantCluster {
public:

  // The number of input pixels in the buffer pointed to by inPixelsPtr
  uint32_t numPoints;
  
  // The BGRX format pixels that contain (R G B) pixels to be processed
  uint32_t *inPixelsPtr;

  uint32_t *tmpPixelsPtr;

  // In the case where input pixel values have weights that are pixel
  // specific then this weight vector must be non-NULL.
  
  double *weightsPtr;
  
  // If there is a uniform weight applied to each pixel then weights
  // multiplications can be defered until the end of a loop which
  // can be a significant advantage.
  
  double weightUniform;
  
  // The number of k-means iterations done after each cluster split loop
  int maxNumIterations;
  
  // The caller should set the number of clusters desired, when the
  // logic returns the number of actual clusters can be read from
  // this property of from the colortable.size() property.
  
  int numClusters;
  
  // Value in the range 1 -> 8 to indicate the number of valid bits.
  // A right shift of (8 - N) will ignore bits on the right side
  // of the result.
  
  int numBits;
  
  // The output of this logic is a colortable of pixels
  
  std::vector<uint32_t> colortable;

  // The member array is either uint8_t or uint32_t.
  
  MT *member;
  
  // constructor
  DivQuantCluster<UW, MT>() {}
  
  // The primary entry point
  
  void cluster();
  
private:
  
  Pixel_Double total_mean_prop; // componentwise mean of C
  Pixel_Double total_var_prop; // componentwise variance of C

  // Util method that does initial calculations to determine mean and variance for all input pixels
  
  void initMeanAndVar(Pixel_Double *total_mean, Pixel_Double *total_var) {
    const int num_points = this->numPoints;
    const uint32_t *data = this->inPixelsPtr;
    const double data_weight = this->weightUniform;
    
    double mean_red = 0.0, mean_green = 0.0, mean_blue = 0.0;
    double var_red = 0.0, var_green = 0.0, var_blue = 0.0;
    
    for ( int ip = 0; ip < num_points; ip++ )
    {
      uint32_t pixel = data[ip];
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      if (UW) {
        mean_red += R;
        mean_green += G;
        mean_blue += B;
        
        var_red += ( R * R );
        var_green += ( G * G );
        var_blue += ( B * B );
      } else {
        // non-uniform weights

        double tmp_weight = weightsPtr[ip];
        
        mean_red += tmp_weight * R;
        mean_green += tmp_weight * G;
        mean_blue += tmp_weight * B;
        
        var_red += tmp_weight * ( R * R );
        var_green += tmp_weight * ( G * G );
        var_blue += tmp_weight * ( B * B );
      }
    }
    
    if (UW) {
      // In uniform weight case do the multiply outside the loop
      
      mean_red *= data_weight;
      mean_green *= data_weight;
      mean_blue *= data_weight;
      
      var_red *= data_weight;
      var_green *= data_weight;
      var_blue *= data_weight;
    }
    
    var_red -= SQR ( mean_red );
    var_green -= SQR ( mean_green );
    var_blue -= SQR ( mean_blue );

    // Copy data to user supplied pointers
    
    total_mean->red = mean_red;
    total_mean->green = mean_green;
    total_mean->blue = mean_blue;
    
    total_var->red = var_red;
    total_var->green = var_green;
    total_var->blue = var_blue;
    
#ifdef VERBOSE
    if ( 1 )
    {
      printf ( "new Global mean (R G B) (%0.2f %0.2f %0.2f)\n", total_mean->red, total_mean->green, total_mean->blue );
      printf ( "new Global var  (R G B) (%0.2f %0.2f %0.2f)\n", total_var->red, total_var->green, total_var->blue );
    }
#endif
  }
};

/* C: cluster to be split (old_index)*/
/* C1: subcluster #1 (old_index) */
/* C2: subcluster #2 (new_index) */

template <bool UW, typename MT, bool KM>
void
DivQuantCluster<UW, MT, KM>::cluster()
{
  int ic, ip, it;
  int max_iters_m1; /* MAX_ITERS - 1 */
  int tmp_num_points; /* number of points in C */
  int old_index; /* index of C or C1 */
  int new_index; /* index of C2 */
  int cut_axis; /* index of the cutting axis */
  int new_size = 0; /* size of C2 */
  int count;
  int shift_amount;
  int num_empty; /* # empty clusters */

#if defined(DEBUG)
  int member_size;
#endif // DEBUG
  int *point_index;
#if defined(DEBUG)
  int size_size;
#endif // DEBUG
  int *size;
  int apply_lkm; /* indicates whether or not LKM is to be applied */
  double max_val;
  double cut_pos; /* cutting position */
  double proj_val; /* projection of a data point on the cutting axis */
  double red, green, blue; /* R, G, B values of a particular pixel */
  double tmp_weight; /* weight of a particular pixel */
  double total_weight; /* weight of C */
  double old_weight; /* weight of C1 */
  double new_weight; /* weight of C2 */
  double lhs;
#ifdef VERBOSE
  double mse;
#endif
#if defined(DEBUG)
  int weight_size;
#endif // DEBUG
  double *weight; /* total weight of each cluster */
#if defined(DEBUG)
  int tse_size;
#endif // DEBUG
  double *tse; /* total squared error of each cluster */
#if defined(DEBUG)
  int mean_size;
#endif // DEBUG
  Pixel_Double *mean; /* componentwise mean (centroid) of each cluster */
#if defined(DEBUG)
  int var_size;
#endif // DEBUG
  Pixel_Double *var; /* componentwise variance of each cluster */
  Pixel_Double *old_mean; /* componentwise mean of C1 */
  Pixel_Double *new_mean; /* componentwise mean of C2 */
  Pixel_Double *old_var; /* componentwise variance of C1 */
  Pixel_Double *new_var; /* componentwise variance of C2 */

  // Capacity in num points that can be stored in tmp_data
  uint32_t *tmp_data; /* temporary data set (holds the cluster to be split) */
  int tmp_buffer_used;
  
  // Input parameter names
  
  const int num_points = this->numPoints;
  assert(num_points > 0);
  const uint32_t *data = this->inPixelsPtr;
  const uint32_t *tmp_buffer = this->tmpPixelsPtr;
  
  const double *dataWeights = this->weightsPtr;
  const double data_weight = this->weightUniform;
  if (dataWeights == nullptr) {
    assert(data_weight > 0.0);
  }
  
  if (UW) {
    assert(data_weight > 0.0);
  } else {
    assert(weightsPtr);
#if defined(DEBUG)
    assert(weightsPtr[0] > 0.0);
    assert(weightsPtr[num_points-1] > 0.0);
#endif // DEBUG
  }
  
  const int num_colors = this->numClusters;
  assert(num_colors > 0);
  const int num_bits = this->numBits;
  const int max_iters = this->maxNumIterations;
  
  apply_lkm = 0 < max_iters ? 1 : 0;
  max_iters_m1 = max_iters - 1;
  
  tmp_data = (uint32_t*) data;
  tmp_buffer_used = 0;
  
#if defined(VERBOSE)
  for ( ip = 0; ip < num_points; ip++ )
  {
    uint32_t pixel = data[ip];
    uint32_t B = pixel & 0xFF;
    uint32_t G = (pixel >> 8) & 0xFF;
    uint32_t R = (pixel >> 16) & 0xFF;
    
    red = R;
    green = G;
    blue = B;
    
    if (UW) {
      tmp_weight = data_weight;
    } else {
      assert(weightsPtr);
      tmp_weight = weightsPtr[ip];
    }
    
    fprintf(stdout, "in data[%5d] = ( %3d %3d %3d ) W = %8.8f\n", ip, R, G, B, tmp_weight);
  }
#endif // VERBOSE
  
  /* Contains point memberships (initially all points belong to cluster 0) */
#if defined(DEBUG)
  member_size = num_points;
#endif // DEBUG

  const bool is64Bit =
#if defined(__LP64__) && __LP64__
  true;
#else
  false;
#endif // __LP64__
  
  if (is64Bit && sizeof(MT) == 1) {
    // MT is a single byte and compiling for 64bit arch
    
    int numDoubleWords = num_points >> 3; // num_points / 8
    if ((num_points % 8) != 0) {
      numDoubleWords++;
    }
    uint64_t *ptr64 = new uint64_t[numDoubleWords]();
    member = (MT*) ptr64;
  } else {
    member = new MT[num_points]();
  }
  
  point_index = nullptr;
  
#if defined(DEBUG)
  weight_size = num_colors;
#endif // DEBUG
  weight = new double[num_colors]();
  
  /*
   * Contains the size of each cluster. The size of a cluster is
   * actually the number unique colors that it represents.
   */
#if defined(DEBUG)
  size_size = num_colors;
#endif // DEBUG
  size = new int[num_colors]();
  
#if defined(DEBUG)
  tse_size = num_colors;
#endif // DEBUG
  tse = new double[num_colors]();
  
#if defined(DEBUG)
  mean_size = num_colors;
#endif // DEBUG
  mean = new Pixel_Double[num_colors]();
  
#if defined(DEBUG)
  var_size = num_colors;
#endif // DEBUG
  var = new Pixel_Double[num_colors]();
  
  Pixel_Double *total_mean = &total_mean_prop;
  Pixel_Double *total_var = &total_var_prop;
  
  memset(total_mean, 0, sizeof(Pixel_Double));
  memset(total_var, 0, sizeof(Pixel_Double));
  
  /* Cluster 0 is always the first cluster to be split */
  old_index = 0;
  
  /* First cluster to be split contains the entire data set */
#if defined(DEBUG)
  assert(old_index >= 0 && old_index < weight_size);
#endif // DEBUG
  weight[old_index] = 1.0;
  
  /*
   * # points is not the same as # pixels. Each point represents
   * potentially multiple pixels with a specific color.
   */
#if defined(DEBUG)
  assert(old_index >= 0 && old_index < size_size);
#endif // DEBUG
  size[old_index] = tmp_num_points = num_points;
  
  /* Perform ( NUM_COLORS - 1 ) splits */
  /*
   OLD_INDEX denotes the index of the cluster to be split.
   When cluster OLD_INDEX is split, the indexes of the two subclusters
   are given by OLD_INDEX and NEW_INDEX, respectively.
   */
  for ( new_index = 1; new_index < num_colors; new_index++ )
  {
    /* STEPS 1 & 2: DETERMINE THE CUTTING AXIS AND POSITION */
    
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < weight_size);
#endif // DEBUG
    total_weight = weight[old_index];
    
    if ( new_index == 1 )
    {
      initMeanAndVar(total_mean, total_var);
    }
    else
    {
      // Cluster mean/variance has already been calculated
#if defined(DEBUG)
      assert(old_index >= 0 && old_index < mean_size);
#endif // DEBUG
      total_mean->red = mean[old_index].red;
      total_mean->green = mean[old_index].green;
      total_mean->blue = mean[old_index].blue;
      
#if defined(DEBUG)
      assert(old_index >= 0 && old_index < var_size);
#endif // DEBUG
      total_var->red = var[old_index].red;
      total_var->green = var[old_index].green;
      total_var->blue = var[old_index].blue;
      
#ifdef VERBOSE
      if ( 1 )
      {
        printf ( "Global mean (R G B) (%0.2f %0.2f %0.2f)\n", total_mean->red, total_mean->green, total_mean->blue );
        printf ( "Global var  (R G B) (%0.2f %0.2f %0.2f)\n", total_var->red, total_var->green, total_var->blue );
      }
#endif
      
    }
    
    /* Determine the axis with the greatest variance */
    
    max_val = total_var->red;
    cut_axis = 0;
    cut_pos = total_mean->red;
    
    if ( max_val < total_var->green )
    {
      max_val = total_var->green;
      cut_axis = 1;
      cut_pos = total_mean->green;
    }
    
    if ( max_val < total_var->blue )
    {
      cut_axis = 2;
      cut_pos = total_mean->blue;
    }
    
#if defined(DEBUG)
    assert(new_index >= 0 && new_index < mean_size);
    assert(new_index >= 0 && new_index < var_size);
#endif // DEBUG
    
    new_mean = &mean[new_index];
    new_var = &var[new_index];
    
#ifdef VERBOSE
    if ( cut_axis == 0 ) {
      printf ( "Cut on Red %10.8f\n", cut_pos);
    } else if ( cut_axis == 1 ) {
      printf ( "Cut on Green %10.8f\n", cut_pos);
    } else if ( cut_axis == 2 ) {
      printf ( "Cut on Blue %10.8f\n", cut_pos);
    } else {
      assert(0);
    }
#endif
    
    // Reset the statistics of the new cluster
    new_weight = 0.0;
    uint32_t new_weight_count = 0;
    RESET_PIXEL ( new_mean );
    
    if ( !KM && !apply_lkm )
    {
      new_size = 0;
      RESET_PIXEL ( new_var );
    }
    
    /* STEP 3: SPLIT THE CLUSTER OLD_INDEX */
    for ( ip = 0; ip < tmp_num_points; ip++ )
    {
      uint32_t pixel = tmp_data[ip];
      uint32_t B = pixel & 0xFF;
      uint32_t G = (pixel >> 8) & 0xFF;
      uint32_t R = (pixel >> 16) & 0xFF;
      
      red = R;
      green = G;
      blue = B;
      
#ifdef VERBOSE
      printf ( "pixel (R G B) (%3.2f %3.2f %3.2f)\n", red, green, blue );
#endif
      
      proj_val = ( ( cut_axis == 0 ) ? red :
                  ( ( cut_axis == 1 ) ? green : blue ) );
      
      // FIXME: if different loops for different cut axis, then that might
      // execute more quickly over the N points. Avoids 2 branches.
      // Also note that these compare operations are on floats. Also,
      // no reason to do 3 memory reads if only one will be used.
      
#ifdef VERBOSE
      printf ( "proj_val %8.2f and cut_pos %8.2f\n", proj_val, cut_pos);
#endif
      
      if ( cut_pos < proj_val )
      {
#ifdef VERBOSE
        printf ( "Cut GT   : %0.2f < %0.2f\n", cut_pos, proj_val);
#endif
        
        if (UW) {
          new_mean->red += red;
          new_mean->green += green;
          new_mean->blue += blue;
        } else {
          // non-uniform weights
          
          int pointindex = ip;
          if (point_index) {
            pointindex = point_index[ip];
          }
#if defined(DEBUG)
          assert(pointindex >= 0 && pointindex < member_size);
#endif // DEBUG
          tmp_weight = weightsPtr[pointindex];
          
          new_mean->red += tmp_weight * red;
          new_mean->green += tmp_weight * green;
          new_mean->blue += tmp_weight * blue;
        }
        
        // Update the point membership and variance/size of the new cluster
        if ( !KM && !apply_lkm )
        {
          int pointindex = ip;
          if (point_index) {
            pointindex = point_index[ip];
          }
#if defined(DEBUG)
          assert(pointindex >= 0 && pointindex < member_size);
#endif // DEBUG
          member[pointindex] = new_index;
#ifdef VERBOSE
          fprintf(stdout, "write member[%d] = %d (ip = %d)\n", pointindex, member[pointindex], ip);
#endif
          
          if (UW) {
            new_var->red += ( R * R );
            new_var->green += ( G * G );
            new_var->blue += ( B * B );
          } else {
            // non-uniform weights
            
            // tmp_weight already set above in loop
            
            new_var->red += tmp_weight * ( R * R );
            new_var->green += tmp_weight * ( G * G );
            new_var->blue += tmp_weight * ( B * B );
          }
          
          new_size++;
        }
        
        // Update the weight of the new cluster
        
        if (UW) {
          new_weight_count += 1;
        } else {
          new_weight += tmp_weight;
        }
      } else {
#ifdef VERBOSE
        printf ( "Cut LTEQ : %0.2f >= %0.2f\n", cut_pos, proj_val);
#endif
      }
    } // end split for loop
      
    if (UW) {
      new_mean->red *= data_weight;
      new_mean->green *= data_weight;
      new_mean->blue *= data_weight;
      
      new_weight = new_weight_count * data_weight;
      
      if ( !KM && !apply_lkm ) {
        new_var->red *= data_weight;
        new_var->green *= data_weight;
        new_var->blue *= data_weight;
      }
    }
    
    // Calculate the weight of the old cluster
    old_weight = total_weight - new_weight;
    
    // Calculate the mean of the new cluster
    new_mean->red /= new_weight;
    new_mean->green /= new_weight;
    new_mean->blue /= new_weight;
    
#ifdef VERBOSE
    printf ( "total_weight %8.2f : new_weight %8.2f  : old_weight %8.2f\n", total_weight, new_weight, old_weight);
#endif
    
#ifdef VERBOSE
    printf ( "New mean : R G B : %0.2f %0.2f %0.2f\n", new_mean->red, new_mean->green, new_mean->blue);
#endif
    
    /* Calculate the mean of the old cluster using the 'combined mean' formula */
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < mean_size);
#endif // DEBUG
    old_mean = &mean[old_index];
    old_mean->red = ( total_weight * total_mean->red - new_weight * new_mean->red ) / old_weight;
    old_mean->green = ( total_weight * total_mean->green - new_weight * new_mean->green ) / old_weight;
    old_mean->blue = ( total_weight * total_mean->blue - new_weight * new_mean->blue ) / old_weight;
    
#ifdef VERBOSE
    printf ( "Old mean : R G B : %0.2f %0.2f %0.2f\n", old_mean->red, old_mean->green, old_mean->blue);
#endif
    
    /* LOCAL K-MEANS BEGIN */
    
#ifdef VERBOSE
    if ( apply_lkm )
    {
      printf ( "Global Iteration %d\n", new_index - 1 );
    }
#endif
    
    for ( it = 0; it < max_iters; it++ )
    {
      // Precalculations
      lhs = 0.5 *
      ( SQR ( old_mean->red ) - SQR ( new_mean->red ) +
       SQR ( old_mean->green ) - SQR ( new_mean->green ) +
       SQR ( old_mean->blue ) - SQR ( new_mean->blue ) );
      
      double rhs_red = old_mean->red - new_mean->red;
      double rhs_green = old_mean->green - new_mean->green;
      double rhs_blue = old_mean->blue - new_mean->blue;
      
      // Reset the statistics of the new cluster
      new_weight = 0.0;
      new_size = 0;
      RESET_PIXEL ( new_mean );
      RESET_PIXEL ( new_var );
      
#ifdef VERBOSE
      mse = 0.0;
#endif
      
#ifdef VERBOSE
      printf ( "Local kmeans Iteration %d\n", it );
#endif
      
      for ( ip = 0; ip < tmp_num_points; )
      {
        int maxLoopOffset = 0xFFFF;
        int numLeft = (tmp_num_points - ip);
        if (numLeft < maxLoopOffset) {
          maxLoopOffset = numLeft;
        }
        maxLoopOffset += ip;
        
        uint32_t new_mean_red = 0;
        uint32_t new_mean_green = 0;
        uint32_t new_mean_blue = 0;
        
        uint32_t new_var_red = 0;
        uint32_t new_var_green = 0;
        uint32_t new_var_blue = 0;
        
        for ( ; ip < maxLoopOffset; ip++ ) {
          
          uint32_t pixel = tmp_data[ip];
          uint32_t B = pixel & 0xFF;
          uint32_t G = (pixel >> 8) & 0xFF;
          uint32_t R = (pixel >> 16) & 0xFF;
          
          red = R;
          green = G;
          blue = B;
          
          int pointindex = ip;
          if (point_index) {
            pointindex = point_index[ip];
          }
#if defined(DEBUG)
          assert(pointindex >= 0 && pointindex < num_points);
          assert(pointindex >= 0 && pointindex < member_size);
#endif // DEBUG
          if (UW) {
#ifdef VERBOSE
            tmp_weight = data_weight;
#endif // VERBOSE
          } else {
            tmp_weight = weightsPtr[pointindex];
          }
          
          if ( lhs < ( (rhs_red * red) + (rhs_green * green) + (rhs_blue * blue) ) )
          {
#ifdef VERBOSE
            // Update the MSE of the old cluster
            mse += tmp_weight *
            ( SQR ( red - old_mean->red ) +
             SQR ( green - old_mean->green ) +
             SQR ( blue - old_mean->blue ) );
#endif
            
            if ( it == max_iters_m1 )
            {
              // Save the membership of the point
              member[pointindex] = old_index;
#ifdef VERBOSE
              fprintf(stdout, "write member[%d] = %d (ip = %d)\n", pointindex, member[pointindex], ip);
#endif
            }
          }
          else
          {
#ifdef VERBOSE
            // Update the MSE of the new cluster
            mse += tmp_weight *
            ( SQR ( red - old_mean->red + rhs_red ) +
             SQR ( green - old_mean->green + rhs_green ) +
             SQR ( blue - old_mean->blue + rhs_blue ) );
#endif
            
            if ( it != max_iters_m1 )
            {
              // Update only mean
              
              if (UW) {
                new_mean_red += R;
                new_mean_green += G;
                new_mean_blue += B;
              } else {
                new_mean->red += tmp_weight * red;
                new_mean->green += tmp_weight * green;
                new_mean->blue += tmp_weight * blue;
              }
            }
            else
            {
              // Update mean and variance
              
              if (UW) {
                new_mean_red += R;
                new_mean_green += G;
                new_mean_blue += B;
              } else {
                new_mean->red += tmp_weight * red;
                new_mean->green += tmp_weight * green;
                new_mean->blue += tmp_weight * blue;
              }
              
              if (UW) {
                new_var_red += ( R * R );
                new_var_green += ( G * G );
                new_var_blue += ( B * B );
              } else {
                new_var->red += tmp_weight * ( R * R );
                new_var->green += tmp_weight * ( G * G );
                new_var->blue += tmp_weight * ( B * B );
              }
              
              // Save the membership of the point
              member[pointindex] = new_index;
#ifdef VERBOSE
              fprintf(stdout, "write member[%d] = %d (ip = %d)\n", pointindex, member[pointindex], ip);
#endif
            }
            
            // Update the weight/size of the new cluster
            
            if (UW) {
            } else {
              new_weight += tmp_weight;
            }
            new_size++;
          }
        } // end foreach tmp_num_points inner loop
        
        if (UW) {
          new_mean->red += new_mean_red;
          new_mean->green += new_mean_green;
          new_mean->blue += new_mean_blue;
          
          new_var->red += new_var_red;
          new_var->green += new_var_green;
          new_var->blue += new_var_blue;
        }
        
      } // end foreach tmp_num_points outer loop
      
#ifdef VERBOSE
      printf ( "\tLocal Iteration %d: MSE = %f\n", it, mse );
      if ( it == max_iters_m1 )
      {
        printf ( "\n" );
      }
#endif
      
      if (UW) {
        new_mean->red *= data_weight;
        new_mean->green *= data_weight;
        new_mean->blue *= data_weight;
        
        new_weight = new_size * data_weight;
        
        new_var->red *= data_weight;
        new_var->green *= data_weight;
        new_var->blue *= data_weight;
      }
      
      // Calculate the mean of the new cluster
      new_mean->red /= new_weight;
      new_mean->green /= new_weight;
      new_mean->blue /= new_weight;
      
      // Calculate the weight of the old cluster
      old_weight = total_weight - new_weight;
      
      // Calculate the mean of the old cluster using the 'combined mean' formula
      old_mean->red = ( total_weight * total_mean->red - new_weight * new_mean->red ) / old_weight;
      old_mean->green = ( total_weight * total_mean->green - new_weight * new_mean->green ) / old_weight;
      old_mean->blue = ( total_weight * total_mean->blue - new_weight * new_mean->blue ) / old_weight;
    }
    
    /* LOCAL K-MEANS END */
    
    /* Store the updated cluster sizes */
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < size_size);
    assert(new_index >= 0 && new_index < size_size);
#endif // DEBUG
    size[old_index] = tmp_num_points - new_size;
    size[new_index] = new_size;
    
    if ( new_index == num_colors - 1 )
    {
#ifdef VERBOSE
      printf ( "\tLast Iteration\n" );
#endif
      
      /* This is the last iteration. So, there is no need to
       determine the cluster to be split in the next iteration. */
      break;
    }
    
    /* Calculate the variance of the new cluster */
    /* Alternative weighted variance formula: ( sum{w_i * x_i^2} / sum{w_i} ) - bar{x}^2 */
    new_var->red = new_var->red / new_weight - SQR ( new_mean->red );
    new_var->green = new_var->green / new_weight - SQR ( new_mean->green );
    new_var->blue = new_var->blue / new_weight - SQR ( new_mean->blue );
    
    /* Calculate the variance of the old cluster using the 'combined variance' formula */
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < var_size);
#endif // DEBUG
    old_var = &var[old_index];
    old_var->red = ( ( total_weight * total_var->red -
                      new_weight * ( new_var->red + SQR ( new_mean->red - total_mean->red ) ) ) / old_weight ) -
    SQR ( old_mean->red - total_mean->red );
    
    old_var->green = ( ( total_weight * total_var->green -
                        new_weight * ( new_var->green + SQR ( new_mean->green - total_mean->green ) ) ) / old_weight ) -
    SQR ( old_mean->green - total_mean->green );
    
    old_var->blue = ( ( total_weight * total_var->blue -
                       new_weight * ( new_var->blue + SQR ( new_mean->blue - total_mean->blue ) ) ) / old_weight ) -
    SQR ( old_mean->blue - total_mean->blue );
    
    /* Store the updated cluster weights */
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < weight_size);
    assert(new_index >= 0 && new_index < weight_size);
#endif // DEBUG
    weight[old_index] = old_weight;
    weight[new_index] = new_weight;
    
    /* Store the cluster TSEs */
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < tse_size);
    assert(new_index >= 0 && new_index < tse_size);
#endif // DEBUG
    tse[old_index] = old_weight * ( old_var->red + old_var->green + old_var->blue );
    tse[new_index] = new_weight * ( new_var->red + new_var->green + new_var->blue );
    
    /* STEP 4: DETERMINE THE NEXT CLUSTER TO BE SPLIT */
    
    /* Split the cluster with the maximum TSE */
    max_val = DBL_MIN;
    for ( ic = 0; ic <= new_index; ic++ )
    {
#if defined(DEBUG)
      assert(ic >= 0 && ic < tse_size);
#endif // DEBUG
      if ( max_val < tse[ic] )
      {
        max_val = tse[ic];
        old_index = ic;
      }
    }
    
#if defined(DEBUG)
    assert(old_index >= 0 && old_index < size_size);
#endif // DEBUG
    tmp_num_points = size[old_index];
    
    // Allocate tmp_data and point_index only after initial division and then reuse buffers
    
    if (tmp_buffer_used == 0) {
      // When the initial input points are first split into 2 clusters, allocate tmp_data
      // as a buffer large enough to hold the largest of the 2 initial clusters. This
      // buffer is significantly smaller than the original input size and it can be
      // reused for all smaller cluster sizes.
      
#if defined(DEBUG)
      assert(tmp_data == data);
      assert(point_index == nullptr);
#endif // DEBUG
      
      int largerSize = size[0];
      if ((num_colors > 1) && (size[1] > largerSize)) {
        largerSize = size[1];
      }
      
      tmp_data = (uint32_t*) tmp_buffer;
#if defined(DEBUG)
      memset(tmp_data, 0, largerSize * sizeof ( uint32_t ));
#endif // DEBUG
      
      tmp_buffer_used = largerSize;
      
      // alloc and init to zero
      point_index = new int[largerSize]();
    } else {
#if defined(DEBUG)
      assert(tmp_data == tmp_buffer);
      assert(point_index != nullptr);
      assert(tmp_buffer_used >= tmp_num_points);
#endif // DEBUG
    }
    
    // Setup the points and their indexes in the next cluster to be split
    
    count = 0;
    ip = 0;
    
    if (is64Bit && sizeof(MT) == 1) {
      // Read 8 single byte values at a time from member array
      
      uint64_t *member64 = (uint64_t*) member;
      int numDoubleWordLoops = num_points >> 3; // num_points / 8
      
      for ( int i = 0; i < numDoubleWordLoops; i++) {
        uint64_t dword = member64[i];
        
        for ( uint8_t bytei = 0; bytei < 8; bytei++ ) {
          uint32_t shiftright = (bytei << 3); // bytei * 8
          uint8_t memberVal = (dword >> shiftright) & 0xFF;
#ifdef VERBOSE
          fprintf(stdout, "member[%d] = %d\n", ip, memberVal);
#endif
#if defined(DEBUG)
          assert(ip >= 0 && ip < member_size);
#endif // DEBUG
          
          if ( memberVal == old_index ) {
            uint32_t pixel = data[ip];
            
#ifdef VERBOSE
            uint32_t B = pixel & 0xFF;
            uint32_t G = (pixel >> 8) & 0xFF;
            uint32_t R = (pixel >> 16) & 0xFF;
            
            if (UW) {
              tmp_weight = data_weight;
            } else {
              assert(weightsPtr);
              tmp_weight = weightsPtr[ip];
            }
            
            fprintf(stdout, "in (copy) data[%5d] = ( %3d %3d %3d ) W = %8.8f\n", ip, R, G, B, tmp_weight);
#endif
            
            tmp_data[count] = pixel;
            point_index[count] = ip;
            count++;
          }
          
          ip += 1;
        }
      }
      
#if defined(DEBUG)
      // Remaining num loops is LT 8
      assert((num_points - ip) < 8);
#endif // DEBUG

      // In the case where ((num_points - ip) > 0) the loop below
      // will be executed to copy any remaining values.
    }
    
    // Read 1 to N values from member array one at a time
      
    for ( ; ip < num_points; ip++ )
    {
#if defined(DEBUG)
      assert(ip >= 0 && ip < member_size);
#endif // DEBUG
      if ( member[ip] == old_index )
      {
        uint32_t pixel = data[ip];
        
#ifdef VERBOSE
        uint32_t B = pixel & 0xFF;
        uint32_t G = (pixel >> 8) & 0xFF;
        uint32_t R = (pixel >> 16) & 0xFF;
        
        if (UW) {
          tmp_weight = data_weight;
        } else {
          assert(weightsPtr);
          tmp_weight = weightsPtr[ip];
        }
        
        fprintf(stdout, "in (copy) data[%5d] = ( %3d %3d %3d ) W = %8.8f\n", ip, R, G, B, tmp_weight);
#endif
        
        tmp_data[count] = pixel;
        point_index[count] = ip;
        count++;
      }
    }
    
    if ( count != tmp_num_points )
    {
      fprintf ( stderr, "Cluster to be split is expected to be of size %d not %d !\n",
               tmp_num_points, count );
      abort ( );
    }
  }
  
  /* Determine the final cluster centers */
  shift_amount = 8 - num_bits;
  num_empty = 0;
  for ( ic = 0; ic < num_colors; ic++ )
  {    
#if defined(DEBUG)
    assert(ic >= 0 && ic < size_size);
#endif // DEBUG
    
    if ( 0 < size[ic] )
    {
#if defined(DEBUG)
      assert(ic >= 0 && ic < mean_size);
      assert(ic >= 0 && ic < var_size);
#endif // DEBUG
      
#ifdef VERBOSE
      printf ( "\t%d : Round and shift ( %0.2f %0.2f %0.2f ) \n", ic, mean[ic].red, mean[ic].green, mean[ic].blue);
#endif
      
      uint32_t R = ( ( uint8_t ) ( mean[ic].red + 0.5 ) ) << shift_amount; /* round */
      uint32_t G = ( ( uint8_t ) ( mean[ic].green + 0.5 ) ) << shift_amount; /* round */
      uint32_t B = ( ( uint8_t ) ( mean[ic].blue + 0.5 ) ) << shift_amount; /* round */
      uint32_t pixel = (R << 16) | (G << 8) | B;
      colortable.push_back(pixel);
      
#ifdef VERBOSE
      printf ( "\t%d : Rounded ( %d %d %d ) \n", ic, R, G, B);
#endif
    }
    else 
    {
      /* Empty cluster */
      num_empty++;
    }
  }
  
  if ( num_empty )
  {
    fprintf ( stderr, "# empty clusters: %d\n", num_empty );
  }
  
#ifdef VERBOSE
  for ( int ip = 0; ip < num_points; ip++) {
#if defined(DEBUG)
    assert(ip >= 0 && ip < member_size);
#endif // DEBUG
    int offset = member[ip];
    fprintf ( stdout, "member[%4d] = %d\n", ip, offset );
  }
#endif
  
  
  if (tmp_buffer_used > 0) {
    delete [] point_index;
  }
  delete [] member;
  delete [] weight;
  delete [] size;
  delete [] tse;
  delete [] mean;
  delete [] var;
  
  this->numClusters = num_colors - num_empty;
  assert(this->numClusters == (uint32_t)colortable.size());
}

std::vector<uint32_t>
quant_varpart_fast (
                    const uint32_t numPixels,
                    const uint32_t *inPixels,
                    uint32_t *tmpPixels,
                    const uint32_t numRows,
                    const uint32_t numCols,
                    int num_colors,
                    const int num_bits,
                    const int dec_factor,
                    const int max_iters,
                    const bool allPixelsUnique)
{
  int num_points;
  
  if ( !validate_num_bits ( num_bits ) )
  {
    assert(0);
  }
  
  num_points = numPixels;

  bool inputPixelsAllocated = false;
  uint32_t *inputPixels = (uint32_t*) inPixels;
  
  double weightUniform = 0.0;
  double *weightsPtr = nullptr;
  
  if ((allPixelsUnique && (num_bits == 8 && dec_factor == 1) && 1)) {
    // No duplicate pixels and no decimation or bit shifting
    weightUniform = get_double_scale(inPixels, numPixels);
  } else if (!allPixelsUnique && num_bits == 8) {
    // No cut bits, but duplicate pixels, dedup now
    weightsPtr = calc_color_table(inPixels, numPixels, tmpPixels, numRows, numCols, dec_factor, &num_points);
    inputPixelsAllocated = true;
    inputPixels = new uint32_t[num_points];
    memcpy(inputPixels, tmpPixels, num_points * sizeof(uint32_t));
  } else {
    // cut bits with right shift and dedup to generate significantly smaller sized buffer
    cut_bits(inPixels, numPixels, tmpPixels, num_bits, num_bits, num_bits);
    weightsPtr = calc_color_table(tmpPixels, numPixels, tmpPixels, numRows, numCols, dec_factor, &num_points);
    inputPixelsAllocated = true;
    inputPixels = new uint32_t[num_points];
    memcpy(inputPixels, tmpPixels, num_points * sizeof(uint32_t));
  }
  
  vector<uint32_t> vec;
  
  if (weightsPtr == nullptr) {
    // Uniform weight
    
    if (num_colors <= 256) {
      // Uniform weight and each cluster int fits in one byte
      
      DivQuantCluster<true, uint8_t, true> dqCluster;
      
      dqCluster.numPoints = num_points;
      dqCluster.numClusters = num_colors;
      
      dqCluster.inPixelsPtr = inputPixels;
      dqCluster.tmpPixelsPtr = tmpPixels;
      
      dqCluster.weightsPtr = weightsPtr;
      dqCluster.weightUniform = weightUniform;
      
      dqCluster.maxNumIterations = max_iters;
      dqCluster.numBits = num_bits;
      
      dqCluster.cluster();
      
      vec = dqCluster.colortable;
    } else {
      // Uniform weight where each cluster fits in a word

      DivQuantCluster<true, uint32_t, true> dqCluster;
      
      dqCluster.numPoints = num_points;
      dqCluster.numClusters = num_colors;
      
      dqCluster.inPixelsPtr = inputPixels;
      dqCluster.tmpPixelsPtr = tmpPixels;
      
      dqCluster.weightsPtr = weightsPtr;
      dqCluster.weightUniform = weightUniform;
      
      dqCluster.maxNumIterations = max_iters;
      dqCluster.numBits = num_bits;
      
      dqCluster.cluster();
      
      vec = dqCluster.colortable;
    }
  } else {
    // Non-uniform weights (num clusters unrestrained)
    
    if (num_colors <= 256) {
      DivQuantCluster<false, uint8_t, true> dqCluster;
      
      dqCluster.numPoints = num_points;
      dqCluster.numClusters = num_colors;
      
      dqCluster.inPixelsPtr = inputPixels;
      dqCluster.tmpPixelsPtr = tmpPixels;
      
      dqCluster.weightsPtr = weightsPtr;
      dqCluster.weightUniform = weightUniform;
      
      dqCluster.maxNumIterations = max_iters;
      dqCluster.numBits = num_bits;
      
      dqCluster.cluster();
      
      vec = dqCluster.colortable;
    } else {
      DivQuantCluster<false, uint32_t, true> dqCluster;
      
      dqCluster.numPoints = num_points;
      dqCluster.numClusters = num_colors;
      
      dqCluster.inPixelsPtr = inputPixels;
      dqCluster.tmpPixelsPtr = tmpPixels;
      
      dqCluster.weightsPtr = weightsPtr;
      dqCluster.weightUniform = weightUniform;
      
      dqCluster.maxNumIterations = max_iters;
      dqCluster.numBits = num_bits;
      
      dqCluster.cluster();
      
      vec = dqCluster.colortable;
    }
  }
  
  if (weightsPtr != nullptr) {
    delete [] weightsPtr;
  }
  
  if (inputPixelsAllocated) {
    delete [] inputPixels;
  }
  
  return vec;
}

