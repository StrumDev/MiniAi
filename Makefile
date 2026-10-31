CC = clang

ifeq ($(OS),Windows_NT)
    PROGRAM = MiniAi.exe
    FLAGS = -std=gnu11 -O2 -s
    LIBRARIES =
else
    PROGRAM = MiniAi
    FLAGS = -std=gnu11 -O2 -s -pthread
    LIBRARIES = -lm
endif

.PHONY: run

run:
	@$(CC) $(FLAGS) $(wildcard src/*.c) -o $(PROGRAM) $(LIBRARIES)
	@./$(PROGRAM)
