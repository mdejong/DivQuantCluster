//
//  DivQuantTest.m
//  DivQuantTest
//
//  Created by Mo DeJong on 9/21/15.
//  Copyright (c) 2015 helpurock. All rights reserved.
//
//  This test module does a basic sanity check of the quant_recurse() method

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#include "quant_util.h"

@interface DivQuantTest : XCTestCase

@end

@implementation DivQuantTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testQuantN1 {
  // 10 grayscale values
  
  int min = 0;
  int max = 0xFF;
  int step = (max - min) / 10;
  
  uint32_t pixels[10];
  
  for ( int i = 0; i < 10; i++ ) {
    uint32_t gray = (i * step);
    uint32_t grayPixel = (gray << 16) | (gray << 8) | gray;
    pixels[i] = grayPixel;
    //printf("pixels[%d] = 0x%08X\n", i, pixels[i]);
  }
  
  const int numPixels = 10;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 1;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == 1, @"colortable");
  XCTAssert(colortable[0] == 0x00000000, @"colortable");
  
  return;
}

- (void)testQuantN2 {
  // 10 grayscale values
  
  int min = 0;
  int max = 0xFF;
  int step = (max - min) / 10;
  
  uint32_t pixels[10];
  
  for ( int i = 0; i < 10; i++ ) {
    uint32_t gray = (i * step);
    uint32_t grayPixel = (gray << 16) | (gray << 8) | gray;
    pixels[i] = grayPixel;
    //printf("pixels[%d] = 0x%08X\n", i, pixels[i]);
  }
  
  const int numPixels = 10;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 2;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == 2, @"colortable");
  
  XCTAssert(colortable[0] == 0x00323232, @"colortable");
  XCTAssert(colortable[1] == 0x00AFAFAF, @"colortable");
  
  return;
}

- (void)testQuantN3 {
  // 10 grayscale values
  
  int min = 0;
  int max = 0xFF;
  int step = (max - min) / 10;
  
  uint32_t pixels[10];
  
  for ( int i = 0; i < 10; i++ ) {
    uint32_t gray = (i * step);
    uint32_t grayPixel = (gray << 16) | (gray << 8) | gray;
    pixels[i] = grayPixel;
    
    if ((0)) {
      printf("pixels[%d] = 0x%08X\n", i, pixels[i]);
    }
  }
  
  const int numPixels = 10;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 3;
  uint32_t colortable[numClusters];

  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == 3, @"colortable");
  
  XCTAssert(colortable[0] == 0x00191919, @"colortable");
  XCTAssert(colortable[1] == 0x00AFAFAF, @"colortable");
  XCTAssert(colortable[2] == 0x00585858, @"colortable");
  
  return;
}

// Quant 2 color pixels with num clusters = 3, this should reduce the
// number of output clusters to 2 since not all table values used.

- (void)testQuantN3N2 {
  uint32_t pixels[2];
  
  pixels[0]  = 0x008DC63F; // Green
  pixels[1]  = 0x00F26522; // Orange
  
  if ((1)) {
    for ( int i = 0; i < 2; i++ ) {
      printf("pixels[%d] = 0x%08X\n", i, pixels[i]);
    }
  }
  
  const int numPixels = 2;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 3;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == 2, @"colortable");
  
  XCTAssert(colortable[0] == 0x00F26522, @"colortable");
  XCTAssert(colortable[1] == 0x008DC63F, @"colortable");
  
  return;
}

// 2 gray pixels on either side of the middle between 0x0 and 0x00FFFFFF

- (void)testQuantGray2 {
  uint32_t pixels[2];
  
  pixels[0]  = 0x000A0A0A; // 10
  pixels[1]  = 0x00F5F5F5; // 255-10
  
  if ((0)) {
    for ( int i = 0; i < 2; i++ ) {
      printf("pixels[%d] = 0x%08X\n", i, pixels[i]);
    }
  }
  
  const int numPixels = 2;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 2;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == 2, @"colortable");
  
  XCTAssert(colortable[0] == 0x000A0A0A, @"colortable");
  XCTAssert(colortable[1] == 0x00F5F5F5, @"colortable");
  
  return;
}

// Color quant 16 (4x4 pixels) from color umbrella, in this case
// 16 colors are processed into 4 clusters

- (void) testQuant_4x4_N4 {
  uint32_t pixels[16];
  
  pixels[0]  = 0x00EBC58B;
  pixels[1]  = 0x00DAD4E7;
  pixels[2]  = 0x00D7779D;
  pixels[3]  = 0x007E393D;
  pixels[4]  = 0x00ABA4BA;
  pixels[5]  = 0x00CF4B53;
  pixels[6]  = 0x00C49AC7;
  pixels[7]  = 0x00AC7292;
  pixels[8]  = 0x00ECEFE7;
  pixels[9]  = 0x00DC789D;
  pixels[10] = 0x00A8ABC4;
  pixels[11] = 0x00906E9E;
  pixels[12] = 0x00B54748;
  pixels[13] = 0x00A24F44;
  pixels[14] = 0x00857E77;
  pixels[15] = 0x007F654B;
  
  const int numPixels = 16;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 4;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == numClusters, @"colortable");
  XCTAssert(colortable[0] == 0x00A14D48, @"colortable");
  XCTAssert(colortable[1] == 0x00C292B3, @"colortable");
  XCTAssert(colortable[2] == 0x00E6D8C8, @"colortable");
  XCTAssert(colortable[3] == 0x0096758D, @"colortable");
  
  return;
}

// Color quant 16 (4x4 pixels) from color umbrella, in this case
// 16 colors are processed into 16 clusters so 1 pixel for each cluster

- (void) testQuant_4x4_N16 {
  uint32_t pixels[16];
  
  pixels[0]  = 0x00EBC58B;
  pixels[1]  = 0x00DAD4E7;
  pixels[2]  = 0x00D7779D;
  pixels[3]  = 0x007E393D;
  pixels[4]  = 0x00ABA4BA;
  pixels[5]  = 0x00CF4B53;
  pixels[6]  = 0x00C49AC7;
  pixels[7]  = 0x00AC7292;
  pixels[8]  = 0x00ECEFE7;
  pixels[9]  = 0x00DC789D;
  pixels[10] = 0x00A8ABC4;
  pixels[11] = 0x00906E9E;
  pixels[12] = 0x00B54748;
  pixels[13] = 0x00A24F44;
  pixels[14] = 0x00857E77;
  pixels[15] = 0x007F654B;
  
  const int numPixels = 16;
  uint32_t *inPixels = pixels;
  uint32_t outPixels[numPixels];
  
  const int numClusters = 16;
  uint32_t colortable[numClusters];
  
  int allPixelsUnique = 1;
  
  uint32_t numActualClusters = numClusters;
  
  quant_recurse(numPixels, inPixels, outPixels, &numActualClusters, colortable, allPixelsUnique );
  
  XCTAssert(numActualClusters == numClusters, @"colortable");
  
  XCTAssert(colortable[0] == 0x007E393D, @"colortable");
  XCTAssert(colortable[1] == 0x00D7779D, @"colortable");
  XCTAssert(colortable[2] == 0x00EBC58B, @"colortable");
  XCTAssert(colortable[3] == 0x00857E77, @"colortable");
  
  XCTAssert(colortable[4] == 0x00DAD4E7, @"colortable");
  XCTAssert(colortable[5] == 0x00ABA4BA, @"colortable");
  XCTAssert(colortable[6] == 0x00A24F44, @"colortable");
  XCTAssert(colortable[7] == 0x00AC7292, @"colortable");
  
  XCTAssert(colortable[8] == 0x00CF4B53, @"colortable");
  XCTAssert(colortable[9] == 0x007F654B, @"colortable");
  XCTAssert(colortable[10] == 0x00906E9E, @"colortable");
  XCTAssert(colortable[11] == 0x00C49AC7, @"colortable");
  
  XCTAssert(colortable[12] == 0x00ECEFE7, @"colortable");
  XCTAssert(colortable[13] == 0x00B54748, @"colortable");
  XCTAssert(colortable[14] == 0x00A8ABC4, @"colortable");
  XCTAssert(colortable[15] == 0x00DC789D, @"colortable");
  
  return;
}

@end
