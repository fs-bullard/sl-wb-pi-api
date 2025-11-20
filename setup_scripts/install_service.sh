#!/bin/bash
#
# Camera API Service Installation Script
#
# This script installs the Camera REST API as a systemd service on Pi Zero 2W.
#
# Prerequisites:
#   - Python 3.9 or newer installed
#   - Git repository cloned to ~/camera-api
#
# Usage:
#   cd ~/camera-api
#   chmod +x setup_scripts/install_service.sh
#   ./setup_scripts/install_service.sh

set -e  # Exit on any error

echo "============================================"
echo "Camera API Service Installation"
echo "============================================"
echo ""

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Project directory: $PROJECT_DIR"
echo ""

# Check if running from correct directory
if [ ! -f "$PROJECT_DIR/camera_api.py" ]; then
    echo "ERROR: camera_api.py not found in $PROJECT_DIR"
    echo "Please run this script from the project root directory"
    exit 1
fi

# Check Python version
echo "Step 1: Checking Python version..."
PYTHON_CMD="python3"
if ! command -v $PYTHON_CMD &> /dev/null; then
    echo "ERROR: python3 not found"
    exit 1
fi

PYTHON_VERSION=$($PYTHON_CMD --version | awk '{print $2}')
echo "  ✓ Found Python $PYTHON_VERSION"
echo ""

# Create virtual environment
echo "Step 2: Creating virtual environment..."
VENV_DIR="$PROJECT_DIR/venv"

if [ -d "$VENV_DIR" ]; then
    echo "  Virtual environment already exists"
    read -p "  Remove and recreate? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf "$VENV_DIR"
        $PYTHON_CMD -m venv "$VENV_DIR"
        echo "  ✓ Virtual environment recreated"
    else
        echo "  Using existing virtual environment"
    fi
else
    $PYTHON_CMD -m venv "$VENV_DIR"
    echo "  ✓ Virtual environment created"
fi
echo ""

# Activate virtual environment and install dependencies
echo "Step 3: Installing Python dependencies..."
source "$VENV_DIR/bin/activate"

# Upgrade pip
pip install --upgrade pip -q

# Install requirements
if [ -f "$PROJECT_DIR/requirements.txt" ]; then
    pip install -r "$PROJECT_DIR/requirements.txt"
    echo "  ✓ Dependencies installed from requirements.txt"
else
    echo "ERROR: requirements.txt not found"
    exit 1
fi

deactivate
echo ""

# Create config directory
echo "Step 4: Creating configuration directory..."
CONFIG_DIR="$PROJECT_DIR/config"
mkdir -p "$CONFIG_DIR"
echo "  ✓ Created $CONFIG_DIR"
echo ""

# Install systemd service
echo "Step 5: Installing systemd service..."

SERVICE_FILE="$PROJECT_DIR/systemd/camera-api.service"
if [ ! -f "$SERVICE_FILE" ]; then
    echo "ERROR: Service file not found: $SERVICE_FILE"
    exit 1
fi

# Update service file with actual project directory
TEMP_SERVICE="/tmp/camera-api.service"
sed "s|/home/pi/camera-api|$PROJECT_DIR|g" "$SERVICE_FILE" > "$TEMP_SERVICE"

# Copy to systemd directory
sudo cp "$TEMP_SERVICE" /etc/systemd/system/camera-api.service
rm "$TEMP_SERVICE"

echo "  ✓ Service file installed"
echo ""

# Reload systemd and enable service
echo "Step 6: Enabling service..."
sudo systemctl daemon-reload
sudo systemctl enable camera-api.service
echo "  ✓ Service enabled (will start on boot)"
echo ""

# Start service
echo "Step 7: Starting service..."
sudo systemctl start camera-api.service
sleep 2
echo ""

# Check service status
echo "Step 8: Checking service status..."
if sudo systemctl is-active --quiet camera-api.service; then
    echo "  ✓ Service is running!"
else
    echo "  ✗ Service failed to start"
    echo ""
    echo "Viewing last 20 lines of log:"
    sudo journalctl -u camera-api -n 20 --no-pager
    exit 1
fi
echo ""

# Get IP addresses
echo "============================================"
echo "Installation Complete!"
echo "============================================"
echo ""
echo "Service Status:"
sudo systemctl status camera-api.service --no-pager -l | head -n 10
echo ""
echo "Network Information:"
echo "  USB IP (if configured): 192.168.7.2"

# Try to get WiFi IP
WIFI_IP=$(hostname -I | awk '{print $1}')
if [ -n "$WIFI_IP" ]; then
    echo "  WiFi IP: $WIFI_IP"
fi
echo ""

echo "API Endpoints:"
echo "  http://192.168.7.2:5000 (via USB)"
if [ -n "$WIFI_IP" ]; then
    echo "  http://$WIFI_IP:5000 (via WiFi)"
fi
echo ""

echo "Useful Commands:"
echo "  • View logs:"
echo "    sudo journalctl -u camera-api -f"
echo ""
echo "  • Restart service:"
echo "    sudo systemctl restart camera-api"
echo ""
echo "  • Stop service:"
echo "    sudo systemctl stop camera-api"
echo ""
echo "  • Check status:"
echo "    sudo systemctl status camera-api"
echo ""
echo "  • Pull updates and restart:"
echo "    cd $PROJECT_DIR && git pull && sudo systemctl restart camera-api"
echo ""
