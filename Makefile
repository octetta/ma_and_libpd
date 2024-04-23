LIBPD = ../libpd/libpd_wrapper
PD = ../libpd/pure-data/src
INC = -I$(LIBPD) -I$(PD)
LIB = ../libpd/libs/libpd.a

all: test1

st1: test1.c
	cc -g $(INC) test1.c -o test1 $(LIB) -lm \
	-framework AudioUnit \
	-framework CoreAudio \
	-framework CoreFoundation \
	-framework AudioToolbox \
	-framework CoreServices \
	#

clean:
	rm -f test1
	rm -f test1.o

