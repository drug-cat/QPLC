def main():
    # String literal + builtins: len() returns INT, print() emits sim output
    msg = "Hello, PLC!"
    length = len(msg)
    if len(msg) > 5:
        lamp = True
    prefix = msg[0:5]      # slice substring
    print("Current message: ")
    print(msg)
