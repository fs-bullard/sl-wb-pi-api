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
    led_pin = 22
    led = PWMLED(led_pin)
    led.on()

    dv = 0.1

    val = 1
    while val >= 0:
        print(val)
        led.value = val
        val -= dv 
        sleep(0.1)

    while True:
        val = float(input('Enter value: ').strip())
        print(f'Setting val to {val}')
        led.value = val


