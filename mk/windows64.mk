CXX = x86_64-w64-mingw32-g++
CC = x86_64-w64-mingw32-gcc
CCFLAGS = -O2 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Iodb/include
CXXFLAGS = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Icvmlib/include
CXXGUAC =  -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/guacamole -Icvmlib/include
LIBS = -lboost_system-mt-x64 -lcairo -lfreetype -lvncclient -lnettle -lgnutls -lgmp -lmswsock -lws2_32 -lp11-kit.dll -lharfbuzz -lgraphite2 -ldwrite -lrpcrt4 -lintl -liconv -lbz2 -lz -luser32 -lgdi32 -lpng -lz -luuid -lturbojpeg -lpixman-1 -ldl -Lcvmlib/lib -lodb-sqlite -lodb -lsqlite3 -s

OBJS = obj/windows64/Main.o obj/windows64/CollabVM.o obj/windows64/Database.o obj/windows64/Config-odb.o obj/windows64/VMSettings-odb.o obj/windows64/QMP.o obj/windows64/QEMUController.o obj/windows64/VMController.o obj/windows64/QMPClient.o obj/windows64/AgentClient.o obj/windows64/unicode.o obj/windows64/error.o obj/windows64/guac_clipboard.o obj/windows64/guac_cursor.o obj/windows64/guac_display.o obj/windows64/guac_dot_cursor.o obj/windows64/guac_iconv.o obj/windows64/guac_list.o obj/windows64/guac_pointer_cursor.o obj/windows64/guac_rect.o obj/windows64/guac_string.o obj/windows64/guac_surface.o obj/windows64/hash.o obj/windows64/id.o obj/windows64/palette.o obj/windows64/pool.o obj/windows64/protocol.o obj/windows64/timestamp.o obj/windows64/GuacSocket.o obj/windows64/GuacWebSocket.o obj/windows64/GuacBroadcastSocket.o obj/windows64/GuacClient.o obj/windows64/GuacUser.o obj/windows64/GuacVNCClient.o obj/windows64/GuacInstructionParser.o obj/windows64/UriCommon.o obj/windows64/UriFile.o obj/windows64/UriNormalizeBase.o obj/windows64/UriParse.o obj/windows64/UriResolve.o obj/windows64/UriCompare.o obj/windows64/UriIp4Base.o obj/windows64/UriNormalize.o obj/windows64/UriQuery.o obj/windows64/UriShorten.o obj/windows64/UriEscape.o obj/windows64/UriIp4.o obj/windows64/UriParseBase.o obj/windows64/UriRecompose.o
.PHONY: all pre clean
.SUFFIXES: .o .cpp

all: pre bin/windows64/collab-vm-server.exe

pre:
	@if [ ! -d "bin/windows64/" ]; then echo "[>] MakeDir 'bin/windows64/'" && mkdir -p bin/windows64/; fi; 
	@if [ ! -d "obj/windows64/" ]; then echo "[>] MakeDir 'obj/windows64/'" && mkdir -p obj/windows64/; fi; 

bin/windows64/collab-vm-server.exe: $(OBJS)
	@echo "[>] Link $@"
	@$(CXX) $(OBJS) -o $@ $(CCFLAGS) $(LIBS) 

obj/windows64/%.o: src/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows64/%.o: src/Database/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows64/%.o: src/Database/%.cxx
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows64/%.o: src/VMControllers/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 
	
obj/windows64/%.o: src/Sockets/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/windows64/%.o: src/guacamole/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/windows64/%.o: src/guacamole/vnc/%.cpp
	@echo  "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/windows64/%.o: src/uriparser/%.c
	@echo "[>] CC $<"
	@$(CC) -c $< $(CCFLAGS) -o $@ 

clean:
	@echo "[>] RmDir 'bin/windows64/'"
	@$(RM) -r bin/windows64/
	@echo "[>] RmDir 'obj/windows64/'" 
	@$(RM) -r obj/windows64/windows/
