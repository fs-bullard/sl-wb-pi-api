# LED pin 11
# button pin 13

from gpiozero import LED
import RPI.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)

GPIO.setup(13, GPIO.IN)

led = LED(17)

while True:
    if GPIO.input(13):
        led.on()
    else:
        led.off()
