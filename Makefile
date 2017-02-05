TARGET = rfrecorder
PREFIX = /usr/local

LIBS = -lwiringPi
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean install uninstall

default: $(TARGET)
all: default

OBJECTS = \
			main.c
			 #\
		#	gpio.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(OBJECTS_SERVER)
	$(CC) $(OBJECTS) $(OBJECTS_SERVER) $(CFLAGS) $(LIBS) -o $@

install:
	cp $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/($TARGET)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
