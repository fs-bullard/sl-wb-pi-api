from gpiozero import PWMLED
from time import sleep

class LED:
    def __init__(self, pin: int, val: float = 0.01):
        self.led = PWMLED(pin)
        self.val = val

    def set_on(self, on: bool):
        if on:
            self.led.on()
        else:
            self.led.off()

    def set_val(self, val: float):
        self.led.value = val
        self.val = val

    def get_val(self):
        return self.val
    

if __name__ == '__main__':
    led_pin = 22
    led = LED(led_pin)
    led.on()

    dv = 0.1

    val = 1
    while val >= 0:
        print(val)
        led.set_val(val)
        val -= dv 
        sleep(0.1)

    while True:
        val = float(input('Enter value: ').strip())
        print(f'Setting val to {val}')
        led.set_val(val)


