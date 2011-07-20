# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = $(SMSDK_ROOT)/sourcemod-1.3
SRCDS_BASE = /opt/srcds
HL2SDK_L4D2 = $(SMSDK_ROOT)/hl2sdk-l4d2
MMSOURCE18 = $(SMSDK_ROOT)/mmsource-1.8

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = tickrate_enabler

OBJECTS = tickrate_enabler.cpp MemoryUtils/memutils.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

HL2PUB = $(HL2SDK_L4D2)/public
HL2LIB = $(HL2SDK_L4D2)/lib/linux
METAMOD = $(MMSOURCE18)/core
SRCDS = $(SRCDS_BASE)/left4dead2

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a libvstdlib.so libtier0.so $(METAMOD)/Release.left4dead2/sourcehook/*.o

INCLUDE += -I. -I$(HL2PUB) -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(METAMOD) 

LINK += -m32 -ldl -lm

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
        -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -Wno-switch \
        -Wno-unused -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32

CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fno-threadsafe-statics

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release.left4dead2
	CFLAGS += $(C_OPT_FLAGS)
endif

OS := $(shell uname -s)
ifeq "$(OS)" "Darwin"
	LINK += -dynamiclib
	BINARY = $(PROJECT).dylib
else
	LINK += -static-libgcc -shared
	BINARY = $(PROJECT).so
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)
ifeq "$(GCC_VERSION)" "4"
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	cp $(SRCDS)/bin/libvstdlib.so libvstdlib.so;
	cp $(SRCDS)/bin/libtier0.so libtier0.so;
	$(MAKE) -f Makefile extension

extension: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -o $(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean:
	find $(BIN_DIR) -iname *.o | xargs rm -f
	rm -rf $(BIN_DIR)/$(BINARY)
	rm ./*.so
