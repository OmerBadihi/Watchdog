
# make clean/all CONFIG=Debug/Release ARCH=linux64/raspi ECHO=0/1

# the parrent dir of "arch" dir
ROOT_DIR = ../../..

#PROJ_SRCS = $(wildcard *.c)
PROJ_SRCS = wdlib.c

# list of directories containing header files
PROJ_INC_DIR = ../../../ds/SHeap ../../../ds/uid/

# the executable file
PROJ_OUT = libwd.so

# list of ADDITIONAL compiler flags (prepended with "-" sign)
#PROJ_CFLAGS = -Wunused-but-set-variable
PROJ_CFLAGS = -D_POSIX_C_SOURCE=199309L
PROJ_CPPFLAGS =

#PROJ_MACROS = _BSD_SOURCE
PROJ_MACROS =

# additional object files (created by other projects)
PROJ_ADD_OBJS = SHeap.o heap.o dynamic_vector.o PQHeap.o uid.o task.o

# additional libraries
PROJ_LIBS = pthread

# linker flags
PROJ_LDFLAGS =

# list of projects (makefiles) this project depends on
PROJ_DEPENDS  = ../../../ds/SHeap/Makefile

include $(ROOT_DIR)/make/Project.mak
