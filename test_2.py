from gpiozero import LED


led = LED(17)
strip = LED(22)


while True:
    if GPIO.input(13):
        led.on()
        strip.off()
    else:
        led.off()
        strip.on()
