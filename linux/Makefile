# Makefile for SynthMark - audio performance benchmark

TARGET = synthmark.app
SOURCEDIR = source
LIBS = -lm -lpthread
CFLAGS = -g -Wall -Werror -Isource -std=c++11 -Ofast

VPATH = apps:source:source/tools

OBJECTS := $(patsubst %.cpp,%.o,$(shell find $(SOURCEDIR) -name '*.cpp'))

.PHONY: default all clean

default: $(TARGET)
all: default

HEADERS := $(shell find $(SOURCEDIR) -name '*.h')

%.o: %.c $(HEADERS)  linux/Makefile
	$(CXX) $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS) linux/Makefile
	$(CXX) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET)

echo:
	@echo $(HEADERS)

%.app: %.o $(OBJECTS)
	$(CXX) $< $(OBJECTS) -Wall $(LIBS) -o $@

synthmark: $(TARGET)
	./$(TARGET)

clean:
	-rm -f *.o
	-rm -f *.app
	-rm -f $(TARGET)
