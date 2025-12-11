CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = lexer.exe

SRCS = main.c lexer.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) test.c

clean:
	del /Q $(OBJS) $(TARGET) 2>nul || exit 0

debug: $(TARGET)
	./$(TARGET) test.c

.PHONY: all clean test debug