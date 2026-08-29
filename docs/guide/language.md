# Language Reference

QPLC is a Python-like DSL for Siemens S7-1200 PLCs that compiles to **Ladder Logic** and **SCL**.

## Program structure

Every program must define a `main()` function. Additional user functions are allowed and called as standalone statements.

```python
def main():
    motor_run = start_button and not stop_button
```

## Assignments

```python
# Simple assignment (boolean or numeric)
motor_run = start_button and not stop_button
speed     = base_speed + 100

# Indexed assignment (arrays)
outputs[0] = inputs[0] and enable
analog_outputs[1] = analog_inputs[1] * SCALE
```

## Operators (lowest to highest precedence)

| Operator | Meaning |
|----------|---------|
| `or`, `xor` | logical OR / exclusive OR |
| `and` | logical AND |
| `not` | logical negation |
| `trueExpr if cond else falseExpr` | Python-style ternary |
| `== != < > <= >=` | comparison (yields BOOL) |
| `+ -` | addition, subtraction |
| `* / %` | multiplication, division, remainder |
| `-x` | unary minus |

## Conditionals

```python
if temperature > MAX_TEMP:
    cooling_valve = True
elif temperature > MIN_TEMP:
    cooling_valve = True
else:
    cooling_valve = False
```

::: warning
`if`/`while`/`for` statements and function calls cannot appear directly inside an `if` branch (a compile error). Only assignments and `break`/`continue` are allowed there.
:::

## for loops

```python
for i in range(4):
    outputs[i] = inputs[i] and enable
```

`for` loops are **unrolled at compile time**: each iteration emits separate ladder rungs so absolute array addressing is possible. `range(n)` iterates `0 .. n-1`; `range(a, b)` iterates `a .. b-1`.

## while loops with break/continue

```python
count = 0
while loop_condition:
    count = count + 1
    if not enable:
        continue        # jump back to the top of the loop
    outputs[0] = True
    if count >= MAX_COUNT:
        break           # exit the loop
```

`while` is implemented with `label`/`jmp` in ladder. The simulator executes cross-network jumps with a safety cap of 100,000 steps per scan.

## Comments

```python
# single-line comment

/* block comment
   spanning multiple lines */
```

## Constants `[constants]`

Defined in `conf.qplc` and substituted as literals in the code:

```ini
[constants]
MAX_TEMP = 80.0
START_DELAY = T#2s
MAX_COUNT = 3
```

```python
if temperature > MAX_TEMP:
    timer_output = TON(start_button, START_DELAY)
```

::: tip
Constants are **read-only** — assigning to them is a compile error.
:::

## Ternary operator

```python
# trueExpr when cond is true, otherwise falseExpr
outputs[0] = True if enable else False
outputs[1] = (speed > 50) if enable else (speed < 10)
```

In a boolean context the compiler emits `DNF(cond) × DNF(trueExpr)` ladder terms.

## Early return

```python
def main():
    outputs[0] = enable
    if not enable:
        return            # early exit from main (equivalent to EXIT in main)
    outputs[1] = True
```

`return` is allowed in `main()` and in other user functions.

## Built-in functions

### Math functions (IEC 61131-3)

| QPLC name | IEC name | Args | Description |
|-----------|----------|------|-------------|
| `min(a, b)` | `MIN` | 2 | minimum |
| `max(a, b)` | `MAX` | 2 | maximum |
| `abs(x)` | `ABS` | 1 | absolute value |
| `clamp(x, lo, hi)` | `LIMIT` | 3 | clamp to range `[lo, hi]` |
| `sel(g, a, b)` | `SEL` | 3 | select between `a`/`b` based on `g` |
| `mux(k, ...)` | `MUX` | 2+ | multiplexer |

```python
outputs[0] = max(base_speed, offset) > 50
outputs[1] = clamp(speed, 0, 100)
```

In SCL the output maps directly to the IEC names.

### Timers (BOOL output)

| QPLC name | Standard IEC name | Parameters | Description |
|-----------|-------------------|------------|-------------|
| `on_delay(x, T#)` | `TON(x, T#)` | BOOL input, TIME | turn-on delay |
| `off_delay(x, T#)` | `TOF(x, T#)` | BOOL input, TIME | turn-off delay |
| `pulse(x, T#)` | `TP(x, T#)` | BOOL input, TIME | fixed-duration pulse |

```python
motor_run = TON(start_button, T#5s)      # equivalent to on_delay
valve     = TOF(stop_button, T#3s)       # equivalent to off_delay
```

### Counters (BOOL output)

| QPLC name | IEC name | Parameters | Description |
|-----------|----------|------------|-------------|
| `count_up(i, r, pv)` | `CTU(i, r, pv)` | input, reset, target | count up |
| `count_down(i, l, pv)` | `CTD(i, l, pv)` | input, load, initial | count down |
| `count_updown(u, d, r, l, pv)` | `CTUD(u, d, r, l, pv)` | up, down, reset, load, target | bidirectional |

```python
done = CTU(sensor, reset, 3)
```

### Edge detection

| Function | Description |
|----------|-------------|
| `rising_edge(x)` | True for one scan on the False→True transition |
| `falling_edge(x)` | True for one scan on the True→False transition |

```python
if rising_edge(start_button):
    lamp = True
```

::: tip
Negating an edge yields the opposite edge: `not rising_edge(x)` is equivalent to `falling_edge(x)`.
:::

## User functions

```python
def start_motor(cond):
    motor_run = cond and not emergency

def main():
    start_motor(start_button)    # called as a statement
```

- Parameters are **untyped** (usable as BOOL or numeric)
- The function body is **inlined** at the call site (max depth 8)
- Functions have no return value (effects happen through global variables)
- Definition order does not matter

## Configuration file (conf.qplc)

```ini
[hardware]
cpu = S7-1214C
ip = 192.168.0.1

[constants]
MAX_TEMP = 80.0

[io]
start_button = I0.0:BOOL
motor_run = Q0.0:BOOL
temperature = IW64:REAL
speed = QW80:INT
count = MW10:INT
inputs = I0.0:BOOL[4]       # 4-element array
```

### Addressing

- `I0.0` — digital input
- `Q0.0` — digital output
- `MW10` — word memory
- `IW64` / `QW80` — word input/output

Arrays use automatic stride addressing: BOOL = 1 bit, INT = 2 bytes, REAL/TIME/DINT = 4 bytes.

## Compiler usage

```
qplc <conf.qplc> <source.q> [-o output.xml] [-s output.scl] [--tokens] [--ast]
```

| Flag | Description |
|------|-------------|
| `-o file.xml` | write Ladder XML to a file (preferred over shell redirection) |
| `-s file.scl` | generate SCL for TIA Portal V19 |
| `--tokens` | print lexer tokens (debug) |
| `--ast` | print the parse tree (debug) |

## Importing SCL into TIA Portal V19

1. Compile with `-s`: `qplc conf.qplc prog.q -o out.xml -s out.scl`
2. In TIA Portal: **Project → External source files → Add new external source file**
3. Select and import the `.scl` file
4. The `QPLC_Main` FB is created — call it in OB1

::: tip
The SCL output uses absolute addressing (`%I0.0`, `%MW10`, ...) so no tag table is required.
:::

## Known limitations

- Compound statements (`if`/`while`/`for`/`call`) inside `if` branches are not allowed
- `for` loops require an explicit numeric bound (not a variable/constant)
- User functions have no return value (global variables only)
- SCL output requires manual testing in TIA Portal
