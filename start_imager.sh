#!/bin/bash
printf '%*s\n' 50 | tr ' ' '-'
date -I'seconds'
echo 'Starting Imager Server'
printf '%*s\n' 50 | tr ' ' '-'
echo

# Set directory
cd sl-wb-pi-api

# Kill process on port 5000 if running
sudo kill -9 $(sudo lsof -t -i:5000)

# Start Flask server
sudo .venv/bin/python -m src.flask_server &

exit 0