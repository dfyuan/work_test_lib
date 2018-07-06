CC =gcc

INCLUDES = -I./include
LIBS	= -L./lib

CFLAGS += -fno-strict-aliasing  $(INCLUDES) -Wall -O2 -g

LDFLAGS	= $(LIBS) -Wl,--start-group $(LDLIBS) -Wl,--end-group
APPS = hidraw_test

.SILENT:

all: $(APPS)


hidraw_test: hidraw_test.o 
	$(CC) $(CFLAGS) $? -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(APPS) *.o
