#!/bin/bash
#
# USB Gadget Setup for Pi Zero 2W
#
# This script configures the Pi Zero 2W to act as a USB gadget device,
# enabling networking over USB connection with static IP 192.168.7.2.
#
# Run this script once on the Pi Zero 2W:
#   chmod +x setup_usb_gadget.sh
#   sudo ./setup_usb_gadget.sh
#
# After running, reboot the Pi for changes to take effect.

set -e  # Exit on any error

echo "============================================"
echo "USB Gadget Setup for Pi Zero 2W"
echo "============================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root (use sudo)"
    exit 1
fi

# Check if running on Raspberry Pi
if [ ! -f /proc/device-tree/model ]; then
    echo "WARNING: Cannot detect Raspberry Pi model"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo "Step 1: Configuring boot files..."

# Determine boot directory (Bookworm uses /boot/firmware, older uses /boot)
if [ -d /boot/firmware ]; then
    BOOT_DIR="/boot/firmware"
else
    BOOT_DIR="/boot"
fi

echo "  Using boot directory: $BOOT_DIR"

# 1. Enable dwc2 overlay in config.txt
CONFIG_FILE="$BOOT_DIR/config.txt"
if ! grep -q "^dtoverlay=dwc2" "$CONFIG_FILE"; then
    echo "dtoverlay=dwc2" >> "$CONFIG_FILE"
    echo "  ✓ Added dwc2 overlay to $CONFIG_FILE"
else
    echo "  ✓ dwc2 overlay already present in $CONFIG_FILE"
fi

# 2. Enable dwc2 and g_ether in cmdline.txt
CMDLINE_FILE="$BOOT_DIR/cmdline.txt"
if ! grep -q "modules-load=dwc2,g_ether" "$CMDLINE_FILE"; then
    # Backup original cmdline.txt
    cp "$CMDLINE_FILE" "${CMDLINE_FILE}.backup"
    # Add modules-load after rootwait
    sed -i 's/rootwait/rootwait modules-load=dwc2,g_ether/' "$CMDLINE_FILE"
    echo "  ✓ Updated $CMDLINE_FILE"
    echo "  ✓ Backup saved to ${CMDLINE_FILE}.backup"
else
    echo "  ✓ Modules already loaded in $CMDLINE_FILE"
fi

echo ""
echo "Step 2: Configuring network interface..."

# 3. Configure static IP on usb0 interface
INTERFACES_FILE="/etc/network/interfaces.d/usb0"
cat > "$INTERFACES_FILE" <<EOF
# USB Gadget Networking Interface
allow-hotplug usb0
iface usb0 inet static
    address 192.168.7.2
    netmask 255.255.255.0
EOF

echo "  ✓ Created $INTERFACES_FILE with static IP 192.168.7.2"

echo ""
echo "Step 3: Enabling modules..."

# 4. Ensure modules are loaded on boot
MODULES_FILE="/etc/modules"
for module in dwc2 g_ether; do
    if ! grep -q "^$module" "$MODULES_FILE"; then
        echo "$module" >> "$MODULES_FILE"
        echo "  ✓ Added $module to $MODULES_FILE"
    else
        echo "  ✓ $module already in $MODULES_FILE"
    fi
done

echo ""
echo "============================================"
echo "USB Gadget Setup Complete!"
echo "============================================"
echo ""
echo "Configuration summary:"
echo "  • Pi USB IP: 192.168.7.2"
echo "  • Host IP will be: 192.168.7.1"
echo "  • Interface: usb0"
echo ""
echo "IMPORTANT: Reboot required for changes to take effect!"
echo ""
echo "After reboot:"
echo "  1. Connect Pi to laptop via USB data port"
echo "  2. Wait ~30 seconds for Pi to boot"
echo "  3. From laptop, test connection:"
echo "     ping 192.168.7.2"
echo "  4. Access API at:"
echo "     http://192.168.7.2:5000"
echo ""
read -p "Reboot now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Rebooting..."
    reboot
else
    echo "Remember to reboot before using USB gadget mode!"
fi
