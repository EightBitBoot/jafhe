SHELL = /bin/bash
CC = gcc
CFLAGS = `pkg-config --cflags --libs gtk+-3.0`


.SUFFIXES:
.SUFFIXES: .c .o

objects = main.o

srcdir = src/
builddir = build/
bindir = bin/

pobjects = $(addprefix $(builddir), $(objects))

all: build

debug: CFLAGS += -g -Wall -Werror -Wpedantic 
debug: build

build: $(objects) | $(bindir)
	$(CC) -o $(bindir)jafhe $(pobjects) $(CFLAGS)

%.o: $(srcdir)%.c | $(builddir)
	$(CC) -o $(builddir)$@ $< $(CFLAGS) -c

$(builddir):
	mkdir build

$(bindir):
	mkdir bin

.PHONY: clean
clean:
	rm -rf build/ bin/