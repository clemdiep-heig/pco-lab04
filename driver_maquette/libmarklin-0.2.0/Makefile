
# Linux makefile for marklin2.so

CC = g++

override FLAGS        += -std=c+11 
override CFLAGS       += -fPIC -g -Wall
override LDFLAGS      += -shared -lredsusb

TARGET  = libmarklin.so.0.2.0
SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.c=.o)

PREFIX_LIB = /usr/lib/
PREFIX_HEADER = /usr/include/

.PHONY: all clean

all		: $(TARGET)

$(TARGET) 	: $(OBJECTS)
			$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o		: %.c
			$(CC) $(FLAGS) $(CFLAGS) -c $^ -o $@ 

install		: all
			cp -p $(TARGET) $(PREFIX_LIB)
			ln -sf $(PREFIX_LIB)$(TARGET) $(PREFIX_LIB)libmarklin.so
			cp -p $(HEADERS) $(PREFIX_HEADER)
			
clean		:
		rm -rf *.o
		rm -rf *.so*
