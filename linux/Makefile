TARGET = sm_hs_measure50_app.app
TARGET_LATENCY = sm_hs_latency.app
SOURCEDIR = source
LIBS = -lm
CC = g++
CFLAGS = -g -Wall -Werror -Isource -std=c++11 -Ofast
TARGET_WAVE = synthmark_one_voice.wav

VPATH = apps:source:source/portaudio

OBJECTS = write_wav.o

.PHONY: default all clean

default: $(TARGET)
all: default

HEADERS := $(shell find $(SOURCEDIR) -name '*.h')

%.o: %.c $(HEADERS)  linux/Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS) linux/Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET)

echo:
	echo $(HEADERS)

%.app: %.o $(OBJECTS)
	$(CC) $< $(OBJECTS) -Wall $(LIBS) -o $@

$(TARGET_WAVE): sm_record_one.app
	./sm_record_one.app $(TARGET_WAVE)

wavout: $(TARGET_WAVE)

voicemark: $(TARGET)
	./$(TARGET)

latency: $(TARGET_LATENCY)
	./$(TARGET_LATENCY)

clean:
	-rm -f *.o
	-rm -f *.app
	-rm -f $(TARGET_WAVE)
	-rm -f $(TARGET)

