#!/bin/bash
#
# Deployment Script for Western Blot Capture API
#
# Quick rebuild and restart for development workflow
#
# Usage:
#   cd /home/freddie/sl-wb-pi-api
#   ./scripts/deploy.sh

set -e  # Exit on any error

echo "============================================"
echo "Western Blot Capture API Deployment"
echo "============================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Project directory: $PROJECT_DIR"
echo ""

# Step 1: Stop service
echo "Step 1: Stopping service..."
sudo systemctl stop capture.service
echo "  ✓ Service stopped"
echo ""

# Step 2: Rebuild library
echo "Step 2: Rebuilding libcapture.so..."
cd "$PROJECT_DIR"
make clean
make

if [ ! -f "libcapture.so" ]; then
    echo "ERROR: libcapture.so build failed"
    exit 1
fi

echo "  ✓ Library rebuilt"
echo ""

# Step 3: Restart service
echo "Step 3: Restarting service..."
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

# Step 4: Display status
echo "============================================"
echo "Deployment Complete!"
echo "============================================"
echo ""
echo "Service Status:"
sudo systemctl status capture.service --no-pager -l | head -n 10
echo ""
echo "View live logs:"
echo "  sudo journalctl -u capture -f"
echo ""
