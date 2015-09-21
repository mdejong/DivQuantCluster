# DivQuantCluster
Clustering using divisive split and k-means

This clustering approach was originally implemented by M. Emre Celebi. The clustering implementation
provides an exceptionally fast clustering algorithm in C++ that is optimized speed and low memory
embedded devices.

The main.cpp logic demonstrates a specific application of clustering for image pixels. Each pixel is
clustered in 3D space to produce a color sorted list of clusters and then the original pixels are
reordered in terms of the sorted cluster center order.

For example, the very large 4K (4096 x 2160 pixels) image at:

http://andyyoong.com/fs700-4k-raw-test/

This image can be processed in about 10 seconds with a reasonably fast quant methods like Gvm or pngquant.

This library is able to process the image in about 1/2 a second on the desktop.
