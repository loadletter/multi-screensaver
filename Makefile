LDLIBS= -lX11 -lXpm
#CFLAGS= -march=native -O2 -Wall

multi: multi.o

all: multi

clean:
	-rm -f multi multi.o
	
