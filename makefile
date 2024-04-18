CC = gcc
CFLAGS = -Wall -Werror -std=c11 -lpthread

TARGET = monitor sensor

all: $(TARGET)

monitor: monitor.c
	$(CC) $(CFLAGS) -o $@ $<

sensor: sensor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)