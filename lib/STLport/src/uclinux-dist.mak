#
# Note : this makefile is for gcc-2.95 and later !
#

#CROSS=sh-linux-
#
# compiler
#
#CC=$(CROSS)gcc 
DYN_LINK=$(CXX) -nostdlib -fexceptions -L/home/matthewn/src/head/sme-575/lib/uClibc/lib -L/usr/local/lib/gcc-lib/sh-linux/3.0.4/ -shared -rdynamic -o

CXX+= -fexceptions

#
# Basename for libraries
#
LIB_BASENAME = libstdc++

#
# guts for common stuff
#
#
LINK=$(AR) cr
# 2.95 flag

OBJEXT=o
DYNEXT=so
STEXT=a
RM=rm -rf
PATH_SEP=/
MKDIR=mkdir -p
COMP=GCC$(ARCH)
INSTALL_STEP = install_unix 

SH-LINUX-INCLUDES=-I$(ROOTDIR)/include -idirafter $(ROOTDIR)/include/include

all:  all_dynamic all_static symbolic_links 

include common_macros.mak

WARNING_FLAGS= -Wall -W -Wno-sign-compare -Wno-unused -Wno-uninitialized -ftemplate-depth-32 -frtti

CXXFLAGS_COMMON = -nostdinc -nostdlib -nostdinc++ -I${STLPORT_DIR} ${SH-LINUX-INCLUDES} ${WARNING_FLAGS} -D_STLP_UNIX=1

CXXFLAGS_RELEASE_static = $(CXXFLAGS_COMMON) -O2
CXXFLAGS_RELEASE_dynamic = $(CXXFLAGS_COMMON) -O2 -fPIC

CXXFLAGS_DEBUG_static = $(CXXFLAGS_COMMON) -O -g
CXXFLAGS_DEBUG_dynamic = $(CXXFLAGS_COMMON) -O -g -fPIC

CXXFLAGS_STLDEBUG_static = $(CXXFLAGS_DEBUG_static) -D_STLP_DEBUG
CXXFLAGS_STLDEBUG_dynamic = $(CXXFLAGS_DEBUG_dynamic) -D_STLP_DEBUG

include common_percent_rules.mak
include common_rules.mak


#install: all
#	cp -p $(LIB_TARGET) ${D_LIB_TARGET} ../lib

#%.s: %.cpp
#	$(CXX) $(CXXFLAGS) -O4 -S -pto $<  -o $@


