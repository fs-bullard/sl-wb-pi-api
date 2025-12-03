from gpiozero import LED, Button
from time import sleep

def button_pressed(led: LED):
    led.on()
    sleep(1)
    led.off()


led_pin = 14
button_pin = 2

led = LED(led_pin)
button = Button(button_pin)

button.when_pressed = lambda: button_pressed(led)

