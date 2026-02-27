from gpiozero import Button


class ShutdownButton:
    """
    Monitors a momentary pushbutton and fires a callback when held for
    hold_time seconds.

    GPIO3 is used here because it also wakes the Pi from a halted state
    (hardware feature of the BCM2837/2711 SCL pin).
    """

    def __init__(self, pin: int = 3, hold_time: float = 3.0, on_held=None):
        self._button = Button(
            pin,
            pull_up=True,
            hold_time=hold_time,
            hold_repeat=False,
        )
        if on_held is not None:
            self._button.when_held = on_held

    def close(self):
        self._button.close()
