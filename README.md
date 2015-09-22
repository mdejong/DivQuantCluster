# DivQuantCluster
Clustering using divisive split and k-means

This clustering approach was originally implemented by M. Emre Celebi. The clustering implementation
provides an exceptionally fast clustering algorithm in C++ that is optimized for execution speed
and for low memory use on embedded devices.

The main.cpp logic demonstrates a specific application of clustering for image pixels. Each pixel is
clustered in 3D space to produce a color sorted list of clusters and then the original pixels are
reordered in terms of the sorted cluster center order.

For example, the very large 4K (4096 x 2160 pixels) image at:

http://andyyoong.com/fs700-4k-raw-test/

On a desktop system, the 4k image above can be processed in about 10 seconds with
a reasonably fast quant method like Gvm or pngquant.

https://pngquant.org/

https://github.com/mdejong/GvmCpp

This library is able to process the image into 256 clusters in about 1/2 a second while
using less than 10 megs of memory.

M. E. Celebi, Q. Wen, S. Hwang, An Effective Real-Time Color Quantization Method Based on Divisive Hierarchical Clustering, Journal of Real-Time Image Processing, 10(2): 329-344, 2015

http://link.springer.com/article/10.1007%2Fs11554-012-0291-4
