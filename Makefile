# (C)2004-2008 SourceMod Development Team
# Original Makefile written by David "BAILOPAN" Anderson

SRCDS_BASE ?= ~/srcds
HL2SDK_L4D2 ?= $(SMSDK_ROOT)/hl2sdk-l4d2
HL2SDK_L4D ?= $(SMSDK_ROOT)/hl2sdk-l4d
MMSOURCE18 ?= $(MMSOURCE)
L4D ?= 2

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = tickrate_enabler

SH_OBJECTS = sourcehook.cpp sourcehook_impl_chookidman.cpp

OBJECTS = tickrate_enabler.cpp memutils.cpp $(addprefix sourcehook/,$(SH_OBJECTS))

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

ifeq "$(L4D)" "1"
	HL2PUB = $(HL2SDK_L4D)/public
else
	HL2PUB = $(HL2SDK_L4D2)/public
endif
ifeq "$(L4D)" "1"
	HL2LIB = $(HL2SDK_L4D)/lib/linux
else
	HL2LIB = $(HL2SDK_L4D2)/lib/linux
endif
ifeq "$(L4D)" "1"
	SRCDS = $(SRCDS_BASE)/l4d
else
	SRCDS = $(SRCDS_BASE)/left4dead2
endif

LINK += $(HL2LIB)/tier1_i486.a $(HL2LIB)/mathlib_i486.a libvstdlib.so libtier0.so

INCLUDE += -I. -I$(HL2PUB) -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -Isourcehook

LINK += -m32 -ldl -lm

CFLAGS += -D_LINUX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
        -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -Wall -Werror -Wno-switch \
        -Wno-error=uninitialized -Wno-unused -Wno-error=delete-non-virtual-dtor -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32

CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fno-threadsafe-statics

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

ifeq "$(DEBUG)" "true"
	ifeq "$(L4D)" "1"
		BIN_DIR = Debug.left4dead
	else
		BIN_DIR = Debug.left4dead2
	endif
	CFLAGS += $(C_DEBUG_FLAGS)
else
	ifeq "$(L4D)" "1"
		BIN_DIR = Release.left4dead
	else
		BIN_DIR = Release.left4dead2
	endif
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
	mkdir -p $(BIN_DIR) $(BIN_DIR)/sourcehook
	$(MAKE) -f Makefile extension

l4d1:
	$(MAKE) -f Makefile all L4D=1


extension: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) $(LINK) -o $(BIN_DIR)/$(BINARY)

l4d1-debug:
	$(MAKE) -f Makefile all DEBUG=true L4D=1

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean:
	find $(BIN_DIR) -iname *.o | xargs rm -f
	rm -rf $(BIN_DIR)/$(BINARY)
	rm ./*.so
