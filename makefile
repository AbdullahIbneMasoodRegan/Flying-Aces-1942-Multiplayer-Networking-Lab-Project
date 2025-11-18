# Makefile for Flying Aces: 1942 Multiplayer

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lSDL2 -lSDL2_net -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm

# Targets
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = network_server.c
CLIENT_SRC = main_multiplayer.c network_client.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Default target: build both
all: $(SERVER) $(CLIENT)

# Build server
$(SERVER): $(SERVER_OBJ)
	@echo "Linking server..."
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJ) $(LDFLAGS)
	@echo "Server built successfully!"

# Build client
$(CLIENT): $(CLIENT_OBJ)
	@echo "Linking client..."
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_OBJ) $(LDFLAGS)
	@echo "Client built successfully!"

# Compile source files to object files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -f $(SERVER) $(CLIENT) $(SERVER_OBJ) $(CLIENT_OBJ)
	@echo "Clean complete!"

# Install dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	@echo "Installing dependencies for Ubuntu/Debian..."
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev libsdl2-net-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

# Install dependencies (Fedora/RHEL)
install-deps-fedora:
	@echo "Installing dependencies for Fedora/RHEL..."
	sudo dnf install -y SDL2-devel SDL2_net-devel SDL2_image-devel SDL2_ttf-devel SDL2_mixer-devel

# Install dependencies (macOS with Homebrew)
install-deps-macos:
	@echo "Installing dependencies for macOS..."
	brew install sdl2 sdl2_net sdl2_image sdl2_ttf sdl2_mixer

# Run server
run-server: $(SERVER)
	@echo "Starting server..."
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	@echo "Starting client..."
	./$(CLIENT)

# Help target
help:
	@echo "Flying Aces: 1942 Multiplayer - Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make                    Build both server and client"
	@echo "  make server             Build server only"
	@echo "  make client             Build client only"
	@echo "  make clean              Remove build artifacts"
	@echo "  make run-server         Build and run server"
	@echo "  make run-client         Build and run client"
	@echo "  make install-deps-ubuntu   Install dependencies (Ubuntu/Debian)"
	@echo "  make install-deps-fedora   Install dependencies (Fedora/RHEL)"
	@echo "  make install-deps-macos    Install dependencies (macOS)"
	@echo "  make help               Show this help message"

.PHONY: all clean install-deps-ubuntu install-deps-fedora install-deps-macos run-server run-client help