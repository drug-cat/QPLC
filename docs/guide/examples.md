# Examples

All examples live in the [`examples/`](https://github.com/YOUR_USERNAME/QPLC/tree/main/examples) directory. Each `.q` file is a complete program that compiles against `conf.qplc`.

## The configuration file

```ini
[hardware]
cpu = S7-1214C
ip = 192.168.0.1

[constants]
MAX_TEMP = 80.0
START_DELAY = T#2s

[io]
start_button = I0.0:BOOL
motor_run = Q0.0:BOOL
temperature = IW64:REAL
inputs = I0.0:BOOL[4]
outputs = Q0.0:BOOL[4]
```

## Motor with on-delay timer

`examples/main.q` — a starter motor wired through an on-delay timer:

```python
def main():
    motor_run = on_delay(start_button, T#5s)
```

## Timers, counters, and edges

- `timer_test.q` — `TON` / `TOF` / `TP` timers
- `counter_test.q` — `CTU` / `CTD` / `CTUD` counters
- `edge_test.q` — `rising_edge` / `falling_edge`

```python
def main():
    motor_run = TON(start_button, T#5s)
    done = CTU(sensor, reset, 3)
    if rising_edge(start_button):
        lamp = True
```

## Math and IEC names

- `math_test.q` — arithmetic with constants
- `iec_names_test.q` — IEC vs QPLC function names (`TON` ≡ `on_delay`, ...)
- `stdlib_test.q` — `MIN` / `MAX` / `ABS` / `LIMIT`

```python
def main():
    outputs[1] = clamp(speed, 0, 100)
    outputs[0] = max(base_speed, offset) > 50
```

## Language features

- `ternary_test.q` — Python-style conditional expressions
- `return_test.q` — early `return` in functions
- `while_test.q` / `while_break_test.q` / `loop_test.q` — loops with `break`/`continue`
- `functions_test.q` — user-defined functions with inlining
- `array_test.q` / `array_int_test.q` — indexed arrays
- `constants_test.q` — `[constants]` substitution
- `test_all.q` — the golden test covering everything

## Modbus demo

`modbus_demo.q` pairs with the simulator's built-in Modbus TCP server. Compile it, run the simulator with Modbus enabled, and read coils/holding registers from any Modbus client.

```bash
./build/qplc examples/conf.qplc examples/modbus_demo.q -o modbus_demo.xml
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc modbus_demo.xml
```

See the [Modbus guide](./modbus) for a full walkthrough.

## Running all tests

```bash
bash tests/run_tests.sh
```

This compiles every example, verifies golden output, and checks semantic error reporting — 32 tests in total.
