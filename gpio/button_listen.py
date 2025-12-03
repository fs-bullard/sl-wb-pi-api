from gpiozero import LED, Button
from time import sleep
from signal import pause


def button_pressed(led: LED):
    print('Button Pressed')
    led.on()
    sleep(1)
    led.off()


led_pin = 17
button_pin = 27

led = LED(led_pin)
button = Button(button_pin)

button_pressed(led)


# button.when_activated = lambda: button_pressed(led)

# pause()


while True:
    print(button.value)


