# struct_enum_test.q — verifies struct definition, struct literal, field access,
#                      struct assignment, and enum definitions.
struct Motor:
    name:      STRING
    speed:     INT
    running:   BOOL

enum State:
    Idle
    Running
    Fault

def main():
    m = Motor(name="M1", speed=0, running=False)
    m.speed = 100
    m.running = True
    outputs[0] = m.running
    speed = m.speed
    if m.speed > 0:
        cooling_valve = True
