# فصل ۲ — مفاهیم پایه

> در این فصل متغیرها، انواع داده، عملگرها، رشته‌ها و ثابت‌ها را می‌آموزید.

## متغیرها

QPLC دو نوع متغیر دارد: **بولی** (`BOOL`) و **عددی** (`INT`/`REAL`/`TIME`). نوع هر متغیر در فایل `conf.qplc` (بخش `[io]`) و نه در کد تعیین می‌شود — یعنی تایپ از سخت‌افزار می‌آید، نه از برنامه.

```python
def main():
    motor_run = start_button and not stop_button
    speed = base_speed + 10
```

در اینجا `start_button` و `stop_button` از `[io]` و `base_speed` از `[constants]` آمده‌اند.

## رشته‌ها (String)

رشته‌ها با `"` ساخته می‌شوند. `len(...)` طول رشته و `print(...)` مقدار را خروجی می‌دهد:

```python
def main():
    prefix = "Motor"
    full_name = prefix + " #1"
    if len(prefix) > 5:
        lamp = True
    print(full_name)
```

برخی از نمونه‌ها:

```python
greeting = "Hello, PLC!"
length   = len(greeting)    # 13
begin    = greeting[0:5]    # "Hello"
```

## ثابت‌ها

در بخش `[constants]` تعریف می‌شوند. به آن‌ها نمی‌توان مقدار داد:

```python
def main():
    if speed > MAX_TEMP:    # MAX_TEMP از conf.qplc
        alarm = True
```

## عملگرهای مقایسه

`==`, `!=`, `<`, `>`, `<=`, `>=` خروجی بولی می‌دهند:

```python
if temperature >= 80.0:
    fan = True
```

## عملگرهای منطقی

`and`, `or`, `xor`, `not` برای بولی‌ها:

```python
lamp = start and not stop
alarm = (temp > 80) xor (level < 2)   # دقیقاً یکی
```

## عملگر سه‌تایی

```python
outputs[0] = True if enable else False
```

## تمرین

برنامه‌ای بنویسید که اگر طول فیلد متنی `message` بیشتر از ۱۰ بود چراغ `alarm` را روشن کند:

```python
def main():
    if len(message) > 10:
        alarm = True
```

---

[ادامه → فصل ۳، کنترل جریان](ch03-control-flow)
