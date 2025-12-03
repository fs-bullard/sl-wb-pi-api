from gpiozero import PWMLED
from time import sleep

class LED:
    def __init__(self, pin: int):
        self.led = PWMLED(pin)

    def set_on(self, on: bool):
        if on:
            self.led.on()
        else:
            self.led.off()
    

if __name__ == '__main__':
    led_pin = 17
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

