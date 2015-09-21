// This header can be included into either C++ or C code.

#ifndef quant_util_h
#define quant_util_h

#ifdef __cplusplus
extern "C" {
#endif
    
  void quant_recurse ( uint32_t numPixels, const uint32_t *inPixelsPtr, uint32_t *outColorTableOffsetPtr, uint32_t *numClustersPtr, uint32_t *outColortablePtr );

#ifdef __cplusplus
}
#endif

#endif // quant_util_h