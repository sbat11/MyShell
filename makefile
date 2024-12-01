# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g 

# Target executable
TARGET = myshell

# Source file
SRC = mysh.c

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Run target
run: $(TARGET)
	./$(TARGET)

# Clean target
clean:
	rm -f $(TARGET) *.txt