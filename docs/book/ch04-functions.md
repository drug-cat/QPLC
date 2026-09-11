# فصل ۴ — توابع

> در این فصل تعریف، پارامتر، مقدار بازگشتی و inline-expansion توابع را می‌آموزید.

## تعریف

هر برنامه حداقل یک تابع `main` دارد:

```python
def main():
    motor_run = start_button and not stop_button
```

سایر توابع با `def` کنار `main` (در سطح بالا) تعریف می‌شوند:

```python
def is_too_hot(temp):
    result = temp > 85.0

def main():
    if is_too_hot(sensor_temp):
        fan = True
```

## پارامترها

پارامترها **بی‌نوع** هستند — می‌توانند بولی یا عددی باشند:

```python
def set_motor(run, speed):
    motor_run = run
    motor_speed = speed

def main():
    set_motor(start_button and not stop_button, 1000)
```

## ساختار توابع

QPLC از قراردادهای IEC 61131-3 پیروی می‌کند:

| مفهوم QPLC | معادل IEC |
|-----------|-----------|
| `def main():` | `FUNCTION_BLOCK "QPLC_Main"` در SCL |
| `def aux():` | `FUNCTION aux` در SCL |
| `result` متغیر | متغیر محلی/خروجی FB |

## مقدار بازگشتی

QPLC توابع را **inline** می‌کند — یعنی بدنه تابع در محل فراخوانی کپی می‌شود و پارامترها با آرگومان‌های واقعی جایگزین می‌شوند (مانند ماکرو):

```python
def scale(value, factor):
    result = value * factor

def main():
    scaled = scale(analog_input, 2.5)
```

در خروجی Ladder، این به‌صورت یک جابه‌جایی واحد (`MOV`) دیده می‌شود. عمق‌تو‌در‌تو حداکثر ۸ است — فراخوانی بازگشتی بی‌پایان در semantic رد می‌شود.

## تعریف زودهنگام

ترتیب تعریف مهم نیست:

```python
def main():
    check_alarm()        # قبل از تعریف `check_alarm` فراخوانی شده

def check_alarm():
    if pressure > HIGH_P:
        sound_alarm = True
```

## کلمات کلیدی مرتبط

- `return` — خروج از تابع کاربر (در `main` معادل EXIT است)
- `break` / `continue` — فقط داخل حلقه‌ها

```python
def find_first():
    i = 0
    while i < 10:
        if sensor_array[i]:
            result = i
            return
        i = i + 1
```

---

[ادامه → فصل ۵، تایمرها و شمارنده‌ها](ch05-timers-counters)
