# MacOSX will typically alias these as gcc and g++
#CC=clang
#CXX=clang++

CC=gcc
CXX=g++

#CFLAGS=-g
CFLAGS=-O3

# Note that g++ may require -std=c++11 here
CXXFLAGS=$(CFLAGS) -std=gnu++11 -stdlib=libc++

INC_FLAGS=-Ilibpng -IDivQuant

# Assumes zlib is available as a system library in std location
LIBS=-lz

LIBPNG_OBJS=\
libpng/png.o \
libpng/pngerror.o \
libpng/pngget.o \
libpng/pngmem.o \
libpng/pngpread.o \
libpng/pngread.o \
libpng/pngrio.o \
libpng/pngrtran.o \
libpng/pngrutil.o \
libpng/pngset.o \
libpng/pngtrans.o \
libpng/pngwio.o \
libpng/pngwrite.o \
libpng/pngwtran.o \
libpng/pngwutil.o

DIVQUANT_OBJS=\
DivQuant/DivQuantCluster.o \
DivQuant/DivQuantMapColors.o \
DivQuant/DivQuantMisc.o \
DivQuant/DivQuantUni.o \
DivQuant/quant_util.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

main: $(LIBPNG_OBJS) $(DIVQUANT_OBJS) main.cpp
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o DivQuantCluster main.cpp $(LIBPNG_OBJS) $(DIVQUANT_OBJS) $(LIBS)

all: main

clean:
	rm -f $(LIBPNG_OBJS) $(DIVQUANT_OBJS)
