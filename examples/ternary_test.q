def main():
    # Test the Python-style ternary operator in a boolean context
    outputs[0] = True if enable else False
    outputs[1] = (offset > 5) if enable else (offset < 0)
    outputs[2] = (not enable) or (offset == 0)
