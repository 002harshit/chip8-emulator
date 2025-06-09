CC = clang

LIBS = -lX11
FLAGS = -ggdb -O0
OUTPUT_DIR = ./bin
EMU_BIN = $(OUTPUT_DIR)/c8emu

build: $(OUTPUT_DIR) main.c
	$(CC) main.c $(LIBS) $(FLAGS)  -o $(EMU_BIN)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

format:
	clang-format -i *.c *.h

clean:
	rm -p $(EMU_BIN)

.PHONY: build clean format
