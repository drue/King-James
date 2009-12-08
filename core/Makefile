OBJS =  memq.o port.o Transport.o core.o
CC = g++
CPPFLAGS =-O2 -pipe -g -Wall -D_LARGEFILE64_SOURCE -DDEBUG
CFLAGS = -O2 -pipe -g -Wall -D_LARGEFILE64_SOURCE -DDEBUG
LDFLAGS = -lasound -pg -lFLAC -lpython2.4 -lpthread -g
DEBUG=1


score: $(OBJS) tportmodule.o
	$(CC) $(CFLAGS) -o score $(OBJS) tportmodule.o $(LDFLAGS)

tportmodule.o:
	$(CC) $(CFLAGS) -I/usr/include/python2.4 -c tportmodule.cc -o tportmodule.o

all: score

clean:
	rm *.o
