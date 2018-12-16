# Common rules and compiler flags
# How to use:

# Fill OBJDIR with objdir for platform (e.g obj/linux/)
# Fill BINDIR with bindir for platform (e.g bin/linux)
# Define CC and CXX
# then, in your .mk file include this file as:
# -include mk/common.mk
# then, it will work!

CCFLAGS = -O2 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser
CXXFLAGS = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/rapidjson -Isrc/Database -Isrc/VMControllers -Isrc/Sockets -Isrc/guacamole -Isrc/uriparser
CXXGUAC = -O2 -Wno-deprecated-declarations -Wno-deprecated -Wno-write-strings -fpermissive -std=c++11 -Isrc -Isrc/guacamole
OBJS = $(OBJDIR)/Main.o $(OBJDIR)/CollabVM.o $(OBJDIR)/Database.o $(OBJDIR)/Config-odb.o $(OBJDIR)/VMSettings-odb.o $(OBJDIR)/QMP.o $(OBJDIR)/QEMUController.o $(OBJDIR)/VMController.o $(OBJDIR)/QMPClient.o $(OBJDIR)/AgentClient.o $(OBJDIR)/unicode.o $(OBJDIR)/error.o $(OBJDIR)/guac_clipboard.o $(OBJDIR)/guac_cursor.o $(OBJDIR)/guac_display.o $(OBJDIR)/guac_dot_cursor.o $(OBJDIR)/guac_iconv.o $(OBJDIR)/guac_list.o $(OBJDIR)/guac_pointer_cursor.o $(OBJDIR)/guac_rect.o $(OBJDIR)/guac_string.o $(OBJDIR)/guac_surface.o $(OBJDIR)/hash.o $(OBJDIR)/id.o $(OBJDIR)/palette.o $(OBJDIR)/pool.o $(OBJDIR)/protocol.o $(OBJDIR)/timestamp.o $(OBJDIR)/GuacSocket.o $(OBJDIR)/GuacWebSocket.o $(OBJDIR)/GuacBroadcastSocket.o $(OBJDIR)/GuacClient.o $(OBJDIR)/GuacUser.o $(OBJDIR)/GuacVNCClient.o $(OBJDIR)/GuacInstructionParser.o $(OBJDIR)/UriCommon.o $(OBJDIR)/UriFile.o $(OBJDIR)/UriNormalizeBase.o $(OBJDIR)/UriParse.o $(OBJDIR)/UriResolve.o $(OBJDIR)/UriCompare.o $(OBJDIR)/UriIp4Base.o $(OBJDIR)/UriNormalize.o $(OBJDIR)/UriQuery.o $(OBJDIR)/UriShorten.o $(OBJDIR)/UriEscape.o $(OBJDIR)/UriIp4.o $(OBJDIR)/UriParseBase.o $(OBJDIR)/UriRecompose.o

all: pre $(BINDIR)/collab-vm-server

pre:
	@if [ ! -d "$(BINDIR)/" ]; then echo "[>] MKDIR '$(BINDIR)/'" && mkdir -p $(BINDIR)/; fi; 
	@if [ ! -d "$(OBJDIR)" ]; then echo "[>] MKDIR '$(OBJDIR)'" && mkdir -p $(OBJDIR)/; fi; 

$(BINDIR)/collab-vm-server: $(OBJS)
	@echo "[>] Link $@"
	@$(CXX) $(OBJS) -o $@ $(CXXFLAGS) $(LIBS) 

$(OBJDIR)/%.o: src/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

$(OBJDIR)/%.o: src/Database/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

$(OBJDIR)/%.o: src/Database/%.cxx
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

$(OBJDIR)/%.o: src/VMControllers/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 
	
$(OBJDIR)/%.o: src/Sockets/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXFLAGS) -o $@ 

$(OBJDIR)/%.o: src/guacamole/%.cpp
	@echo "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

$(OBJDIR)/%.o: src/guacamole/vnc/%.cpp
	@echo  "[>] CXX $<"
	@$(CXX) -c $< $(CXXGUAC) -o $@ 

$(OBJDIR)/%.o: src/uriparser/%.c
	@echo "[>] CC $<"
	@$(CC) -c $< $(CCFLAGS) -o $@ 

clean:
	@echo "[>] RMDIR '$(BINDIR)/'"
	@$(RM) -r $(BINDIR)/
	@echo "[>] RMDIR '$(OBJDIR)'" 
	@$(RM) -r $(OBJDIR)/
