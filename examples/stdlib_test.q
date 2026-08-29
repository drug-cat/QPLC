def main():
    # Standard IEC + QPLC math functions in a boolean context
    outputs[0] = max(base_speed, offset) > 50
    outputs[1] = min(base_speed, offset) < 10
    outputs[2] = abs(offset) > 0
    outputs[3] = (clamp(base_speed, 0, 100) >= 0) and enable
