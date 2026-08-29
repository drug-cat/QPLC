/* User functions: define modular subroutines
   Parameters are untyped and substituted at the call site */

def start_motor(cond):
    motor_run = cond and not emergency

def stop_motor():
    motor_run = False

def monitor():
    if temperature > MAX_TEMP:
        cooling_valve = True
    else:
        cooling_valve = False

def main():
    # Called as a standalone statement (CallStmt)
    start_motor(start_button)
    stop_motor()
    monitor()

    # A loop with a conditional exit via a helper function
    count = 0
    while running:
        count = count + 1
        start_motor(enable)