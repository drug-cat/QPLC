# Module system test: imports utils.q and uses its functions
import "utils.q" as utils

def main():
    # Call an imported function — it is inlined like local functions
    utils.add(base_speed, 100)
    speed = result

    utils.scale(base_speed, 2)
    doubled = result