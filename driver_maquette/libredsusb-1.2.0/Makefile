
# Linux makefile for redsusb.so


override FLAGS        += -std=c99 
override CFLAGS       += -fPIC -g -Wall -I/usr/include/libusb-1.0/ 
override LDFLAGS      += -shared -lusb-1.0 

TARGET  = libredsusb.so
SOURCES = $(wildcard *.c)
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
			cp -p $(HEADERS) $(PREFIX_HEADER)
			cp -p 77-redsusb-maqtrain.rules /etc/udev/rules.d/
			
clean		:
		rm -rf *.o
		rm -rf *.so
