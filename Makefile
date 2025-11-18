CC=gcc
CFLAGS=-Wall -O2 -fomit-frame-pointer
TARGET=logger
SRC=logger.c
all:$(TARGET)
$(TARGET):$(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
clean:
	rm -f $(TARGET)
.PHONY: all clean
