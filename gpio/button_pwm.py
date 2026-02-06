from gpiozero import PWMLED
import RPi.GPIO as GPIO

from time import sleep


class Button:
    def __init__(self, pin):
        self.pin = pin

    def pressed(self):
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(self.pin, GPIO.IN)
        return GPIO.input(self.pin)
    
    def wait_for_press(self):
        GPIO.add_event_detect(self.pin, GPIO.RISING)
        if GPIO.event_detected(self.pin):
            return True

class ButtonWithLED:
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
