from gpiozero import PWMLED
from time import sleep

pin = 14
led = PWMLED(pin)

led.value = 1
sleep(1)
led.value = 0
sleep(1)
led_value = 1
sleep(1)
led_value = 0

led.off()