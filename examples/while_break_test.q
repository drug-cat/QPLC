def main():
    # while with break and continue
    count = 0
    while loop_condition:
        count = count + 1

        # continue: jump to the top of the loop when the input is inactive
        if not enable:
            continue

        outputs[0] = True

        # break: exit the loop when the counter reaches the limit
        if count >= MAX_COUNT:
            break

    # Special case: while True with break — must always have a break
    tick_count = 0
    while True:
        tick_count = tick_count + 1
        if sensor:
            break