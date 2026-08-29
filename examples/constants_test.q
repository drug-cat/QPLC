def main():
    # Use constants defined in the [constants] section of conf.qplc
    if temperature > MAX_TEMP:
        cooling_valve = True

    # Time constant
    timer_output = TON(start_button, START_DELAY)

    # Constant in a comparison and calculation
    if count >= MAX_COUNT:
        done = True

    speed = BASE_SPEED + 10