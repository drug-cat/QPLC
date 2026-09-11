# فصل ۱۰ — پروژه واقعی: کنترل دمای یک موتور

> در این فصل هر آنچه در فصل‌های ۱ تا ۹ آموخته‌اید را در یک پروژه صنعتی واقعی به کار می‌گیرید.

## سناریو

یک موتور سه‌فاز داریم که باید:
1. با دکمه START روشن و با دکمه STOP خاموش شود
2. در دمای بالای ۸۵ درجه، یک فن خنک‌کننده روشن کند
3. در صورت خطای تایمر، پیام مناسب ثبت کند
4. تعداد روشن‌شدن‌ها را بشمارد

## فایل پیکربندی

```ini
[hardware]
cpu = S7-1214C
ip = 192.168.0.1

[io]
start_button = I0.0:BOOL
stop_button  = I0.1:BOOL
temp_sensor  = IW64:INT
motor_run    = Q0.0:BOOL
fan          = Q0.1:BOOL
start_count  = MW10:INT
```

## کد کامل

```python
struct Motor:
    name: STRING
    speed: INT
    running: BOOL

def main():
    # ۱. راه‌اندازی
    motor_run = start_button and not stop_button

    # ۲. تشخیص دما و فن
    if temp_sensor > 85:
        fan = True
    else:
        fan = False

    # ۳. شمارش روشن‌شدن‌ها (با لبه صعودی)
    if rising_edge(start_button):
        start_count = start_count + 1

    # ۴. استفاده از struct
    m = Motor(name="M1", speed=0, running=motor_run)

    # ۵. مدیریت خطا
    try:
        if m.speed > 1000:
            raise RuntimeError("speed exceeds 1000")
    except RuntimeError:
        print("Motor fault: " + type(0) + "")
```

## اجرا و مشاهده

```bash
qplc conf.qplc main.q -o output.xml
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll conf.qplc output.xml
```

در REPL:
- `set temp_sensor 90` → فن باید روشن شود
- `set start_button True` یک‌بار → `start_count` باید ۱ شود
- `set start_button False` → خاموش شود

## نکات حرفه‌ای

- **نام‌گذاری**: از استاندارد IEC 61131-3 استفاده کنید (`start_button`، `motor_run`، نه `sb`/`mr`).
- **ثابت‌ها**: همه عددهای جادویی را در `[constants]` تعریف کنید.
- **ساختار**: برنامه را به structهای منطقی تقسیم کنید تا قابل نگهداری باشد.

---

## خلاصه کتاب

تبریک! شما هر ۱۰ فصل را کامل کرده‌اید:
- [x] فصل ۱ — شروع کار
- [x] فصل ۲ — مفاهیم پایه
- [x] فصل ۳ — کنترل جریان
- [x] فصل ۴ — توابع
- [x] فصل ۵ — تایمر و شمارنده
- [x] فصل ۶ — struct و enum
- [x] فصل ۷ — رشته‌ها
- [x] فصل ۸ — ماژول‌ها
- [x] فصل ۹ — مدیریت خطا
- [x] فصل ۱۰ — پروژه واقعی

حالا شما می‌توانید **QPLC.Studio** را اجرا کنید، یک فایل `conf.qplc` و یک برنامه `main.q` بسازید و لدر خود را ببینید. برای تماس با جامعه توسعه‌دهندگان به `CONTRIBUTING.md` در ریشهٔ ریپو مراجعه کنید.
