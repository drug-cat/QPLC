def main():
    motor_run = on_delay(start_button, T#5s)
    valve = off_delay(stop_button, T#3s)
    lamp = pulse(emergency, T#2s)