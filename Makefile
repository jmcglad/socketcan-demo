TARGETS = socketcan-raw-demo socketcan-bcm-demo socketcan-cyclic-demo

# Compiler setup
# Note, the code depends on glibc
CC = gcc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -std=gnu17 -Wall -Wextra

#
# Rules
#

.PHONY: all debug clean

all: CPPFLAGS += -DNDEBUG
all: CFLAGS += -O2
all: $(TARGETS)

debug: CFLAGS += -g
debug: $(TARGETS)

socketcan-raw-demo: socketcan-raw-demo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

socketcan-bcm-demo: socketcan-bcm-demo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

socketcan-cyclic-demo: socketcan-cyclic-demo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGETS)
