
OBJS = \
        7zlzma.o        \
        alloc.o         \
        AriBitCoder.o   \
        decompress.o    \
        IInOutStreams.o \
        InByte.o        \
        LenCoder.o      \
        LiteralCoder.o  \
        LZMA.o          \
        LZMADecoder.o   \
        LZMAEncoder.o   \
        minilzo.o       \
        n2b_99.o        \
        n2d_99.o        \
        nrv2d_d.o       \
        OutByte.o       \
        ProgramStore.o  \
        StdAfx.o        \
        WindowIn.o      \
        WindowOut.o

CPPFLAGS += -DGNU -I7z -fpermissive -m32

VPATH += 7z decompress


ifeq ($(MINGW),1)

    # COMPILE.cpp += -B C:/mingw/lib/gcc-lib/ -I c:/mingw/include -I 7z

    CC  = gcc -B C:/mingw/lib/gcc-lib/ -mconsole
    CXX = g++ -B C:/mingw/lib/gcc-lib/ -mconsole
    CPPFLAGS += -I c:/mingw/include
    LDFLAGS  += -Lc:/mingw/lib -LC:/mingw/lib/gcc-lib/mingw32/2.95.3-4

all: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o ProgramStore

else

    CPPFLAGS += -m32 -static -O2

all: $(OBJS)
	$(CXX) $(OBJS) $(CPPFLAGS) -o ProgramStore -Wl,-Map -Wl,ps.map

endif

clean:
	rm -f *.o ProgramStore

