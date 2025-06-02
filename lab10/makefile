CC = gcc
CFLAGS = -Wall -pthread
TARGET = doctor
SOURCE = doctor.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all

test: $(TARGET)
	@echo "=== TEST: 6 patients ==="
	timeout 30s ./$(TARGET) 6

.PHONY: all clean rebuild test test1 test2 test_error debug