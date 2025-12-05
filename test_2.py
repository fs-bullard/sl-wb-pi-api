import RPi.GPIO as GPIO
from gpiozero import LED


GPIO.setmode(GPIO.BOARD)

GPIO.setup(13, GPIO.IN)

led = LED(17)
strip = LED(22)


while True:
    if GPIO.input(13):
        led.on()
        strip.off()
    else:
        led.off()
        strip.on()
