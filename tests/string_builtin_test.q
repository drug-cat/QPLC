def main():
    # String type: literals + len/print/assert builtins
    msg = "Hello, PLC!"
    length = len(msg)
    if len(msg) > 5:
        lamp = True
    prefix = msg[0:5]
    print(msg)
    print("length =", length)
    assert len(msg) > 3
    # String via builtins in config
    if type(enable) == "BOOL" and type(base_speed) == "INT":
        type_ok = True
