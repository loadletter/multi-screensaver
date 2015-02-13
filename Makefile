LDLIBS= -lX11 -lXpm -lXt
CFLAGS+= -Wall -Wextra 

multi: multi.o

all: multi

clean:
	-rm -f multi multi.o
	
