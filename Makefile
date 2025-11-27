TARGETS=socketcan-raw-demo socketcan-bcm-demo socketcan-cyclic-demo
SRCDIR=src

# Compiler setup
CC=gcc
CPPFLAGS=-Isrc
CFLAGS=-std=gnu11 -pedantic -Wall -Wextra
LIBS=

# Programs
RM=rm -f

# Rules
.PHONY: all debug clean rebuild

all: CPPFLAGS+=-DNDEBUG
all: CFLAGS+=-O3
all: $(TARGETS)

debug: CFLAGS+=-g
debug: $(TARGETS)

socketcan-raw-demo: $(SRCDIR)/socketcan-raw-demo.o $(SRCDIR)/util.o
	$(CC) -o $@ $^ $(LIBS)

socketcan-bcm-demo: $(SRCDIR)/socketcan-bcm-demo.o $(SRCDIR)/util.o
	$(CC) -o $@ $^ $(LIBS)

socketcan-cyclic-demo: $(SRCDIR)/socketcan-cyclic-demo.o
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(SRCDIR)/*.o
	$(RM) socketcan-raw-demo
	$(RM) socketcan-bcm-demo
	$(RM) socketcan-cyclic-demo

rebuild: clean all

