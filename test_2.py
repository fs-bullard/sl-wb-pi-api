import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)

GPIO.setup(13, GPIO.IN)



while True:
    if GPIO.input(13):
        print(True)
    else:
        print(False)
