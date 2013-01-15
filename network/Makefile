#.SUFFIXES: .cpp .o .c
CLIENTSDK_DIR = ../clientsdk
TRACE_DIR = ../SSTrace
CC = gcc
CC_FLAGS = 
CC_DEFINES = 
CC_INCLUDES = 
CPP = g++
CPP_FLAGS = -g
CPP_DEFINES = -DUNIX -D_UNIX__ -DSERVER -DLINUX -D_GNU_SOURCE -DCM_DEBUG -D_REENTRANT -Wall
CPP_INCLUDES = -Iinclude -I$(CLIENTSDK_DIR)/h -I../common  -I$(TRACE_DIR) 
LINKER = g++
LINKER_FLAGS = -L$(TRACE_DIR) -lsstrace
AR = ar
AR_FLAGS = rcs
LIBSOURCES = $(wildcard *.cpp)

LIBOBJECTS = $(subst .cpp,.o,$(LIBSOURCES))

TARGETLIB = libvlnetwork.a

all: $(TESTEXEC) $(TARGETLIB)

$(TARGETLIB): $(LIBOBJECTS)
	$(AR) $(AR_FLAGS) $@ $(LIBOBJECTS)
	
.c.o:
	$(CC) -c $< $(CC_FLAGS) $(CC_DEFINES) $(CC_INCLUDES)
.cpp.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)

clean: 
	rm -f $(LIBOBJECTS) $(TARGETLIB)
	rm -f $(subst .cpp,.d,$(LIBSOURCES))

-include $(subst .cpp,.d,$(LIBSOURCES))

%.d: %.cpp
	$(CC) -M $(CPP_INCLUDES) $< > $@.$$$$;                      \
        sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$
