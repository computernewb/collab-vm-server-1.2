CXX = i686-w64-mingw32-g++
CC = i686-w64-mingw32-gcc
CCFLAGS = -O2 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Iodb/include
CXXFLAGS = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Icvmlib32/include
CXXGUAC =  -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/guacamole -Icvmlib32/include
LIBS = -lboost_system-mt -lcairo -lfreetype -lvncclient -lnettle -lgnutls -lgmp -lmswsock -lws2_32 -lp11-kit.dll -lharfbuzz -lgraphite2 -ldwrite -lrpcrt4 -lintl -liconv -lbz2 -lz -luser32 -lgdi32 -lpng -lz -luuid -lturbojpeg -lpixman-1 -ldl -Lcvmlib32/lib -lodb-sqlite -lodb -lsqlite3 -s

OBJS = obj/windows32/Main.o obj/windows32/CollabVM.o obj/windows32/Database.o obj/windows32/Config-odb.o obj/windows32/VMSettings-odb.o obj/windows32/QMP.o obj/windows32/QEMUController.o obj/windows32/VMController.o obj/windows32/QMPClient.o obj/windows32/AgentClient.o obj/windows32/unicode.o obj/windows32/error.o obj/windows32/guac_clipboard.o obj/windows32/guac_cursor.o obj/windows32/guac_display.o obj/windows32/guac_dot_cursor.o obj/windows32/guac_iconv.o obj/windows32/guac_list.o obj/windows32/guac_pointer_cursor.o obj/windows32/guac_rect.o obj/windows32/guac_string.o obj/windows32/guac_surface.o obj/windows32/hash.o obj/windows32/id.o obj/windows32/palette.o obj/windows32/pool.o obj/windows32/protocol.o obj/windows32/timestamp.o obj/windows32/GuacSocket.o obj/windows32/GuacWebSocket.o obj/windows32/GuacBroadcastSocket.o obj/windows32/GuacClient.o obj/windows32/GuacUser.o obj/windows32/GuacVNCClient.o obj/windows32/GuacInstructionParser.o obj/windows32/UriCommon.o obj/windows32/UriFile.o obj/windows32/UriNormalizeBase.o obj/windows32/UriParse.o obj/windows32/UriResolve.o obj/windows32/UriCompare.o obj/windows32/UriIp4Base.o obj/windows32/UriNormalize.o obj/windows32/UriQuery.o obj/windows32/UriShorten.o obj/windows32/UriEscape.o obj/windows32/UriIp4.o obj/windows32/UriParseBase.o obj/windows32/UriRecompose.o
.PHONY: all pre clean
.SUFFIXES: .o .cpp

all: pre bin/windows32/collab-vm-server.exe

pre:
	@if [ ! -d "bin/windows32/" ]; then echo "[>] MakeDir 'bin/windows32/'" && mkdir -p bin/windows32/; fi; 
	@if [ ! -d "obj/windows32/" ]; then echo "[>] MakeDir 'obj/windows32/'" && mkdir -p obj/windows32/; fi; 

bin/windows32/collab-vm-server.exe: $(OBJS)
	@echo "[>] Link $@"
	@$(CXX) $(OBJS) -o $@ $(CCFLAGS) $(LIBS) 

obj/windows32/%.o: src/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows32/%.o: src/Database/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows32/%.o: src/Database/%.cxx
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows32/%.o: src/VMControllers/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 
	
obj/windows32/%.o: src/Sockets/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows32/%.o: src/guacamole/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/windows32/%.o: src/guacamole/vnc/%.cpp
	@echo  "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/windows32/%.o: src/uriparser/%.c
	@echo "[>] CC $<"
	@$(CC) -c $< $(CCFLAGS) -o $@ 

clean:
	@echo "[>] RmDir 'bin/windows32/'"
	@$(RM) -r bin/windows32/
	@echo "[>] RmDir 'obj/windows32/'" 
	@$(RM) -r obj/windows32/windows32/
