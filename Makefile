CC = clang

ifeq ($(OS),Windows_NT)
    PROGRAM = MiniAi.exe
    FLAGS = -std=gnu11 -O2 -s -Isrc
    LIBRARIES =
else
    PROGRAM = MiniAi
    FLAGS = -std=gnu11 -O2 -s -Isrc -pthread
    LIBRARIES = -lm
endif

.PHONY: build run

build:
	@$(CC) $(FLAGS) $(wildcard src/*.c) $(wildcard src/*/*.c) -o $(PROGRAM) $(LIBRARIES)

run: build
	@./$(PROGRAM)