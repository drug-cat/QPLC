# فصل ۸ — ماژول‌ها و import

> هر برنامه‌ای که بزرگ می‌شود باید به فایل‌های کوچک‌تر تقسیم شود. QPLC مثل Python یک سیستم ماژول مبتنی بر فایل دارد.

## ساختار

یک فایل `.q` می‌تواند `import` کند:

```python
import "utils.q" as utils

def main():
    result = utils.clamp(base_speed, 0, 100)
```

- `utils.q` در همان پوشهٔ فایل اصلی جستجو می‌شود.
- `as utils` نام مستعار می‌دهد — بدون آن، از نام فایل استفاده می‌شود.
- توابع واردشده با پیشوند ماژول فراخوانی می‌شوند: `utils.clamp(...)`.

## دسترسی از ماژول به متغیرهای سراسری

QPLC به‌دلایل ladder همه‌چیز را در یک بافت سراسری نگه می‌دارد، پس:

```python
# utils.q
def clamp(v, lo, hi):
    if v < lo:
        result = lo
    elif v > hi:
        result = hi
    else:
        result = v
```

تابع به‌جای `return`، متغیر سراسری `result` را می‌نویسد. پس از فراخوانی، `result` در دسترس است:

```python
utils.clamp(inputs[0], 0, 100)
scaled = result
```

## تجزیه

imp‌ها پس از تمام توابع top-level در شاخه `import` لیست می‌شوند و parser آن‌ها را جداگانه پارس می‌کند. در فاز ماژول، محتوای فایل واردشده در همان برنامه merge می‌شود و semantic یکبار کل برنامه را می‌بیند.

## نمونهٔ واقعی: کتابخانهٔ ریاضی

`examples/conf.qplc` تمام توابع ریاضی استاندارد IEC (MIN/MAX/ABS/LIMIT/SEL/MUX) را تعریف می‌کند، در حالی که کد فقط آن‌ها را صدا می‌زند:

```python
if utils.clamp(temperature, 20.0, 90.0) > 60:
    fan = True
```

## تست

تست دایمی `module_test` در `tests/run_tests.sh` وجود دارد و سیستمعامل‌های Windows/Linux هر دو آن را اجرا می‌کنند (33/33 فعلی).

---

[بعدی → فصل ۹، مدیریت خطا](ch09-errors)
