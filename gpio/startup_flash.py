"""
Startup LED flash — run once at boot by gpio_startup.service (Type=oneshot).

Flashes green for 3 seconds to indicate the Pi has booted, then exits.
The capture service starts afterwards and sets the LED to solid green
once the Flask API is ready.
"""

from time import sleep

from gpio.rgb_led import RGBLED

led = RGBLED(r_pin=17, g_pin=27, b_pin=22)

try:
    led.flash_green()
    sleep(3)
finally:
    led.off()
    led.close()
