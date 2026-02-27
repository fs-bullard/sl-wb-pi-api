from gpiozero import PWMLED


class RGBLED:
    """
    Common-anode RGB LED controller using three PWMLED channels.

    Common anode means the shared pin is connected to 3V3, so channels
    are active-low — active_high=False inverts the logic so that
    set_color(1, 0, 0) lights red as expected.
    """

    def __init__(self, r_pin: int, g_pin: int, b_pin: int):
        self.r = PWMLED(r_pin, active_high=False)
        self.g = PWMLED(g_pin, active_high=False)
        self.b = PWMLED(b_pin, active_high=False)

    def set_color(self, r: float, g: float, b: float):
        """Set RGB values in range 0.0-1.0."""
        self.r.value = r
        self.g.value = g
        self.b.value = b

    def solid_green(self):
        self.r.off()
        self.g.on()
        self.b.off()

    def flash_green(self, on_time: float = 0.3, off_time: float = 0.3):
        """Non-blocking green blink."""
        self.r.off()
        self.b.off()
        self.g.blink(on_time=on_time, off_time=off_time)

    def flash_red(self, on_time: float = 0.3, off_time: float = 0.3):
        """Non-blocking red blink."""
        self.g.off()
        self.b.off()
        self.r.blink(on_time=on_time, off_time=off_time)

    def flash_blue(self, on_time: float = 0.1, off_time: float = 0.1):
        """Non-blocking blue blink."""
        self.r.off()
        self.g.off()
        self.b.blink(on_time=on_time, off_time=off_time)

    def off(self):
        self.r.off()
        self.g.off()
        self.b.off()

    def close(self):
        self.r.close()
        self.g.close()
        self.b.close()
