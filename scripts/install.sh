#!/bin/bash
#
# Installation Script for Western Blot Capture API
#
# Installs dependencies, builds C library, and sets up Python environment
# for Raspberry Pi Zero 2W
#
# Usage:
#   cd /home/freddie/sl-wb-pi-api
#   ./scripts/install.sh

set -e  # Exit on any error

echo "============================================"
echo "Western Blot Capture API Installation"
echo "============================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Project directory: $PROJECT_DIR"
echo ""

# Check if running from correct directory
if [ ! -f "$PROJECT_DIR/Makefile" ]; then
    echo "ERROR: Makefile not found in $PROJECT_DIR"
    echo "Please run this script from the project root directory"
    exit 1
fi

# Step 1: Install system dependencies
echo "Step 1: Installing system dependencies..."
echo ""

# Update package list
sudo apt-get update

# Install build essentials
sudo apt-get install -y build-essential gcc make

# Install libxdtusb dependencies
sudo apt-get install -y libcurl4-openssl-dev libusb-1.0-0-dev

# Install Python dependencies
sudo apt-get install -y python3 python3-pip python3-venv

echo "  ✓ System dependencies installed"
echo ""

# Step 2: Install libxdtusb if not present
echo "Step 2: Checking for libxdtusb..."
echo ""

if ldconfig -p | grep -q libxdtusb; then
    echo "  ✓ libxdtusb already installed"
else
    echo "  libxdtusb not found in system"

    # Check if library files exist in libraries
    if [ -f "$PROJECT_DIR/libraries/libxdtusb.so" ]; then
        echo "  Installing libxdtusb from libraries..."
        sudo cp "$PROJECT_DIR/libraries/libxdtusb.so" /usr/local/lib/
        sudo ldconfig
        echo "  ✓ libxdtusb installed"
    else
        echo "  ERROR: libxdtusb.so not found in libraries/"
        echo "  Please place the libxdtusb library files in libraries/"
        exit 1
    fi
fi
echo ""

# Step 3: Build libcapture.so
echo "Step 3: Building libcapture.so..."
echo ""

cd "$PROJECT_DIR"
make clean
make

if [ ! -f "libcapture.so" ]; then
    echo "ERROR: libcapture.so build failed"
    exit 1
fi

echo "  ✓ libcapture.so built successfully"
echo ""

# Step 4: Create Python virtual environment
echo "Step 4: Setting up Python virtual environment..."
echo ""

VENV_DIR="$PROJECT_DIR/venv"

if [ -d "$VENV_DIR" ]; then
    echo "  Virtual environment already exists"
    read -p "  Remove and recreate? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf "$VENV_DIR"
        python3 -m venv "$VENV_DIR"
        echo "  ✓ Virtual environment recreated"
    else
        echo "  Using existing virtual environment"
    fi
else
    python3 -m venv "$VENV_DIR"
    echo "  ✓ Virtual environment created"
fi
echo ""

# Step 5: Install Python dependencies
echo "Step 5: Installing Python dependencies..."
echo ""

source "$VENV_DIR/bin/activate"

# Upgrade pip
pip install --upgrade pip -q

# Install Flask (minimal dependencies only)
pip install flask

echo "  ✓ Python dependencies installed"
deactivate
echo ""

# Step 6: Install systemd service
echo "Step 6: Installing systemd service..."
echo ""

SERVICE_FILE="$PROJECT_DIR/config/capture.service"

if [ ! -f "$SERVICE_FILE" ]; then
    echo "ERROR: Service file not found: $SERVICE_FILE"
    exit 1
fi

# Copy service file to systemd directory
sudo cp "$SERVICE_FILE" /etc/systemd/system/capture.service

# Reload systemd
sudo systemctl daemon-reload

# Enable service
sudo systemctl enable capture.service

echo "  ✓ Service installed and enabled"
echo ""

# Step 7: Start service
echo "Step 7: Starting service..."
echo ""

sudo systemctl start capture.service
sleep 2

# Check service status
if sudo systemctl is-active --quiet capture.service; then
    echo "  ✓ Service is running!"
else
    echo "  ✗ Service failed to start"
    echo ""
    echo "Viewing last 20 lines of log:"
    sudo journalctl -u capture -n 20 --no-pager
    exit 1
fi
echo ""

# Step 8: Display summary
echo "============================================"
echo "Installation Complete!"
echo "============================================"
echo ""
echo "Service Status:"
sudo systemctl status capture.service --no-pager -l | head -n 10
echo ""
echo "API Endpoints:"
echo "  GET  http://localhost:5000/health"
echo "  POST http://localhost:5000/capture"
echo "  POST http://localhost:5000/shutdown"
echo ""
echo "Useful Commands:"
echo "  • View logs:"
echo "    sudo journalctl -u capture -f"
echo ""
echo "  • Restart service:"
echo "    sudo systemctl restart capture"
echo ""
echo "  • Stop service:"
echo "    sudo systemctl stop capture"
echo ""
echo "  • Check status:"
echo "    sudo systemctl status capture"
echo ""
echo "  • Rebuild library:"
echo "    cd $PROJECT_DIR && make clean && make"
echo ""
echo "  • Test capture:"
echo "    curl -X POST http://localhost:5000/capture -H 'Content-Type: application/json' -d '{\"exposure_ms\": 100}' --output test.raw"
echo ""
