LDLIBS= -lX11 -lXpm
CFLAGS+= -Wall -Wextra -DUSE_XSCREENSAVER

multi: multi.o xscreensave.o

all: multi

clean:
	-rm -f multi multi.o xscreensave.o	
