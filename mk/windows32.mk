CXX = i686-w64-mingw32-g++
CC = i686-w64-mingw32-gcc
LIBS = -lboost_system-mt -lcairo -lfreetype -lvncclient -lnettle -lgnutls -lgmp -lmswsock -lws2_32 -lp11-kit.dll -lharfbuzz -lgraphite2 -ldwrite -lrpcrt4 -lintl -liconv -lbz2 -lz -luser32 -lgdi32 -lpng -lz -luuid -lturbojpeg -lpixman-1 -ldl -Lcvmlib32/lib -lodb-sqlite -lodb -lsqlite3 -s
OBJDIR = obj/windows32/
BINDIR = bin/windows32
.PHONY: all pre clean
.SUFFIXES: .o .cpp

-include mk/common.mk
CCFLAGS += -Icvmlib32/include
CXXFLAGS += -Icvmlib32/include
CXXGUAC += -Icvmlib32/include
