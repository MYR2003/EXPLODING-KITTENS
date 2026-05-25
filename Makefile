# Makefile for Exploding Kittens Game
# Usage: make          (builds everything)
#        make server   (builds only server)
#        make client   (builds only client)
#        make clean    (remove binaries)
#        make run      (run server in background)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2
DEBUG_FLAGS = -g -O0

# Libraries
LIBS = -lm -lpthread
RAYLIB_LIBS = -lraylib -lm -lpthread

# Source files
FUNCIONES = funciones.c
SERVER_SRC = server_complete_enhanced.c
CLIENT_SRC = client_raylib.c

# Object files
FUNCIONES_OBJ = funciones.o
SERVER_OBJ = server_complete_enhanced.o
CLIENT_OBJ = client_raylib.o

# Binaries
SERVER_BIN = server
CLIENT_BIN = client

# Phony targets
.PHONY: all server client clean run debug help

# Default target
all: $(SERVER_BIN) $(CLIENT_BIN)
	@echo "✓ Build complete!"
	@echo "Run: ./server (in one terminal)"
	@echo "Run: ./client (in another terminal)"

# Compile funciones.c object file
$(FUNCIONES_OBJ): $(FUNCIONES)
	@echo "Compiling funciones.c..."
	$(CC) $(CFLAGS) -c $(FUNCIONES) -o $(FUNCIONES_OBJ)

# Build server
$(SERVER_BIN): $(SERVER_SRC) $(FUNCIONES_OBJ)
	@echo "Building server..."
	$(CC) $(CFLAGS) $(SERVER_SRC) $(FUNCIONES_OBJ) $(LIBS) -o $(SERVER_BIN)
	@echo "✓ Server compiled: ./$(SERVER_BIN)"

# Build client
$(CLIENT_BIN): $(CLIENT_SRC) $(FUNCIONES_OBJ)
	@echo "Building client with Raylib..."
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(FUNCIONES_OBJ) $(RAYLIB_LIBS) -o $(CLIENT_BIN)
	@echo "✓ Client compiled: ./$(CLIENT_BIN)"

# Debug build (with symbols)
debug: CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra
debug: clean all
	@echo "✓ Debug build complete (symbols included)"
	@echo "Run: gdb ./server"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(FUNCIONES_OBJ) *.o
	@echo "✓ Clean complete"

# Run server in background
run: all
	@echo "Starting server on port 8080..."
	./$(SERVER_BIN) &
	@sleep 1
	@echo "✓ Server started (PID: $$!)"
	@echo "Start clients with: ./$(CLIENT_BIN)"

# Stop all running instances
stop:
	@echo "Stopping server..."
	pkill -f ./server
	@echo "✓ Server stopped"

# Run with memory check
valgrind: debug
	@echo "Running server with memory check..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(SERVER_BIN)

# Check dependencies
check:
	@echo "Checking dependencies..."
	@command -v gcc >/dev/null 2>&1 && echo "✓ GCC found" || echo "✗ GCC not found"
	@pkg-config --exists raylib && echo "✓ Raylib found" || echo "✗ Raylib not found"
	@command -v pkg-config >/dev/null 2>&1 && echo "✓ pkg-config found" || echo "✗ pkg-config not found"

# Help
help:
	@echo "Exploding Kittens - Makefile Commands"
	@echo ""
	@echo "make              - Compile everything (server + client)"
	@echo "make server       - Compile only server"
	@echo "make client       - Compile only client"
	@echo "make debug        - Compile with debug symbols"
	@echo "make clean        - Remove compiled binaries"
	@echo "make run          - Start server in background"
	@echo "make stop         - Stop running server"
	@echo "make valgrind     - Run with memory leak detection"
	@echo "make check        - Check if dependencies are installed"
	@echo "make help         - Show this message"
	@echo ""
	@echo "Examples:"
	@echo "  make             # Compile both"
	@echo "  make debug       # Debug build"
	@echo "  make run         # Start server"
	@echo "  ./client         # Start client (separate terminal)"
	@echo ""

# Print variables (for debugging Makefile)
print:
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo "SERVER_BIN: $(SERVER_BIN)"
	@echo "CLIENT_BIN: $(CLIENT_BIN)"
