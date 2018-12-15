CXX = g++
CC = gcc
CCFLAGS = -O2 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Iodb/include
CXXFLAGS = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser -Icvmlib_lx/include
CXXGUAC = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/guacamole -Icvmlib_lx/include
LIBS = -static-libstdc++ -lpthread -lboost_system -lpng -lz -luuid -lvncclient -lturbojpeg -lcairo -lpixman-1 -ldl -Lcvmlib_lx/lib -lodb-sqlite -lodb -lsqlite3  -s
OBJS = obj/linux/Main.o obj/linux/CollabVM.o obj/linux/Database.o obj/linux/Config-odb.o obj/linux/VMSettings-odb.o obj/linux/QMP.o obj/linux/QEMUController.o obj/linux/VMController.o obj/linux/QMPClient.o obj/linux/AgentClient.o obj/linux/unicode.o obj/linux/error.o obj/linux/guac_clipboard.o obj/linux/guac_cursor.o obj/linux/guac_display.o obj/linux/guac_dot_cursor.o obj/linux/guac_iconv.o obj/linux/guac_list.o obj/linux/guac_pointer_cursor.o obj/linux/guac_rect.o obj/linux/guac_string.o obj/linux/guac_surface.o obj/linux/hash.o obj/linux/id.o obj/linux/palette.o obj/linux/pool.o obj/linux/protocol.o obj/linux/timestamp.o obj/linux/GuacSocket.o obj/linux/GuacWebSocket.o obj/linux/GuacBroadcastSocket.o obj/linux/GuacClient.o obj/linux/GuacUser.o obj/linux/GuacVNCClient.o obj/linux/GuacInstructionParser.o obj/linux/UriCommon.o obj/linux/UriFile.o obj/linux/UriNormalizeBase.o obj/linux/UriParse.o obj/linux/UriResolve.o obj/linux/UriCompare.o obj/linux/UriIp4Base.o obj/linux/UriNormalize.o obj/linux/UriQuery.o obj/linux/UriShorten.o obj/linux/UriEscape.o obj/linux/UriIp4.o obj/linux/UriParseBase.o obj/linux/UriRecompose.o
.PHONY: all pre clean
.SUFFIXES: .o .cpp

all: pre bin/linux/collab-vm-server

pre:
	@if [ ! -d "bin/linux/" ]; then echo "[>] MKDIR 'bin/linux/'" && mkdir -p bin/linux/; fi; 
	@if [ ! -d "obj/linux/" ]; then echo "[>] MKDIR 'obj/linux/'" && mkdir -p obj/linux/; fi; 

bin/linux/collab-vm-server: $(OBJS)
	@echo "[>] Link $@"
	@$(CXX) $(OBJS) -o $@ $(CCFLAGS) $(LIBS) 

obj/linux/%.o: src/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/linux/%.o: src/Database/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/linux/%.o: src/Database/%.cxx
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/linux/%.o: src/VMControllers/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 
	
obj/linux/%.o: src/Sockets/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

obj/linux/%.o: src/guacamole/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/linux/%.o: src/guacamole/vnc/%.cpp
	@echo  "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

obj/linux/%.o: src/uriparser/%.c
	@echo "[>] CC $<"
	@$(CC) -c $< $(CCFLAGS) -o $@ 

clean:
	@echo "[>] RMDIR 'bin/linux/'"
	@$(RM) -r bin/linux/
	@echo "[>] RMDIR 'obj/linux/'" 
	@$(RM) -r obj/linux/
