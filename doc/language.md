# QPLC Language Reference — Stage 1

**Specification version:** 1.0 — proposed rewrite  
**Status:** Proposed implementation baseline; conformance tests must be run before final conformance is claimed  
**Purpose:** State-machine description language that compiles to SCL for Siemens PLCs  
**Scope of this version:** One file, one machine, absolute I/Q addressing, no imports, and no nested machines

---

## Important Note on the Status of This Document

This document does not merely copyedit QPLC 0.2. It makes several design decisions to remove execution-model and grammar inconsistencies. In particular:

- Assignment uses `:=`; `=` is reserved for comparison.
- The `byte` type and correct widths for `%IB`, `%IW`, and `%ID` addresses are defined.
- Every output starts each scan at its declared default. An output assignment in `on entry` has an effect only during that scan.
- Edge detection is allowed only in transition conditions, and the edge operand must have type `bool`.
- Every persistent data variable must have an explicit initial value. A `temp` variable is initialized to its type's default at the start of every scan.
- State transitions, timers, data lifetime, and edge detection are defined independently of incidental statement ordering in generated SCL.

Any feature not defined by this document is outside Stage 1. This document is not a functional-safety standard. QPLC does not replace a safety PLC, safety relay, emergency-stop circuit, or machine risk assessment.

## Table of Contents

1. Purpose and Design Principles
2. Terminology and Conformance
3. Program Structure
4. Lexical Rules
5. Declarations
6. Data Types, Literals, and Conversions
7. Physical Addressing
8. Expressions and Operator Precedence
9. State Machines, Assignments, and `on entry`
10. State Transitions and Edge Detection
11. Timers
12. Deterministic Scan Execution Model
13. Semantic Analysis and Errors
14. SCL Generation Rules
15. Complete Example
16. Valid and Invalid Examples
17. Minimum Conformance Tests
18. Features Outside Stage 1
19. Implementation Checklist
20. Target Technical References
21. Complete EBNF Grammar

---

## 1. Purpose and Design Principles

QPLC is a high-level language for describing the behavior of a state machine that compiles to SCL. The programmer describes states, outputs active in each state, transition conditions, and timed behavior. The compiler generates common implementation details such as the state variable, edge-detection memory, and timer instances.

### 1.1 Normative Principles

1. **Explicit state machine:** The active state is a language construct. The user does not manually declare an `INT` or `ENUM` variable for the machine's primary state model.
2. **Automatic edge detection:** `rises` and `falls` automatically manage the previous value of an expression.
3. **Deterministic output behavior:** Every output returns to its declared default at the beginning of every scan. An output remains non-default only if it is assigned again during that same scan.
4. **Explicit type conversion:** No implicit conversion between variables of different types is allowed. Numeric literals may receive a context-dependent type; this does not count as implicit conversion between variables.
5. **Explicit transition priority:** Transitions are checked from top to bottom. Only the first true transition is selected/scheduled.
6. **Execution semantics independent of generated formatting:** Program behavior follows the QPLC specification, not incidental ordering in generated SCL statements.
7. **Early error rejection:** Naming, type, address, and state-machine structure errors must be detected before final code generation.
8. **No safety claim:** Defaulting outputs alone does not make a machine safe.

### 1.2 Stage 1 Goals

Stage 1 is designed for small and medium-sized machines with one state machine and absolute I/Q addressing. It should be sufficient for controls such as conveyors, pump/motor sequences, filling cycles, simple manual/automatic modes, and process-fault handling, provided that safety requirements are implemented separately.

---

## 2. Terminology and Conformance

The words **MUST**, **MUST NOT**, **MAY**, and **SHOULD** have normative meanings:

- **MUST / MUST NOT:** A mandatory requirement of this specification.
- **MAY:** An optional feature a compiler may support. If it does not support the feature, it must report a clear error.
- **SHOULD:** Not mandatory, but recommended for readability or maintainability.

### 2.1 Core Terms

- **Scan:** One cyclic execution of the machine logic in the generated block.
- **Active state:** The state active at the beginning of the current scan.
- **State entry:** The logical moment when a transition's target becomes the active state at the beginning of the next scan. Initial startup also counts as entry into the initial state.
- **`on entry`:** A block executed only during the first scan of entry into a state.
- **Continuous assignment:** An assignment in the main state body that runs on every scan in which that state executes.
- **Rising edge (`rises`):** A change in a Boolean expression from `false` to `true` between consecutive samples.
- **Falling edge (`falls`):** A change in a Boolean expression from `true` to `false` between consecutive samples.
- **Output:** A variable mapped to the Q area and writable in QPLC only through that machine's permitted output logic.
- **Persistent data variable:** A `data` declaration without the `temp` keyword; its value is retained between scans.
- **Temporary data variable:** A `data` declaration with the `temp` keyword; it receives its type's default value at the start of each scan and is not intended to retain values between scans.

### 2.2 Compiler Conformance

A Stage 1-conforming compiler MUST:

1. Accept all valid inputs defined by this grammar.
2. Reject the invalid examples defined in this document.
3. Report a line number and, where possible, a column for every detected error.
4. Generate the same execution behavior for the same source and the same target profile.
5. Never conceal a type or address error through implicit conversion.
6. Report an “unsupported feature” error for a rule it does not implement; it MUST NOT silently ignore that rule.

---

## 3. Program Structure

Every Stage 1 file MUST contain exactly one top-level `machine` block.

```qplc
machine <machine_name>
    <declarations>
    starts in <initial_state>

    <state_definitions>
```

The components of a `machine` block MUST appear in this order:

1. The `machine` keyword and machine name.
2. Zero or more `input`, `output`, or `data` declarations.
3. Exactly one `starts in` declaration.
4. One or more `state` definitions.

New declarations are not allowed after `starts in`. All declarations MUST appear before it. State definitions follow `starts in`.

### 3.1 Namespace and Identifier Rules

In Stage 1, machine, variable, and state names MUST be unique after case normalization. For example, `Motor` and `motor` are considered the same name. Keywords are case-insensitive; to avoid inconsistencies in generated output, lowercase keywords and variable names are recommended.

Names MUST start with an English letter or `_`, followed only by English letters, digits, or `_`. A name MUST NOT start with a digit. A reserved keyword MUST NOT be used as a machine, state, or variable name.

### 3.2 State Names

A state name is an ordinary identifier. `starts in idle` must refer to a `state idle` definition. A reference to an undefined state is a semantic error. Nested states are not supported in Stage 1.

---

## 4. Lexical Rules

### 4.1 Line Endings and Indentation

- Files MUST use UTF-8. Identifiers in Stage 1 are limited to the ASCII character set defined in Section 3.1.
- Both `LF` and `CRLF` line endings MUST be accepted.
- Indentation is defined with spaces. A tab used for indentation is an error.
- Indentation levels MUST be structurally consistent. Whenever a block returns to a previously used indentation level, the same leading-space count MUST identify that level.
- Indentation on blank lines and comment-only lines does not affect indentation-level calculation.
- A line MUST NOT change indentation levels without a corresponding structural reason.

### 4.2 Comments

Outside a literal, `#` starts a comment that runs to the end of the line. Block comments are not supported in Stage 1.

```qplc
motor := on  # Turn the motor on during every scan of this state
```

### 4.3 Reserved Keywords

The following are reserved and MUST NOT be used as identifiers:

`machine`, `input`, `output`, `data`, `temp`, `retain`, `starts`, `in`, `state`, `on`, `entry`, `when`, `then`, `change`, `to`, `start`, `timer`, `done`, `rises`, `falls`, `and`, `or`, `not`, `mod`, `true`, `false`, `bool`, `byte`, `int`, `dint`, `word`, `dword`, `real`.

In Stage 1, `retain` remains a reserved keyword, but persistence across restart is not yet supported. Using it in an unsupported position is a syntax or semantic error.

### 4.4 Tokens and Literals

- The lexer MUST use a longest-match rule so that addresses such as `%IB0` are not incorrectly split into `%I` and identifier `B0`, and decimal literals are not split into an integer token and a period.
- `INT_LIT`: one or more decimal digits without separators. A minus sign is not part of the literal; it is parsed as a unary operator.
- `REAL_LIT`: a decimal number containing a decimal point, such as `1.25` or `0.0`. Scientific notation such as `1e-3` is not allowed in Stage 1.
- `BOOL_LIT`: `true` or `false`. The words `on` and `off` are accepted as synonyms for `true` and `false` only in a Boolean-value context.
- `DURATION`: a non-negative integer followed by one of `ms`, `s`, or `m`, such as `250ms`, `5s`, or `2m`.
- String, character, hexadecimal, and binary literals are not supported in Stage 1.

### 4.5 Whitespace

Whitespace around operators is not mandatory, but is recommended. Writing complex expressions without spaces is discouraged for readability.

---

## 5. Declarations

### 5.1 Inputs

Syntax:

```qplc
input <name> [ : <type> ] at <address>
```

Examples:

```qplc
input start_button at %I0.0
input analog_value : int at %IW64
```

Rules:

- An input is read-only in QPLC logic.
- Its type may be explicit or inferred from its address.
- If omitted, the default type is determined by the address table in Section 7.
- An explicit type MUST be compatible with the address width.
- Writing to an input anywhere in the program is a semantic error.
- At the start of each scan, inputs are read from the target's input process image. Direct peripheral access using suffixes such as `:P` is not supported in Stage 1.

### 5.2 Outputs

Syntax:

```qplc
output <name> [ : <type> ] at <address> = <literal>
```

Examples:

```qplc
output motor at %Q0.0 = off
output speed_command : int at %QW64 = 0
```

The value following `=` MUST be a literal and defines the output's default value. An expression is not allowed in an output declaration.

Rules:

- An output address MUST belong to the Q area.
- An explicit type MUST be compatible with the address width.
- The default value MUST be type-compatible with the output.
- At the beginning of every scan, an output is set to its default value.
- An output can be changed only through permitted assignments inside states; assigning an output outside a state body is not allowed.
- An output address MUST NOT overlap the address range of any other input or output.

### 5.3 Persistent Data

Syntax:

```qplc
data <name> : <type> = <literal>
```

Examples:

```qplc
data cycle_count : dint = 0
data setpoint : real = 25.0
```

In Stage 1, every persistent `data` variable MUST have an explicit initial value. This prevents indeterminate behavior when the machine starts.

Rules:

- When a machine instance begins execution, the data variable is initialized from its initial literal.
- After initialization, its value is retained between scans.
- Retention between scans does not guarantee retention after power loss, a new download, a cold restart, or recreation of the instance DB.
- Stage 1 does not provide a portable keyword that guarantees retention across restarts.
- A `data` variable may be assigned only in `on entry` or in a state body.
- Since Stage 1 has no general-purpose blocks or functions, data lifetime is limited to that machine instance.

### 5.4 Temporary Data

Syntax:

```qplc
data <name> : <type>, temp
```

Example:

```qplc
data scratch : int, temp
```

Rules:

- At the beginning of each scan, a `temp` variable is set to its type's default value: `false` for `bool` and zero for numeric types.
- Its value is not retained from the previous scan.
- A `temp` variable MUST NOT have an explicit initial value.
- A `temp` variable MUST NOT be used for counters, edge memory, latches, or any information required to persist between scans.
- The implementation MUST explicitly generate initialization at the start of each scan; it MUST NOT rely on arbitrary values or leftover memory in the CPU's TEMP area.

### 5.5 Declaration Order and Uniqueness

All declarations MUST appear before `starts in`. Names MUST be unique. Declaring two variables with the same name, even with different types, is an error. Duplicate or overlapping physical addresses are also errors as defined in Section 7.

---

## 6. Data Types, Literals, and Conversions

### 6.1 Stage 1 Types

| QPLC type | Width | Meaning | `temp` default | Main operators |
|---|---:|---|---|---|
| `bool` | 1 bit | Boolean | `false` | `and`, `or`, `not`, equality comparison |
| `byte` | 8 bits | Unsigned byte value | `0` | Numeric/equality comparison; no direct arithmetic |
| `int` | 16 bits | Signed integer | `0` | Same-type arithmetic and comparison |
| `word` | 16 bits | Unsigned 16-bit value | `0` | Numeric/equality comparison; no direct arithmetic |
| `dint` | 32 bits | Signed integer | `0` | Same-type arithmetic and comparison |
| `dword` | 32 bits | Unsigned 32-bit value | `0` | Numeric/equality comparison; no direct arithmetic |
| `real` | 32 bits | Floating-point number | `0.0` | Floating-point arithmetic and comparison |

`byte`, `word`, and `dword` are interpreted as unsigned values to align with bit-oriented SCL types. In Stage 1, equality and numeric comparison are allowed on them, but direct `+`, `-`, `*`, `/`, or `mod` arithmetic is not. For arithmetic, an explicit valid conversion to `int` or `dint` must be performed. No implicit conversion to a signed type is performed.

Target numeric ranges:

- `byte`: 0 through 255
- `int`: −32768 through 32767
- `word`: 0 through 65535
- `dint`: −2147483648 through 2147483647
- `dword`: 0 through 4294967295
- `real`: according to the target Siemens/TIA 32-bit `REAL` type

### 6.2 Integer Literal Typing

In a context where the destination or another operand has a known numeric type, an integer literal may be interpreted as that type, provided its value lies within the type's range. In numeric expressions, an integer literal may be contextually typed to match an `int`, `dint`, or `real` operand. A literal's contextual type does not permit implicit conversion between variables. For example:

```qplc
data cycle_count : dint = 0

state running
    cycle_count := cycle_count + 1
```

Here, the literal `1` is compatible with the `dint` type of `cycle_count`. This rule applies only to untyped numeric literals; an `int` variable is not automatically converted to `dint`.

If an integer literal has no type context, it receives type `int` when within the `int` range; otherwise it receives type `dint` when within the `dint` range. A number outside all supported ranges is an error.

### 6.3 Initial Values

An initial literal MUST be compatible with the declared type. For numeric literals, an optional `+` or `-` sign is part of the initial value; a sign is not allowed on a Boolean literal. Boolean values `on`/`off` are accepted only for `bool`. A decimal-point literal is for `real`. An integer literal may be contextually interpreted according to the destination type; no conversion is applied to the value of a variable unless an explicit conversion is written.

### 6.4 Explicit Conversion

General form:

```qplc
<type>(<expression>)
```

Example:

```qplc
speed_int := int(speed_real)
```

Explicit conversion is allowed only for conversions supported by the target SCL profile. The compiler MUST validate the source/destination type combination and generate the corresponding TIA/SCL conversion; an unsupported conversion is a compile-time error.

To keep conversion behavior predictable:

- Constant conversions are evaluated at compile time. If the value is outside the destination range, an error is issued.
- For run-time values, exact conversion behavior MUST match the equivalent conversion instruction in the specified Siemens TIA/SCL version, and the target version MUST be recorded in the compilation report.
- The compiler MUST NOT remove an explicit conversion written by the user or silently replace it with a different conversion.
- For portability across PLC vendors, a separate vendor-neutral conversion matrix would be needed. This specification currently targets the Siemens/TIA profile.

### 6.5 Overflow and Division by Zero

The compiler MUST detect constant overflow at compile time. For run-time expressions, behavior that depends on hardware or the TIA version MUST be recorded and tested in the target profile; the compiler MUST NOT claim that all CPUs behave identically without testing.

Division by zero in a constant expression is a compile-time error. For run-time values, the programmer MUST ensure before division that the divisor is non-zero. Stage 1 does not define `try`, exceptions, run-time error handlers, or an automatic transition to an error state. Therefore, division by zero at run time is not a language behavior that can be relied upon and MUST be prevented.

---

## 7. Physical Addressing

### 7.1 Supported Addresses

Stage 1 accepts only the input area `I` and output area `Q`:

| Address form | Width | Inferred type | Meaning |
|---|---:|---|---|
| `%I<byte>.<bit>` | 1 bit | `bool` | Input bit |
| `%Q<byte>.<bit>` | 1 bit | `bool` | Output bit |
| `%IB<byte>` | 8 bits | `byte` | Input byte |
| `%QB<byte>` | 8 bits | `byte` | Output byte |
| `%IW<byte>` | 16 bits | `word` | Input word |
| `%QW<byte>` | 16 bits | `word` | Output word |
| `%ID<byte>` | 32 bits | `dword` | Input double word |
| `%QD<byte>` | 32 bits | `dword` | Output double word |

The number following `IB`, `IW`, `ID`, `QB`, `QW`, or `QD` is the starting byte address, not a word index or element index. The actual valid range depends on the CPU model and hardware configuration. The compiler MAY check general limits, but final hardware compatibility must be checked against the target configuration.

### 7.2 Type and Width Compatibility

An explicit type is accepted only when its width matches the address:

- Bit address: `bool` only.
- Byte address: `byte` only.
- Word address: `word` or `int`.
- Double-word address: `dword` or `dint`.

Stage 1 does not support declaring `real` directly on an absolute address. Floating-point signals must first be obtained from a suitable raw area using an explicit, valid conversion in the program/target profile. The compiler MUST NOT silently place `real` on `%IW` or `%ID`.

### 7.3 Address Direction

An `input` declaration may use only `%I...`; an `output` declaration may use only `%Q...`. Any contrary combination is a semantic error.

### 7.4 Conflicts and Overlaps

Two declarations MUST NOT share a physical address or use overlapping memory ranges. For example, `%I0.0` overlaps `%IB0`, and `%IW0` overlaps `%IB1`. Input/output overlap is already prevented by the different I and Q areas; overlap within the same area is an error.

### 7.5 Process-Image Inputs

Inputs are read from the process image by default. Stage 1 does not define immediate access such as `%I0.0:P`, the `M` memory area, absolute DB access, special rules for distributed I/O modules, or symbolic project addressing.

---

## 8. Expressions and Operator Precedence

### 8.1 Supported Operators

| Precedence | Operators | Rule |
|---:|---|---|
| 1, highest | Parentheses and type conversion | `(expr)`, `int(expr)`, etc. |
| 2 | Unary numeric | `-expr`, `+expr` |
| 3 | Multiplicative | `*`, `/`, `mod` |
| 4 | Additive | `+`, `-` |
| 5 | Comparison | `=`, `<>`, `<`, `<=`, `>`, `>=` |
| 6 | NOT | `not` |
| 7 | AND | `and` |
| 8, lowest | OR | `or` |

Binary operators other than comparisons group from left to right. A comparison expression may contain only one comparison operator; a chained comparison such as `a < b < c` is invalid and must be rewritten as two comparisons joined with `and`.

### 8.2 Operator Type Rules

- `and`, `or`, `not`: Boolean (`bool`) operands only.
- `=`, `<>`: two operands of the same type; allowed for `bool` and all Stage 1 data types.
- `<`, `<=`, `>`, `>=`: same-type numeric operands only; Boolean comparison is not allowed.
- `+`, `-`, `*`, `/`: two same-type operands of `int`, `dint`, or `real`. The result has the same type as the operands.
- `mod`: two same-type operands of `int` or `dint` only; `real`, `byte`, `word`, and `dword` are not allowed.
- Unary `+` and `-`: only for `int`, `dint`, and `real`. Unary minus on `byte`, `word`, or `dword` is an error.
- Explicit conversion: only as permitted by the target profile's conversion matrix.

Except for context-typed numeric literals defined in Section 6.2, no implicit conversion between different types is allowed. For example, `int_value + dint_value` is an error; one operand must be explicitly converted.

### 8.3 Logical Evaluation

In QPLC, `and` and `or` use left-to-right short-circuit evaluation:

- In `A and B`, if `A` is `false`, `B` is not evaluated.
- In `A or B`, if `A` is `true`, `B` is not evaluated.

The compiler MUST preserve this behavior even if a direct equivalent SCL expression does not guarantee it; explicit conditional logic may be generated as needed. Nevertheless, programmers SHOULD avoid placing potentially invalid calculations on the right-hand side of a logical operator and SHOULD validate calculation safety independently of short-circuiting.

### 8.4 Division and `mod`

Integer division for `int` and `dint` returns a result of the same integer type and discards the fractional part according to the target's equivalent instruction. `byte`, `word`, and `dword` values must be converted to a suitable signed type before arithmetic. Division by zero is prohibited. `mod` is defined only for same-type integer operands; the behavior for negative operands MUST be recorded in the Siemens/TIA target profile with conformance tests. Portable code MUST NOT rely on differing results for negative values.

### 8.5 Parentheses and Edge Detection

`rises` and `falls` are not general expression operators. They are allowed only in a transition condition. To detect an edge on a comparison or logical expression, the complete expression MUST be parenthesized:

```qplc
when start_button rises then change state to running
when (weight > limit) rises then change state to fault
when (sensor_a and sensor_b) rises then change state to trigger
```

The expression `when weight > limit rises ...` is invalid because it does not make clear which part of the comparison owns the edge. Parentheses remove the ambiguity.

---

## 9. State Machines, Assignments, and `on entry`

### 9.1 State Definition

```qplc
state <state_name>
    [on entry
        <assignments>]
    <state_body>
```

Every machine MUST contain at least one state. State names MUST be unique within the machine. The initial state must be one of those definitions.

### 9.2 State-Body Structure

To preserve predictable ordering, a state body consists of these sections:

1. An optional `on entry` block, at most once and only at the beginning of the state.
2. Zero or more continuous assignments and at most one `start timer` declaration.
3. Zero or more `when ... then ...` transitions at the end of the state.

After the transition list begins, no further assignment or timer declaration is allowed. A state MUST contain at least one assignment, timer, or transition. If a state intentionally does nothing, it must still contain an appropriate explicit transition or behavior; an empty state is not allowed.

### 9.3 Assignment

Assignment syntax:

```qplc
<assignable_name> := <expression>
```

Examples:

```qplc
motor := on
cycle_count := cycle_count + 1
```

An assignment target may be an output or a `data` variable. An input, state name, timer, or undefined name is not an assignment target.

The expression type MUST match the target type. The only exception is a context-typed numeric literal as defined in Section 6.2. Assigning a variable of a different type without explicit conversion is an error.

### 9.4 Multiple Assignments to One Variable

Within a scan, assignments execute in source order within their respective sections. If the same variable is assigned more than once during the same scan, the last executed assignment determines its final value. This does not mean ambiguous coding is recommended; the compiler SHOULD warn about conflicting assignments to the same output in one state.

`on entry` assignments execute before continuous assignments. If both assign the same variable during the same scan, the continuous assignment determines the final value.

### 9.5 `on entry` Behavior

The `on entry` block executes only once during the scan in which the state is entered. For the initial state, it executes during the machine's first scan. A self-transition also counts as a new entry, so `on entry` executes again in the next scan.

Only assignments are permitted inside `on entry`; state transitions, timer declarations, nested blocks, and loops are not allowed.

Important: an output assignment in `on entry` takes effect only during that scan. During the next scan, the output returns to its default unless the state's continuous body assigns it again. Therefore, to keep a motor on, write:

```qplc
state running
    motor := on
```

not merely:

```qplc
state running
    on entry
        motor := on
```

The second example creates a one-scan pulse, not a continuously maintained output.

### 9.6 Output Values

At the beginning of every scan, all outputs are set to their declared defaults. The active state's `on entry` block, if present and if this is an entry scan, executes next; continuous assignments in that state execute after it.

Consequently:

- If an output was on in the previous state and the new state does not write it, it takes its default during the new state's scan.
- If an output is assigned `on` during every scan of the current state, it remains on throughout the time spent in that state.
- If an output is assigned only in `on entry`, it is non-default only during the entry scan.
- There is no implicit conditional assignment. For conditional values, use state transitions or a Boolean expression on the right-hand side of an assignment.

### 9.7 State Entry and Exit

Stage 1 has no `on exit` block. A transition is scheduled at the end of the current state's logic, and the target state becomes active at the beginning of the next scan. Outputs for the current scan are calculated using the old state; the target state's effects become visible in the next scan.

---

## 10. State Transitions and Edge Detection

### 10.1 Transition Syntax

```qplc
when <condition> then change state to <target_state>
```

The condition MUST have type `bool`. The target must refer to a defined state.

### 10.2 Transition Priority

Transitions are evaluated strictly in source order, from top to bottom. The first true condition is selected; lower conditions are not checked for transition selection during that scan. At most one transition is scheduled in any scan.

If no condition is true, the machine remains in the same state. Having no true transition is not itself an error; it may be intentional for a stable state.

### 10.3 State Changes Take Effect One Scan Later

When a transition condition becomes true during scan N, the transition is scheduled and the target state becomes active at the start of scan N+1. Assignments in the target state do not execute during scan N.

This rule applies to every transition, including a transition to the same state (a self-transition). A self-transition causes a new entry in the next scan; that state's `on entry` block and timer must restart according to their respective rules.

### 10.4 Level Conditions

```qplc
when not safety_ok then change state to fault
```

A level condition is evaluated once per scan while its state is active. If it is true, the transition is selected according to priority. If the signal remains true for multiple scans, it does not cause repeated transitions because the target state becomes active in the next scan.

### 10.5 Edge Conditions

```qplc
when start_button rises then change state to running
when reset_button falls then change state to idle
when (weight > target) rises then change state to fault
```

`rises` and `falls` are allowed only on Boolean expressions. The compiler generates previous-sample memory for each distinct edge expression.

Edge memory:

- MUST NOT use `timer done` in an edge expression, because edge history is global while a timer belongs only to its state.
- Is updated at the end of every scan.
- Is updated independently of which state is active.
- Is seeded with the current expression value when the machine starts; therefore, the initial sample does not generate an artificial edge.
- Is updated even during scans in which the edge condition is not in the active state's transition list. As a result, an edge that occurs while the machine is in another state is not queued as a pending event for later delivery.

### 10.6 Identity of Edge Expressions

The compiler MUST canonicalize identical edge expressions according to a documented rule. In Stage 1, two edge expressions share memory only if their parsed and normalized syntax trees are identical; mathematical or logical equivalence alone is not enough. For example, `start_button rises` and `(start_button) rises` may be treated as identical if the latter form is allowed, but the compiler is not required to algebraically simplify complex expressions.

### 10.7 Global Faults and Safety Priority

Stage 1 has no global `always`, `guard`, `priority transition`, or `safety transition` construct. A fault condition must be represented in every state where it needs to be handled. The program author must order transitions so that relevant process-fault conditions take priority over less important transitions.

**Warning:** QPLC transition ordering is a software rule and is not a substitute for hardware safety functions. Emergency stops and personnel protection MUST be assessed and implemented independently of ordinary QPLC logic.

---

## 11. Timers

### 11.1 Timer Declaration

In Stage 1, each state may have at most one anonymous timer:

```qplc
start timer 5s
```

The timer does not appear in the variable namespace and can be queried only with:

```qplc
timer done
```

Using `timer done` in a state with no `start timer` is a semantic error. Named timers and references to another state's timer are not permitted in Stage 1.

### 11.2 Duration Units and Range

Allowed suffixes:

- `ms`: milliseconds
- `s`: seconds
- `m`: minutes

The duration MUST be non-negative. After conversion to milliseconds, it MUST fit within the range representable by `TIME` in the target profile. A literal outside the range is a compile-time error. `0ms` is allowed, and `timer done` is true on that timer's first valid evaluation.

### 11.3 Timer Semantics

The compiler generates an independent timer instance for each state that declares one. The timer starts from the beginning upon entry into that state. On the first scan in which elapsed time reaches the specified duration, `timer done` becomes true and remains true until the state is left.

When the state is exited, its timer is reset. A self-transition is a new entry and restarts the timer. Elapsed time between scans is determined by the PLC's actual scheduling and the target timer's resolution. The language does not guarantee that the condition is evaluated at the exact microsecond the duration expires; instead, the condition is observed during the first executable scan after the timer has reached the specified duration.

The compiler MAY use IEC `TON` or an equivalent implementation, but it MUST preserve the behavior above in conformance tests, especially for initial entry, exit, self-transition, and zero duration. The implementation MUST NOT add an undocumented extra scan to the duration because of timer-block call order.

### 11.4 Evaluating `timer done`

`timer done` is a Boolean expression and is allowed only within the body of the same state that owns the timer. It may be used as a level transition condition:

```qplc
state filling
    valve := on
    start timer 10s
    when level_switch rises then change state to complete
    when timer done then change state to fault
```

Source order still determines priority. In this example, if `level_switch rises` and `timer done` are both true during one scan, the transition to `complete` wins.

### 11.5 Limitations

- Each state may contain at most one `start timer`.
- The duration is constant; a run-time expression is not allowed as a duration.
- Named timers, timer pause/resume, accumulated timing, `TONR`, `TOF`, and `TP` are not defined in Stage 1.

---

## 12. Deterministic Scan Execution Model

This section is the normative reference for language behavior. If an interpretation inferred from an example conflicts with this section, this section takes precedence.

### 12.1 Machine Instance Startup

When a machine instance starts:

1. All persistent `data` variables are initialized from their initial literals.
2. Internal state-machine variables and the entry flag are initialized.
3. Outputs have their declared default values in the first scan.
4. The state named in `starts in` becomes the initial state, and its `on entry` block runs during the first scan.
5. Edge-detection memory is initialized using the current value of each edge expression during the first scan; the first scan does not create artificial edges.

Preserving data after restart, download, or power loss is outside the language guarantee and depends on CPU/instance DB configuration.

### 12.2 One-Scan Algorithm

Each scan MUST be observably equivalent to the following steps:

1. **Sample inputs:** Read all QPLC inputs from the target's input process image.
2. **Determine the current state:** The state active at the beginning of the scan remains the current state throughout this scan. A transition discovered during this scan does not change the current state immediately.
3. **Initialize temporaries:** Set every `temp` variable to its type's default value.
4. **Initialize outputs:** Set every output to its declared default.
5. **Execute `on entry`:** If this is an entry scan for the current state, execute that state's `on entry` block exactly once.
6. **Execute the continuous body:** Execute the current state's assignments in source order. Timer processing follows a compliant implementation of Section 11.
7. **Evaluate transitions:** Evaluate conditions from top to bottom. Select the first true transition and stop checking transitions. If none is true, the next state is the current state.
8. **Record a transition:** If a transition was selected, record its target for the beginning of the next scan. If the target is the current state, also record a new-entry event for the next scan.
9. **Update edge history:** After evaluating the current state's conditions, update the previous-value memory for every edge expression with that expression's final sampled value. This update is independent of the current state.
10. **Apply outputs:** Deliver the output values calculated during this scan to the PLC output mechanism. The exact physical output update timing depends on the CPU cycle and configuration.
11. **Prepare the next scan:** If a transition was recorded, prepare the target state and entry flag for the start of the next scan; otherwise, remain in the same state.

### 12.3 Read/Write Rules Within a Scan

- Inputs are one logical snapshot at the start of the scan; they are not read from hardware again during that scan.
- `data` assignments execute in source order. A later assignment can observe a value written by an earlier assignment in the same scan.
- `on entry` assignments execute before the continuous body.
- Transition conditions are evaluated after the state's assignments and may observe `data` values updated during that scan.
- A state transition does not run the target state's logic before the next scan.
- Every output is reset at the start of the scan. An output does not stay non-default merely because it was assigned during the previous scan.

### 12.4 Self-Transitions

In a transition such as:

```qplc
when retry_button rises then change state to retry
```

if the condition is evaluated and becomes true while the current state is `retry`, the current state remains `retry` for the rest of this scan. A new entry into `retry` occurs in the next scan. Consequently, `on entry` executes again and the state's timer restarts. Persistent `data` variables are not automatically cleared.

---

## 13. Semantic Analysis and Errors

The parser checks syntax only. Before generating SCL, the compiler MUST perform semantic analysis.

### 13.1 Mandatory Errors

At least the following conditions MUST produce errors:

1. Missing or duplicate `machine` block.
2. Missing or duplicate `starts in` declaration.
3. Undefined initial state.
4. Transition to an undefined state.
5. Duplicate name after applying case-normalization rules.
6. Undefined variable.
7. Attempt to write to an input.
8. Assignment whose value is incompatible with the target type.
9. Operator used with invalid operand types.
10. Unsupported type conversion.
11. Invalid address direction (`input` on Q or `output` on I).
12. Mismatch between type and address width.
13. Invalid address format or general address range.
14. Duplicate or overlapping address.
15. Persistent `data` declaration without an initial value.
16. Initial value supplied for a `temp` declaration.
17. Multiple `on entry` blocks in one state or an `on entry` block placed after the state body has begun.
18. More than one timer in a state.
19. Use of `timer done` in a state with no timer.
20. Transition condition that is not `bool`.
21. Use of `rises`/`falls` outside a transition condition or on an expression that is not `bool`.
22. Chained comparison with multiple comparison operators.
23. Numeric literal outside the permitted range.
24. Timer duration outside the target's supported range.
25. Constant division by zero.
26. Statement or keyword not defined in Stage 1.
27. Empty state body or invalid indentation structure.

### 13.2 Recommended Warnings

The compiler SHOULD warn about:

- A state unreachable from the initial state.
- A state with no transitions that effectively holds the machine forever.
- An output that is never assigned anything other than its default.
- Multiple potentially conflicting assignments to the same output in one state.
- Unused states or variables.
- A transition condition that cannot be selected because a higher-priority condition always captures the same cases.
- A literal or initial value unusually close to a type boundary.
- A timer whose `timer done` condition has no reachable path.

### 13.3 Error Reporting

An error message SHOULD include an error identifier, source location, understandable explanation, and, where possible, a suggested correction. Example:

```text
QPLC-E-TYPE-004 at line 18, column 9:
Cannot assign value of type REAL to output 'motor' of type BOOL.
Use a BOOL expression or an explicit, valid conversion.
```

The identifiers above are illustrative. An implementation may use different internal identifiers, but it MUST provide recognizable error categories and source locations.

---

## 14. SCL Generation Rules

### 14.1 Target Scope

Stage 1 targets SCL compatible with Siemens TIA Portal. Each compiler release MUST record its target profile, including at least the CPU family, TIA Portal version, and instance DB/timer-generation policy.

### 14.2 Recommended Generated Structure

An implementation may translate a machine into an FB and store persistent data and internal memory in static/instance DB storage. `temp` variables may map to TEMP, but explicit initialization at the beginning of each scan MUST be generated to guarantee their default values.

Generated internal names MUST use a reserved namespace, for example `__qplc_state`, `__qplc_entry_pending`, or `__qplc_prev_edge_001`. The compiler MUST prevent collisions between those generated names and user identifiers using a private namespace, collision-proof name generation, or explicit collision checking.

### 14.3 Outputs

At every scan, the compiler MUST write all output defaults before executing the state logic. The `on entry` block and continuous assignments then execute according to Section 12. This order MUST NOT depend on values left over from the previous scan.

### 14.4 State Transitions

To guarantee first-true-condition priority, the compiler MAY use `IF / ELSIF / ELSE`, an internal `transition_selected` flag, or an equivalent structure. Once a transition is selected, lower-priority conditions MUST NOT be able to replace its target.

The target state MUST be recorded at the end of the scan and become active at the beginning of the next scan. A self-transition MUST create a new-entry event even if the state enum/value does not change.

### 14.5 Edge Detection

For each edge expression, the compiler generates a current value and a previous value. `rises` is defined as `current AND NOT previous`; `falls` is defined as `NOT current AND previous`. Previous-value memory is updated at the end of the scan. During the first scan, the previous value is seeded from the current value to prevent an initial artificial edge.

If an expression references values that are valid only in some states, the compiler MUST ensure the expression can be evaluated with valid values during every scan before generating edge logic.

### 14.6 Timers

Each timer instance MUST belong to its designated state. Generated logic MUST guarantee reset on exit and on self-transition, and the timer's scan-start semantics MUST match Section 11. Merely generating a `TON` call without checking re-entry behavior does not constitute full conformance.

### 14.7 Type and Address Mapping

Type and address mapping MUST follow Sections 6 and 7. A common mapping is:

| QPLC | Typical SCL type |
|---|---|
| `bool` | `BOOL` |
| `byte` | `BYTE` |
| `int` | `INT` |
| `word` | `WORD` |
| `dint` | `DINT` |
| `dword` | `DWORD` |
| `real` | `REAL` |

Every generated absolute address MUST be valid for the target CPU/project. If a newer CPU restricts an absolute-address form, the compiler MUST report a target error and MUST NOT silently change the address.

---

## 15. Complete Example

The following is a revised conveyor example. It illustrates basic process logic; it is not, by itself, a safety-validated conveyor design.

```qplc
machine conveyor
    input start_button at %I0.0
    input stop_button at %I0.1
    input safety_ok at %I0.2
    input reset_button at %I0.3
    input weight : int at %IW64

    output motor at %Q0.0 = off
    output alarm at %Q0.1 = off
    output green at %Q0.2 = off

    data cycle_count : dint = 0
    data scratch : int, temp

    starts in idle

    state idle
        motor := off
        alarm := off
        green := on
        when not safety_ok then change state to fault
        when start_button rises and safety_ok then change state to running

    state running
        on entry
            cycle_count := cycle_count + 1
        motor := on
        green := off
        start timer 5s
        when not safety_ok then change state to fault
        when stop_button rises then change state to idle
        when (weight > 100) rises then change state to fault
        when timer done then change state to fault

    state fault
        motor := off
        alarm := on
        green := off
        when reset_button rises and safety_ok then change state to idle
```

### 15.1 Notes on the Example

- In `idle`, `not safety_ok` is checked before the start condition. Therefore, if both conditions are true during the same scan, the transition to `fault` has priority.
- `motor := on` is in the continuous body of `running`, so the motor remains on during every scan of that state.
- `cycle_count` is persistent and increments exactly once each time the machine enters `running`.
- `timer done` is used only in a state that declares `start timer`.
- The edge expression `(weight > 100) rises` has its transition checked only when `running` is active, but its history is updated at the end of every scan.
- `safety_ok` conditions must be evaluated in every relevant state. In a real system, safety stopping must not rely solely on a software state transition.

---

## 16. Valid and Invalid Examples

### 16.1 Valid: Boolean Assignment

```qplc
output motor at %Q0.0 = off

state running
    motor := on
```

### 16.2 Invalid: Writing to an Input

```qplc
input start_button at %I0.0

state idle
    start_button := off
```

Reason: Inputs are read-only.

### 16.3 Invalid: Incompatible Type

```qplc
output motor at %Q0.0 = off

state running
    motor := 1
```

Reason: `motor` has type `bool`; the value `1` is not implicitly converted to `true`.

### 16.4 Valid: Explicit Conversion

```qplc
input raw_value : int at %IW64
data scaled : real = 0.0

state sampling
    scaled := real(raw_value)
```

Final acceptance depends on whether the target profile supports conversion from `INT` to `REAL`. This conversion is supported by conventional Siemens/TIA profiles.

### 16.5 Invalid: Input Using a Q-Area Address

```qplc
input start_button at %Q0.0
```

Reason: `input` can map only to `%I...`.

### 16.6 Invalid: Two Timers in One State

```qplc
state filling
    start timer 5s
    start timer 10s
```

Reason: At most one anonymous timer is allowed per state.

### 16.7 Invalid: Using an Undeclared Timer

```qplc
state idle
    when timer done then change state to fault
```

Reason: This state has no `start timer` declaration.

### 16.8 Invalid: Edge Condition with a Non-Boolean Type

```qplc
when weight rises then change state to fault
```

Reason: `weight` is numeric. A Boolean expression must be used, such as `(weight > 100) rises`.

### 16.9 Invalid: Chained Comparison

```qplc
when low < value < high then change state to running
```

Correct form:

```qplc
when value > low and value < high then change state to running
```

This requires all three values to have compatible identical types and the final expression to be Boolean.

### 16.10 Invalid: Persistent Data without an Initial Value

```qplc
data cycle_count : dint
```

Reason: In Stage 1, persistent data must have an explicit initial value.

---

## 17. Minimum Conformance Tests

Before releasing a Stage 1 compiler, at least the following tests MUST be implemented. Each test must check both that generated SCL is syntactically valid and that its behavior conforms to the execution model in Section 12.

### 17.1 Parser and Lexer

1. A file with LF line endings is accepted.
2. A file with CRLF line endings is accepted.
3. A tab at the start of a line is rejected.
4. A trailing comment does not change statement validity.
5. An identifier starting with a digit is rejected.
6. A reserved keyword used as a name is rejected.
7. Inconsistent indentation is reported as an error.

### 17.2 Semantic Analysis

8. An undefined initial state is rejected.
9. An undefined transition target is rejected.
10. A duplicate name differing only by capitalization is rejected.
11. Assignment to an input is rejected.
12. An assignment with an incompatible type is rejected.
13. An input mapped to `%Q` and an output mapped to `%I` are rejected.
14. Two overlapping addresses are rejected, such as `%IB0` and `%I0.0`.
15. Persistent `data` without an initial value is rejected.
16. `temp` with an initial value is rejected.
17. Multiple timers in one state are rejected.
18. `timer done` in a state without a timer is rejected.
19. A non-Boolean edge condition is rejected.
20. A chained comparison is rejected.

### 17.3 Scan Model

21. The initial state's `on entry` executes exactly once.
22. `on entry` executes exactly once in the entry scan after each actual transition.
23. A self-transition causes `on entry` to execute again during the next scan.
24. A non-default output from the previous state returns to its default in a state that does not assign it.
25. An output assignment in `on entry` takes effect only in the entry scan.
26. A continuous output assignment remains effective during every scan of its state.
27. Persistent data is retained between scans.
28. A `temp` variable has its default value at the start of every scan.
29. An edge expression that is already true when the machine starts does not produce an artificial `rises` edge on the first scan.
30. The history of an edge expression is updated even when its state is inactive.
31. If two transition conditions are true simultaneously, only the first is selected.
32. The target state's logic runs only in the scan after the transition is selected.

### 17.4 Timers

33. A timer starts over upon entry into its state.
34. `timer done` is false before its duration expires, except for zero duration.
35. `timer done` becomes observable during the first scan after the duration has elapsed.
36. The timer resets after state exit.
37. A self-transition restarts the timer.
38. If timer-related and non-timer transition conditions are true simultaneously, source-order priority is respected.
39. A duration outside the target `TIME` range is rejected before SCL generation.

---

## 18. Features Outside Stage 1

The following features are intentionally undefined in this version:

- Imports and multiple files.
- More than one `machine` in a file.
- Multiple concurrent machines or shared signals between machines.
- Nested machines, composite states, or state history.
- Arrays, structs, custom types, user-defined enums, and strings.
- Loops, functions, procedures, user-function calls, and recursion.
- Machine parameters.
- `on exit` blocks.
- Named timers, accumulated timers, and the `TP`, `TOF`, and `TONR` timer families.
- Global guard conditions or transition priorities separate from source order.
- `retain` with a guarantee of retention across restart.
- `M` memory, absolute DB addressing, symbolic addressing, and immediate access using `:P`.
- Direct declaration of `real` at an absolute I/Q address.
- String, character, hexadecimal, binary, and scientific-notation real literals.
- Language-level exception handling or run-time error handlers.
- Any guarantee of functional safety or safety certification.

If any of these features is used, the compiler MUST reject it rather than silently ignore it.

---

## 19. Implementation Checklist

### 19.1 Recommended Compilation Pipeline

1. **Normalize line endings and scan indentation:** Generate `INDENT`, `DEDENT`, and `NL` tokens.
2. **Lexer:** Recognize identifiers, keywords, literals, addresses, operators, and comments.
3. **Parser:** Build an AST according to the EBNF.
4. **Symbol table:** Register the machine, states, inputs, outputs, and data variables.
5. **Type analysis:** Validate expressions, assignments, literals, and conversions.
6. **Address analysis:** Validate area, width, type, range, and overlap.
7. **State-machine analysis:** Validate the initial state, transition targets, timer count, reachability, and logical warnings.
8. **Build an intermediate representation (IR):** Explicitly represent scan phases, output assignments, `on entry`, transitions, and edge history.
9. **Generate SCL:** Generate internal variables, scan logic, and timer calls.
10. **Validate generated output:** Compile with the target profile and run the tests.
11. **Conformance report:** Record compiler version, target version, warnings, and errors.

### 19.2 Implementation Decisions That Must Not Remain Unresolved

- Exact name and type of the internal state variable.
- How data is initialized and how the machine behaves after restart.
- Representation of a scan and state entry.
- Exact timer behavior at scan boundaries and on self-transition.
- How short-circuit evaluation is preserved in generated SCL.
- Internal naming policy and collision prevention.
- Supported TIA Portal versions and CPU families.
- Distinction among errors, warnings, and unsupported features.
- Golden tests for generated code and simulated behavior.

### 19.3 Suggested Versioning

- A breaking syntax change or a change to scan execution semantics: increment the major specification version.
- A backward-compatible feature addition: increment the minor version.
- A documentation-only correction that does not change behavior: increment the patch version.

Until the tests in Section 17 have been executed, this document SHOULD remain a “proposed implementation baseline,” not a “finally frozen and verified” specification.

---

## 20. Target Technical References

These resources are listed to align generated SCL with Siemens type names, absolute addressing, and timer behavior. They are references for the **target profile**; the high-level rules of QPLC are defined by this document. Any behavior not explicitly specified by QPLC should be recorded along with the target platform version and limitations.

1. **Siemens — S7-1200 G2 Programmable Logic Controller, V20, “Using absolute addressing to access CPU data”.** Describes I/Q areas, bit/byte/word/double-word addresses, and absolute addressing rules.  
   https://docs.tia.siemens.cloud/r/simatic_s7_1200_g2_manual_collection_enus_20/plc-concepts-configuration-and-programming/program-structure/data-storage-memory-areas-i/o-and-addressing/using-absolute-addressing-to-access-cpu-data

2. **Siemens — S7-1200 data types.** Reference for `BOOL`, `BYTE`, `WORD`, `DWORD`, `INT`, `DINT`, `REAL`, and their widths.  
   https://docs.tia.siemens.cloud/r/simatic_s7_1200_manual_collection_frfr_20/plc-concepts/data-types/data-types

3. **Siemens — S7-1200 Programmable Controller System Manual.** Reference for IEC timers and using `TON`, `TOF`, and `TP` and timer data structures in SCL.  
   https://support.industry.siemens.com/cs/attachments/download/36932465/s71200_system_manual_en-US_en-US.pdf

---

## 21. Complete EBNF Grammar

This EBNF defines the syntax of Stage 1. Constraints that cannot be expressed by EBNF alone are mandatory semantic-analysis requirements in Section 13.

### 21.1 EBNF Conventions

- Strings in double quotes in the EBNF are literal tokens.
- `{ X }` means zero or more repetitions.
- `[ X ]` means optional.
- `|` means choice.
- `NL`, `INDENT`, and `DEDENT` are tokens generated by the indentation-aware lexer.
- `NAME`, `ADDRESS`, `INT_LIT`, `REAL_LIT`, and `DURATION` are lexical tokens.
- The transition-condition grammar separates edge operators from ordinary expressions so `rises` and `falls` cannot be mistakenly parsed as part of a comparison operand.

### 21.2 Program Grammar

```ebnf
program                 = { NL }, machine_block, { NL }, EOF ;

machine_block           = "machine", NAME, NL, INDENT,
                          machine_body,
                          DEDENT ;

machine_body            = { declaration },
                          "starts", "in", NAME, NL,
                          state_block, { state_block } ;

declaration             = input_decl | output_decl | data_decl ;

input_decl              = "input", NAME, [ ":", type ], "at", ADDRESS, NL ;

output_decl             = "output", NAME, [ ":", type ], "at", ADDRESS,
                          "=", constant_literal, NL ;

data_decl               = "data", NAME, ":", type,
                          ( "=", constant_literal | ",", "temp" ), NL ;

state_block             = "state", NAME, NL, INDENT,
                          [ on_entry ],
                          { state_pre_transition_item },
                          { transition },
                          DEDENT ;

on_entry                = "on", "entry", NL, INDENT,
                          assignment, { assignment },
                          DEDENT ;

state_pre_transition_item = assignment | timer ;

assignment              = NAME, ":=", expr, NL ;

timer                   = "start", "timer", DURATION, NL ;

transition              = "when", transition_test,
                          "then", "change", "state", "to", NAME, NL ;

transition_test         = edge_test | expr ;

edge_test               = edge_operand, edge_operator ;

edge_operand            = NAME | "(", expr, ")" ;

edge_operator           = "rises" | "falls" ;
```

### 21.3 Expression Grammar

```ebnf
expr                    = or_expr ;

or_expr                 = and_expr, { "or", and_expr } ;

and_expr                = not_expr, { "and", not_expr } ;

not_expr                = "not", not_expr | comparison_expr ;

comparison_expr         = additive_expr, [ comparison_operator, additive_expr ] ;

comparison_operator     = "=" | "<>" | "<" | "<=" | ">" | ">=" ;

additive_expr           = multiplicative_expr,
                          { ( "+" | "-" ), multiplicative_expr } ;

multiplicative_expr     = unary_expr,
                          { ( "*" | "/" | "mod" ), unary_expr } ;

unary_expr              = ( "+" | "-" ), unary_expr | primary ;

primary                 = literal
                        | timer_done
                        | NAME
                        | conversion
                        | "(", expr, ")" ;

timer_done              = "timer", "done" ;

conversion              = type, "(", expr, ")" ;

type                    = "bool" | "byte" | "int" | "word"
                        | "dint" | "dword" | "real" ;

literal                 = INT_LIT | REAL_LIT | BOOL_LIT ;

constant_literal        = [ "+" | "-" ], ( INT_LIT | REAL_LIT )
                        | BOOL_LIT ;

BOOL_LIT                = "true" | "false" | "on" | "off" ;

DURATION                = INT_LIT, ( "ms" | "s" | "m" ) ;
```

### 21.4 Basic Lexical Rules

```ebnf
NAME                    = IDENT_START, { IDENT_CONTINUE } ;
IDENT_START             = "A" … "Z" | "a" … "z" | "_" ;
IDENT_CONTINUE          = IDENT_START | "0" … "9" ;

INT_LIT                 = DIGIT, { DIGIT } ;
REAL_LIT                = DIGIT, { DIGIT }, ".", DIGIT, { DIGIT } ;
DIGIT                   = "0" … "9" ;

ADDRESS                 = BIT_ADDRESS
                        | BYTE_ADDRESS
                        | WORD_ADDRESS
                        | DWORD_ADDRESS ;

BIT_ADDRESS             = "%", ( "I" | "Q" ), INT_LIT, ".", INT_LIT ;
BYTE_ADDRESS            = "%", ( "IB" | "QB" ), INT_LIT ;
WORD_ADDRESS            = "%", ( "IW" | "QW" ), INT_LIT ;
DWORD_ADDRESS           = "%", ( "ID" | "QD" ), INT_LIT ;
```

### 21.5 Additional Semantic Constraints for the EBNF

The EBNF above does not enforce the following constraints by itself. The compiler MUST check them separately:

1. Exactly one `machine_block` is allowed in a file.
2. `machine_body` MUST contain exactly one `starts in`, before all states.
3. Machine, state, and variable names MUST be unique.
4. Every transition target MUST refer to a defined state.
5. `input` declarations may use only I-area addresses, and `output` declarations only Q-area addresses.
6. An explicit type MUST be compatible with the address width.
7. Addresses MUST NOT be duplicated or overlapping.
8. Persistent data MUST have an initial literal; `temp` MUST NOT have an initial value.
9. Each state may contain at most one `on entry` and at most one timer.
10. `on entry` MUST be the first section of a state.
11. After the first transition, no assignment or timer declaration is allowed.
12. A transition condition MUST have type `bool`.
13. `rises` and `falls` are allowed only in `transition_test` and only on a Boolean operand; an edge expression MUST NOT contain `timer done`.
14. `timer done` is allowed only in a state that declares a timer.
15. Operators MUST follow the type rules in Section 8.
16. Output and `data` assignments MUST be type-valid.
17. Every state MUST contain at least one valid item.
18. A timer duration MUST be within the range representable by the target timer.
19. `rises` and `falls` are reserved keywords and are valid only in the `edge_test` form.
20. Negative literals in declarations are accepted only through `constant_literal`; in expressions, the sign is parsed as a unary operator.
21. `byte`, `word`, and `dword` are not direct operands of arithmetic operators; they require an explicit valid conversion for arithmetic.
22. Any feature outside Stage 1 MUST be rejected rather than silently ignored.

---

**End of document — QPLC Language Reference, Stage 1, Specification 1.0 (Proposed)**
