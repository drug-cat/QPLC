# فصل ۹ — مدیریت خطا

> در این فصل ساختارهای `try/except/finally`، `assert`، `raise` و الگوهای مبارزه با خطا را می‌آموزید. هدف: برنامه‌ای که در دنیای نویزی PLC خراب نشود.

## اگر خطایی رخ دهد چه باید کرد؟

در PLC، «خطا» معمولاً یعنی: سخت‌افزار از کار افتاده، تایمر درست کار نکند، مقدار آرایه از حد خارج شود، یا سنسور مقدار نامعتبر بدهد. QPLC سه ابزار دارد:

### ۱. انتشار صریح با raise

`raise Name("پیام")` یک خطا را در هر نقطه صدا می‌زند:

```python
def main():
    if not power_checked:
        raise PowerError("Power supply not verified")
```

### ۲. جذب با try/except

```python
def main():
    try:
        result = read_average(sensor)
        outputs[0] = result > 50.0
    except SensorFault:
        outputs[0] = False
        fault_lamp = True
```

- `except Type:` — فقط آن نوع
- `except:` — هر نوع (catch-all)
- `except Type, Type2:` — چند نوع
- متغیر خطا با `as e`:

```python
except SensorFault as e:
    msg = e.message
```

### ۳. تمیزسازی با finally

```python
def main():
    try:
        outputs[2] = do_calculation(x)
    finally:
        watchdog_reset = True    # همیشه اجرا می‌شود، حتی اگر خطا شود
```

`finally` بدون توجه به موفقیت یا خطا، قبل از خروج اجرا می‌شود.

## assert — قرارداد درون‌برنامه‌ای

`assert شرط, "پیام"`:

- اگر شرط نادرست باشد → خطای `AssertionError`
- فقط در `def` — در `main` نوشتن assert فایده‌ای ندارد چون خروجی به SCADA می‌رود

```python
def calculate(x):
    assert x >= 0, "input cannot be negative"
    result = sqrt(x) * 2
```

## الگوهای مبارزه با خطا

### الگو ۱ — مقدار پیش‌فرض امن (مثل `unwrap_or` در Rust)

```python
def read_temp():
    try:
        result = to_number(raw_text)
    except ValueError:
        result = -50.0    # مقدار امن — باعث هشدار نمی‌شود
```

### الگو ۲ — انتشار به بالا (مثل `?` در Rust)

```python
def read_two_sensors():
    try:
        a = read_sensor(1)
        b = read_sensor(2)
    except SensorFault as e:
        raise LinkError("Sensor comm lost: " + e.message)
```

### الگو ۳ — گفتن به اپراتور

```python
def main():
    try:
        speed = get_speed()
    except:
        msg_box = "Fault — contact maintenance"
        speed = 0
```

## چک‌لیست خطاها

| ترکیب | معنی |
|-------|------|
| `try/except` | تلاش + جذب |
| `try/finally` | تلاش + تمیزسازی تضمینی |
| `try/except/finally` | هر سه |
| `try/except ... as e` | دسترسی به پیام |
| `try/finally/except` | ترتیب پذیرفته شده نیست — semantic خطا می‌دهد |
| `except:` بدون نوع | catch-all |
| `raise` خارج از try | خطای اجرایی |
| `finally` بدون try | خطای parse |
| `assert` در main | بی‌اثر (هشدار) |

## نمونهٔ کامل: HMI قابل اطمینان

```python
def main():
    watchdog_trip = False
    try:
        # شبکه اصلی
        t = read_sensor(1)
        if t > MAX_TEMP:
            valve_q = True
    except AnalogInputError as e:
        valve_q = False
        watchdog_trip = True
    except:
        valve_q = False
    finally:
        status_bit = not watchdog_trip
```

---

## جمع‌بندی کتاب

اکنون هر ده فصل را خوانده‌اید. چند تمرین پایانی:

1. یک struct `Tank` با فیلدهای `level`, `full`, `header` بسازید و یک ماژول `tank_utils` بنویسید که ارتفاع/حجم را حساب کند.
2. در `main` یک `try/except` بگذارید که اگر سنسور `level` فالتی شد، `level` را روی «خالی» و خروجی `pump` را خاموش کند.
3. یک enum `AlarmLevel {Info, Warning, Critical}` بسازید و با `match` سه شدت را به سه خروجی متصل کنید.
4. برنامه را با `qplc conf.qplc project.q -o out.xml` کامپایل و همان را به «پروژهٔ AC2» پیوند دهید.

---

کار تمام است. 🎉 پروژهٔ QPLC آمادهٔ انتشار است — فصل‌های کتاب را می‌توانید در `docs/book/` ویرایش و در ReadTheDocs تماس بگیرید.

[برگشت به فهرست کتاب → index](index)
