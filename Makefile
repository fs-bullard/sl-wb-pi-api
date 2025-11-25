# Makefile for Western Blot Capture Library
#
# Builds libcapture.so - C shared library for camera control
# Links against libxdtusb (vendor camera SDK)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared -lpthread -lxdtusb

# Directories
SRC_DIR = src
LIB_DIR = library_examples
BUILD_DIR = build

# Files
TARGET = libcapture.so
SRC = $(SRC_DIR)/capture.c
HEADER = $(SRC_DIR)/capture.h
SDK_HEADER = $(LIB_DIR)/xdtusb.h

# Include path for xdtusb.h
INCLUDES = -I$(LIB_DIR)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC) $(HEADER) $(SDK_HEADER)
	@echo "Building $(TARGET)..."
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/lib..."
	sudo cp $(TARGET) /usr/local/lib/
	sudo ldconfig
	@echo "Install complete"

# Help target
help:
	@echo "Western Blot Capture Library Build"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build libcapture.so (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install library to /usr/local/lib"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build library"
	@echo "  make clean     # Clean build"
	@echo "  make install   # Install to system"
