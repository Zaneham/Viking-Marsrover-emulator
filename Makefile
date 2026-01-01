# DDP-24 Emulator Makefile
# Viking Mars Lander Guidance Computer

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

SRCDIR = src
INCDIR = include
OBJDIR = obj

SOURCES = $(SRCDIR)/ddp24.c $(SRCDIR)/main.c
OBJECTS = $(OBJDIR)/ddp24.o $(OBJDIR)/main.o
TARGET = ddp24

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/ddp24.h | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

test: $(TARGET)
	./$(TARGET) -t

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET).exe

# Windows-specific
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q
    MKDIR = mkdir
endif
