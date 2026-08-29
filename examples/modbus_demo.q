def main():
    # A small demo for connecting over Modbus TCP:
    # coil enable -> coil motor_run
    # the speed register is derived from the coil value
    motor_run = enable and not stop_button
    speed = base_speed
    if motor_run:
        speed = base_speed + offset
