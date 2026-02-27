"""
Startup LED flash — run at boot by gpio_startup.service.

Flashes green until /tmp/capture_ready appears (written by flask_server.py
once the API is ready), then exits. The Flask server then sets the LED to
solid green itself.
"""

import os
from time import sleep

from gpio.rgb_led import RGBLED

READY_FILE = '/tmp/capture_ready'
POLL_INTERVAL = 0.5

led = RGBLED(r_pin=17, g_pin=27, b_pin=22)

try:
    led.flash_green()
    while not os.path.exists(READY_FILE):
        sleep(POLL_INTERVAL)
finally:
    led.off()
    led.close()
