TARGET_RECORD = rfrecorder
TARGET_PLAY = rfplayer
TARGET_SCAN = rfscanner
PREFIX = /usr/local

LIBS = -lwiringPi
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean install uninstall

default: $(TARGET_RECORD) $(TARGET_PLAY) $(TARGET_SCAN)
all: default

OBJECTS_RECORD = rfrecorder.c rfcommon.c
OBJECTS_PLAY = rfplayer.c rfcommon.c
OBJECTS_SCAN = rfscanner.c rfcommon.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET_RECORD) $(OBJECTS_RECORD) $(TARGET_PLAY) $(OBJECTS_PLAY) $(TARGET_SCAN) $(OBJECTS_SCAN)

$(TARGET_RECORD): $(OBJECTS_RECORD)
	$(CC) $(OBJECTS_RECORD) $(CFLAGS) $(LIBS) -o $@

$(TARGET_PLAY): $(OBJECTS_PLAY)
	$(CC) $(OBJECTS_PLAY) $(CFLAGS) $(LIBS) -o $@

$(TARGET_SCAN): $(OBJECTS_SCAN)
	$(CC) $(OBJECTS_SCAN) $(CFLAGS) $(LIBS) -o $@

install:
	cp $(TARGET_RECORD) $(PREFIX)/bin/$(TARGET_RECORD)
	cp $(TARGET_PLAY) $(PREFIX)/bin/$(TARGET_PLAY)
	cp $(TARGET_SCAN) $(PREFIX)/bin/$(TARGET_SCAN)

uninstall:
	rm -f $(PREFIX)/bin/($TARGET_RECORD)
	rm -f $(PREFIX)/bin/($TARGET_PLAY)
	rm -f $(PREFIX)/bin/($TARGET_SCAN)

clean:
	-rm -f *.o
	-rm -f $(TARGET_RECORD)
	-rm -f $(TARGET_PLAY)
	-rm -f $(TARGET_SCAN)
