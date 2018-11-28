CC=gcc
CFLAGS=-g -Wall -O2 -lstdc++
LDFLAGS=-L/usr/lib/
LIBS=-L/usr/lib/ -lpthread 
all:EpolleServer

EpolleServer: CEvent.o
	$(CC) -o ./bin/EpolleServer $(INCLUDES) $(CFLAGS) ./bin/CEvent.o main.cpp $(LIBS)

CEvent.o: CEvent.h
	$(CC) -o ./bin/CEvent.o $(CFLAGS) $(INCLUDES) -c CEvent.cpp


clean:
	rm  -f ./bin/*.o ./bin/EpolleServer
