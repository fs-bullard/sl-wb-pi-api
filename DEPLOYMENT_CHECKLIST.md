# Deployment Checklist

Use this checklist when deploying to the Raspberry Pi Zero 2W.

## Pre-Deployment (On Development Machine)

- [ ] Commit all changes to git
- [ ] Push to remote repository
- [ ] Verify `library_examples/xdtusb.h` is in repo
- [ ] Note: `libxdtusb.so` binary should be copied separately (not in git)

## Deployment to Pi Zero 2W

### 1. Initial Setup

- [ ] SSH into Pi: `ssh freddie@<pi-ip>`
- [ ] Update system: `sudo apt-get update && sudo apt-get upgrade`
- [ ] Install git if needed: `sudo apt-get install git`

### 2. Clone Repository

```bash
cd /home/freddie
git clone <your-repo-url> sl-wb-pi-api
cd sl-wb-pi-api
```

- [ ] Repository cloned successfully
- [ ] Confirm files present: `ls -la`

### 3. Copy libxdtusb Library

Copy the vendor library file to the Pi:

```bash
# Option A: From local machine (run on laptop)
scp /path/to/libxdtusb.so freddie@<pi-ip>:/home/freddie/sl-wb-pi-api/library_examples/

# Option B: From USB drive on Pi
sudo mount /dev/sda1 /mnt
cp /mnt/libxdtusb.so /home/freddie/sl-wb-pi-api/library_examples/
sudo umount /mnt
```

- [ ] `library_examples/libxdtusb.so` present
- [ ] Verify: `ls -lh library_examples/libxdtusb.so`

### 4. Run Installation

```bash
cd /home/freddie/sl-wb-pi-api
chmod +x scripts/*.sh
./scripts/install.sh
```

**Expected output:**
- ✓ System dependencies installed
- ✓ libxdtusb installed
- ✓ libcapture.so built successfully
- ✓ Virtual environment created
- ✓ Python dependencies installed
- ✓ Service installed and enabled
- ✓ Service is running!

- [ ] Installation completed without errors
- [ ] Service status shows "active (running)"

### 5. Verify Installation

#### Check Service Status
```bash
sudo systemctl status capture
```
- [ ] Status: `active (running)`
- [ ] No error messages in recent logs

#### Check Logs
```bash
sudo journalctl -u capture -n 20
```
- [ ] "Device initialized successfully" message present
- [ ] "Flask server on 0.0.0.0:5000" message present
- [ ] No error or warning messages

#### Test Health Endpoint
```bash
curl http://localhost:5000/health
```
Expected: `{"status":"ready","device":"connected"}`

- [ ] Health endpoint returns 200 OK
- [ ] Device status is "ready"

### 6. Test Camera Capture

#### Quick Test (100ms)
```bash
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 100}' \
  --output test_100ms.raw
```

- [ ] Command completes without error
- [ ] File created: `test_100ms.raw`
- [ ] File size is reasonable (width × height × 2 bytes)

#### Check Headers
```bash
curl -i -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 100}' \
  --output test.raw 2>&1 | grep "X-Frame"
```

Expected headers:
- `X-Frame-Width: <width>`
- `X-Frame-Height: <height>`
- `X-Pixel-Size: 2`
- `X-Exposure-Ms: 100`

- [ ] All headers present
- [ ] Width and height match expected sensor resolution

#### Long Exposure Test (10000ms)
```bash
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 10000}' \
  --output test_10000ms.raw
```

- [ ] Command completes (takes ~10+ seconds)
- [ ] File created successfully
- [ ] File size matches 100ms test

### 7. Network Access Test

From your laptop/desktop, test remote access:

```bash
# Replace <pi-ip> with actual Pi IP address
curl http://<pi-ip>:5000/health
```

- [ ] Health endpoint accessible from network
- [ ] Response time reasonable (<1 second)

```bash
curl -X POST http://<pi-ip>:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 500}' \
  --output capture_remote.raw
```

- [ ] Capture works over network
- [ ] Transfer completes successfully

### 8. Service Persistence Test

```bash
# Reboot Pi
sudo reboot

# Wait for Pi to come back up, then SSH in
ssh freddie@<pi-ip>

# Check service auto-started
sudo systemctl status capture
```

- [ ] Service started automatically after reboot
- [ ] Health endpoint responds after reboot

### 9. Error Handling Tests

#### Invalid Exposure (Too Low)
```bash
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 5}'
```
- [ ] Returns 400 error
- [ ] Error message mentions valid range

#### Invalid Exposure (Too High)
```bash
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 20000}'
```
- [ ] Returns 400 error
- [ ] Error message mentions valid range

#### Missing Parameter
```bash
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{}'
```
- [ ] Returns 400 error
- [ ] Error message mentions missing exposure_ms

### 10. Performance Verification

Monitor resource usage during capture:

```bash
# In one terminal, monitor resources
htop

# In another terminal, run capture
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 1000}' \
  --output test.raw
```

Observations:
- [ ] Memory usage stays under 100MB
- [ ] CPU usage spike during transfer, low otherwise
- [ ] No memory leaks (memory returns to baseline)

### 11. Load Test (Optional)

```bash
# Run 10 consecutive captures
for i in {1..10}; do
  echo "Capture $i..."
  curl -X POST http://localhost:5000/capture \
    -H 'Content-Type: application/json' \
    -d '{"exposure_ms": 100}' \
    --output test_$i.raw
done
```

- [ ] All captures succeed
- [ ] File sizes consistent
- [ ] No memory growth (check with htop)
- [ ] No errors in logs

## Troubleshooting

If any step fails, consult:
1. Service logs: `sudo journalctl -u capture -n 50`
2. README_NEW.md troubleshooting section
3. IMPLEMENTATION_SUMMARY.md for technical details

## Common Issues

### "Device not initialized"
- Camera not connected when service started
- USB connection issue
- Solution: Connect camera, restart service

### "ACCESS" error in logs
- USB permissions not configured
- Solution: `sudo ./scripts/fix_usb_permissions.sh` then reconnect camera or reboot

### "libcapture.so not found"
- Build failed during installation
- Solution: `cd /home/freddie/sl-wb-pi-api && make clean && make`

### "libxdtusb.so: cannot open shared object"
- Library not installed correctly
- Solution: `sudo cp library_examples/libxdtusb.so /usr/local/lib/ && sudo ldconfig`

### Capture timeout
- Camera hardware issue
- Long exposure time (normal for 10000ms)
- Solution: Check camera connection, verify with shorter exposure

## Post-Deployment

- [ ] Document Pi IP address: ________________
- [ ] Test from client application
- [ ] Set up monitoring/alerts (optional)
- [ ] Configure firewall rules if needed
- [ ] Schedule regular reboots if desired

## Development Workflow

For code updates after initial deployment:

```bash
# On Pi
cd /home/freddie/sl-wb-pi-api
git pull
./scripts/deploy.sh
```

- [ ] Pull updates work correctly
- [ ] Deploy script rebuilds and restarts
- [ ] Service comes back up successfully

## Sign-Off

Deployment completed by: ________________

Date: ________________

Pi IP Address: ________________

Camera Model: ________________

Notes:
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
