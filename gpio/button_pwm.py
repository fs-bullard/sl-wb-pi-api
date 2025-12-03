from gpiozero import PWMLED, Button
from time import sleep

class ButtonLED:
    def __init__(self, led_pin, button_pin):
        self.led = PWMLED(led_pin)
        self.button = Button(button_pin)

    def led_on(self, on: bool):
        if on: 
            self.led.on()
        else:
            self.led.off()

if __name__ == '__main__':

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