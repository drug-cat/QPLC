def main():
    for i in range(3):
        if i == 0:
            motor_run = True
        elif i == 1:
            cooling_valve = True
        else:
            motor_run = False