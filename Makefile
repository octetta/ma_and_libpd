LIBPD = ../libpd/libpd_wrapper
PD = ../libpd/pure-data/src
INC = -I$(LIBPD) -I$(PD)
LIB = ../libpd/libs/libpd.a

all: st1

st1: st1.c
	cc -g $(INC) st1.c -o st1 $(LIB) -lm \
	-framework AudioUnit \
	-framework CoreAudio \
	-framework CoreFoundation \
	-framework AudioToolbox \
	-framework CoreServices \
	#

clean:
	rm -f st1
	rm -f st1.o

