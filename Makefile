LIBPD = ../libpd/libpd_wrapper
PD = ../libpd/pure-data/src
INC = -I$(LIBPD) -I$(PD)
LIB = ../libpd/libs/libpd.a

LIB += -lpthread  -lm 

ifeq ($(shell uname -s), Darwin)
LIB += -framework AudioUnit -framework CoreAudio -framework CoreFoundation
endif

all: test1

test1: test1.c
	cc -g $(INC) test1.c -o test1 $(LIB)

clean:
	rm -f test1
	rm -f test1.o
	rm -rf *.dSYM

