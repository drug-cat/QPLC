def main():
    # Numeric arithmetic: addition, subtraction, multiplication, division, remainder
    speed = base_speed + offset
    temp = temperature * SCALE
    ratio = count / DIVISOR
    remainder = count % 2

    # Calculation in a comparison expression
    if count % MAX_COUNT == 0:
        done = True

    # Operator precedence: multiplication/division before addition/subtraction
    ratio = count + speed * 2 - BASE_SPEED / DIVISOR

    # Logical xor: exactly one active
    lamp = (motor_run xor cooling_valve)