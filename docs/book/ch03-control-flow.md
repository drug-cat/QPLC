# فصل ۳ — کنترل جریان

> در این فصل ساختارهای شرطی و حلقه — قلب هر برنامه PLC — را کامل می‌آموزید.

## مقایسه و بولی

قبل از اگر، باید بدانید شرط چیست:

```python
over_temp  = temperature > MAX_TEMP
both_hands = left_button and right_button
not_ready  = not ready_lamp

if over_temp and not_ready:
    alarm = True
```

| عملگر | معنی |
|-------|------|
| `==`, `!=` | برابری/نابرابری (مقدار، نه آدرس) |
| `<`, `>`, `<=`, `>=` | مقایسه عددی/زمانی |
| `and`, `or`, `xor` | منطقی روی BOOL |
| `not` | نقیض |

> QPLC **سخت‌گیرتر از Python** است: `if temperature:` غیرمجاز است چون شرط باید بولی صریح باشد. بنویسید: `if temperature > 0:`.

## if / elif / else

```python
if speed > 100:
    overspeed_lamp = True
elif speed > 80:
    warn_lamp = True
else:
    normal_lamp = True
```

- `elif` هر تعداد مجاز است.
- هر شاخه باید `:` و یک بلوک تو رفته داشته باشد.
- `else` اختیاری است.

## while

```python
count = 0
while counter_enabled:
    outputs[count] = inputs[count] and enable
    count = count + 1
```

با `break`/`continue`:

```python
while True:
    scan = scan + 1
    if not power_ok:
        continue        # این اسکن را نادیده بگیر
    if scan >= MAX_SCANS:
        break           # خروج
```

> `while True` بدون `break` در semantic خطای «loop must contain break» می‌گیرد — این یک تضمین ایمنی برای جلوگیری از حلقه بی‌پایان واقعی در CPU است.

## for ... in range()

```python
for i in range(4):
    outputs[i] = inputs[i] and enable
```

- `range(n)` → ۰ تا n-1
- `range(a, b)` → a تا b-1
- حلقه در کدژن **باز می‌شود (unroll)** چون شبکه‌های ladder آدرس ثابت می‌خواهند.

## عملگر سه‌تایی

```python
valve = True if temperature > 60 else False
```

در خروجی ladder به عنوان یک رانگ `LIMIT`/شرطی تبدیل می‌شود.

## match (فصل ۶ را کامل می‌آموزید)

```python
match state:
    case Idle:     lamp = True
    case Running:  lamp = False
```

## raise / استثنا (فصل ۹)

```python
when_fault = sensor_fault

# بدون خطای صریح: اگر شرط b باشد خروجی را روشن کن
```

## تمرین

موتور را با قفل‌های متقاطع بنویسید:

```python
def main():
    forward  = fwd_button and not rev_button and running
    reverse  = rev_button and not fwd_button and running
```

---

[ادامه → فصل ۴، توابع](ch04-functions)
