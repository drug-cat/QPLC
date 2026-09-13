struct Mode:
    code: INT
    label: STRING

def main():
    m = Mode(code=2, label="AUTO")
    match m.code:
        case 0:
            lamp = True
        case 1:
            done = True
        case 2:
            loop_output = True
        case _:
            valve = True
    try:
        if m.code < 0:
            raise ValueError("negative mode")
    except ValueError:
        valve = True
    finally:
        motor_run = True
