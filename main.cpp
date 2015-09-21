#include "PngContext.h"

void process_file(PngContext *cxt)
{
  // Swap the B and R channels in the BGRA format pixels
  
  int numPixels = cxt->width * cxt->height;
  
  for ( int i = 0; i < numPixels; i++) {
    uint32_t pixel = cxt->pixels[i];
    uint32_t B = pixel & 0xFF;
    uint32_t G = (pixel >> 8) & 0xFF;
    uint32_t R = (pixel >> 16) & 0xFF;
    uint32_t A = (pixel >> 24) & 0xFF;
    
    // Swap B and R channels, a grayscale image will not be modified
    
    if ((1)) {
      uint32_t tmp = B;
      B = R;
      R = tmp;
    }
    
    uint32_t outPixel = (A << 24) | (R << 16) | (G << 8) | B;
    
    cxt->pixels[i] = outPixel;
  }
}

/* deallocate memory */

void cleanup(PngContext *cxt)
{
  free_row_pointers(cxt);
  
  free(cxt->pixels);
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    abort_("Usage: %s <in_png> <out_png>", argv[0]);
  }
  
  PngContext cxt;
  read_png_file(argv[1], &cxt);
  process_file(&cxt);
  write_png_file(argv[2], &cxt);
  
  printf("success processing %d pixels from image of dimensions %d x %d\n", cxt.width*cxt.height, cxt.width, cxt.height);
  
  cleanup(&cxt);
  
  return 0;
}
