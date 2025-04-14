CC = gcc
BUILD_DIR = build
CFLAGS = -Wall

all: integral integral_calculator integral_sender

integral: integral.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $< -lm

test_integral: integral
	./$(BUILD_DIR)/integral 0.000000001 20

integral_sender: integral_sender.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $<

integral_calculator: integral_calculator.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $<

run_calculator: integral_calculator 
	./$(BUILD_DIR)/integral_calculator

run_sender: integral_sender
	./$(BUILD_DIR)/integral_sender

clean:
	rm -rf $(BUILD_DIR)