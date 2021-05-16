########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = g++
CXXFLAGS = -std=c++11 -Wall -g
LDFLAGS = -lnfc -lfreefare -lboost_program_options -lboost_filesystem -lboost_system -lsqlite3 -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lcrypto -lssl

# Makefile settings - Can be customized.
EXT = .cpp
SRCDIR = src
LIBDIR = src/lib
OBJDIR = build/obj/lib

############## Do not change anything from here downwards! #############
LIB = $(wildcard $(LIBDIR)/*$(EXT))
OBJ = $(LIB:$(LIBDIR)/%$(EXT)=$(OBJDIR)/%.o)
DEP = $(OBJ:$(OBJDIR)/%.o=%.d)
# UNIX-based OS variables & settings
RM = rm
DELOBJ = $(OBJ)
# Windows OS variables & settings
DEL = del
EXE = .exe
WDELOBJ = $(LIB:$(LIBDIR)/%$(EXT)=$(OBJDIR)\\%.o)

########################################################################
####################### Targets beginning here #########################
########################################################################

all: transactionreader exectransaction notereader financetokenreader setuptag doorreader

manager: build/obj/manager.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

transactionreader: build/obj/transactionreader.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

exectransaction: build/obj/exectransaction.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

notereader: build/obj/notereader.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

financetokenreader: build/obj/financetokenreader.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

setuptag: build/obj/setuptag.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

doorreader: build/obj/doorreader.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

restapitest: build/obj/restapitest.o $(OBJ)
	$(CC) $(CXXFLAGS) -o build/$@ $^ $(LDFLAGS)

# Creates the dependecy rules
%.d: $(LIBDIR)/%$(EXT)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@

# Includes all .h files
-include $(DEP)

# Building rule for .o files and its .c/.cpp in combination with all .h
build/obj/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: $(LIBDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

################### Cleaning rules for Unix-based OS ###################
# Cleans complete project
.PHONY: clean
clean:
	$(RM) $(DELOBJ) $(DEP) $(APPNAME)

# Cleans only all files with the extension .d
.PHONY: cleandep
cleandep:
	$(RM) $(DEP)

#################### Cleaning rules for Windows OS #####################
# Cleans complete project
.PHONY: cleanw
cleanw:
	$(DEL) $(WDELOBJ) $(DEP) $(APPNAME)$(EXE)

# Cleans only all files with the extension .d
.PHONY: cleandepw
cleandepw:
	$(DEL) $(DEP)