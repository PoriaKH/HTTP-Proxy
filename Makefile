CC=gcc
CFLAGS=-ggdb3 -c -Wall -std=gnu99
LDFLAGS=-pthread
SOURCES=./server/httpserver.c ./http-library/libhttp.c ./work-queue/wq.c ./word-library/libword.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=./build/httpserver

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	mkdir build
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
	rmdir build