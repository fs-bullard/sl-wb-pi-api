from gpiozero import PWMLED
from time import sleep

led_pin = 14
led = PWMLED(led_pin)

while True:
    led.on()

    led.value = 1
    sleep(1)
    led.value = 0.5
    sleep(1)
    led_value = 1
    sleep(1)
    led_value = 0
    sleep(1)

    led.off()