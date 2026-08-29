def main():
    # Rising edge detection: active only on the False->True transition
    if rising_edge(start_button):
        lamp = True

    # Falling edge detection
    if falling_edge(stop_button):
        valve = False

    # Edge inside a logical expression
    motor_run = rising_edge(sensor) or enable