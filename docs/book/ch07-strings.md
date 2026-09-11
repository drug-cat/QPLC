# فصل ۷ — رشته‌ها

> در این فصل نوع `STRING` را با همه عملیاتش می‌آموزید: دگرگونی، برش، دگرگونی مورد، الحاق و توابع داخلی.

## رشته‌ها چیستند؟

رشته توالی ثابتی از کاراکترهاست. در QPLC با کوتیشن دوتایی:

```python
greeting = "Good morning, station"
```

برخلاف Python، رشته‌های QPLC در ladder به‌صورت **literal** در جاهای کاربرد جایگزین می‌شوند (مانند زمان‌های `T#5s`) و برای عملیات روی آن‌ها توابع داخلی به کار می‌روند.

## اعمال پایه

| عملیات | نحو | نتیجه |
|--------|-----|--------|
| طول | `len(msg)` | INT |
| دگرگونی | `msg[i]` | STRING یک‌کاراکتری |
| برش | `msg[a:b]` | STRING |
| پیشوند/پسوند چک | `starts_with(msg, p)` | BOOL |
| عددی‌بودن | `is_numeric(msg)` | BOOL |
| ترجمه به عدد | `to_number(msg)` | INT/REAL |
| دگرگونی مورد | `to_upper(msg)` | STRING |
| حذف فاصله | `trim(msg)` | STRING |
| تکرار | `repeat(msg, n)` | STRING |
| پیدا کردن | `find(msg, sub)` | INT (یا -1) |
| جایگزینی | `replace(msg, old, new)` | STRING |

## مثال واقعی

```python
def main():
    tag = "Station-7:Motor-2"
    short = tag[8:14]          # "Motor-2"

    if starts_with(tag, "Station-"):
        scada_ok = True

    server_id = to_number("4200")   # 4200
    if is_numeric(raw_input):
        speed = to_number(raw_input)
```

## برش‌ها (Slices)

`msg[a:b]` — از `a` تا `b-1`:

```python
message = "PLC Rocks"
word1   = message[0:3]    # "PLC"
word2   = message[4:9]    # "Rocks"
last3   = message[-3:]    # "cks"
```

> اندیس منفی از انتها شمرده می‌شود؛ اگر `b` خالی باشد تا انتهای رشته.

## ساختار رشته از متغیرها (مثل f-string)

`"..." . var` رشته را به‌صورت الحاق می‌سازد:

```python
row = "Scan " . str(scan_count) . " done"
```

> `str(x)` هر مقدار را به رشته تبدیل می‌کند؛ `int_to_str`/`real_to_str` نیز همین کار را می‌کنند.

## مقایسه رشته‌ها

```python
if tag == "STATION-OK":
    ready_lamp = True
```

مقایسه با `==`/`!=` روی مقدار مقایسه می‌کند نه روی آدرس.

## مدیریت خطا روی رشته

ترکیب با `try`:

```python
try:
    speed = to_number(raw_value)
except ValueError:
    speed = 0
    error_lamp = True
```

> `to_number` روی ورودی غیرعددی `ValueError` می‌اندازد (فصل ۹ را ببینید).

---

[ادامه → فصل ۸، ماژول‌ها](ch08-modules)