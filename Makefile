CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinclude -O3
LDFLAGS =  -lm -lsndfile 

BUILD   = build
TARGET  = cno
SRC     = $(wildcard src/*.c)
OBJ     = $(patsubst src/%.c, $(BUILD)/%.o, $(SRC))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(TARGET)
