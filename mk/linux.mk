CXX = g++
CC = gcc
LIBS = -static-libstdc++ -lpthread -lboost_system -lpng -lz -lossp-uuid -lvncclient -lturbojpeg -lcairo -lpixman-1 -ldl -Lcvmlib_lx/lib -lodb-sqlite -lodb -lsqlite3  -s
OBJDIR = obj/linux/
BINDIR = bin/linux
.PHONY: all pre clean
.SUFFIXES: .o .cpp

-include mk/common.mk
CXXFLAGS += -Icvmlib_lx/include
CXXGUAC += -Icvmlib_lx/include


