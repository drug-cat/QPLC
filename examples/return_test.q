def start_conditional(cond):
    # Test return: inside a user function, statements after return are not executed
    outputs[0] = cond
    if not cond:
        return
    outputs[1] = True

def main():
    # Called twice: once with cond=False (return triggers) and once True
    start_conditional(False)
    start_conditional(True)
