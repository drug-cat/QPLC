# PLCSIM Advanced HIL demo
# Maps QPLC variables to a virtual S7-1500 in PLCSIM Advanced:
#   starts when "start_btn" (I-point) is True in the vPLC,
#   drives "motor_run" (Q-point) and integrates "speed_ref".
def main():
    motor_run = start_btn and not stop_btn
    if start_btn:
        speed_ref = BASE_SPEED
    else:
        speed_ref = 0
