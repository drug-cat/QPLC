def main():
    # Standard IEC 61131-3 names (equivalent to on_delay/off_delay/pulse)
    motor_run = TON(start_button, T#5s)
    valve = TOF(stop_button, T#3s)
    lamp = TP(emergency, T#2s)

    # Standard counters
    done = CTU(sensor, reset, 3)
    finished = CTD(close_input, load, 5)
    result = CTUD(up_input, down_input, reset_updown, load_updown, 10)