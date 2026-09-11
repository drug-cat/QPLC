# Struct + Enum feature test
# Verifies user-defined types compile through the pipeline

struct Motor:
    name: STRING
    speed: INT
    running: BOOL

enum State:
    Idle
    Starting
    Running
    Fault

def main():
    # Struct literal
    m = Motor(name="M1", speed=0, running=False)

    # Field access in expressions
    # FieldAccessExpr: m.speed, m.running
    motor_run = m.running
    speed = m.speed

    # Struct field assignment
    m.speed = 100
    m.running = True

    # Enum variant (numeric encoding for now)
    if m.speed > 0:
        cooling_valve = True
    else:
        cooling_valve = False