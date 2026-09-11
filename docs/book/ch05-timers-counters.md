# فصل ۵ — تایمرها و شمارنده‌ها

> در این فصل تایمرهای IEC (TON/TOF/TP)، تایمرهای استاندارد، شمارنده‌ها و تشخیص لبه را می‌آموزید — ستون فقرات هر برنامه PLC.

## تایمرها

هر تایمر یک بلوک تابع است که فعلاً در زبان، یک خروجی بولی برمی‌گرداند:

```python
motor_run = TON(start_button, T#5s)
```

| نام QPLC | نام IEC | ورودی | زمان |
|----------|---------|-------|------|
| `on_delay`/`TON` | TON | BOOL | TIME |
| `off_delay`/`TOF` | TOF | BOOL | TIME |
| `pulse`/`TP` | TP | BOOL | TIME |

```python
cooling = busy and on_delay(overheat_sensor, T#50s)
motor   = on_delay(start_button, T#5s)  # از خروجی تایمر به‌عنوان ورودی تایمر دیگر
```

`T#` می‌تواند با ثابت تعریف‌شده در `[constants]` جایگزین شود:

```python
if temperature > MAX_TEMP:
    delay = on_delay(alarm_sensor, COOLING_DELAY)
```

## زمان

زمان‌ها با `T#` نوشته می‌شوند و در SCL به لیترال استاندارد IEC تبدیل می‌شوند:

```python
start_delay = T#2s
long_delay  = T#100ms
hourly      = T#1h
mixed       = T#1m30s     # یک دقیقه و نیم
```

| واحد | معنی |
|------|------|
| `s` | ثانیه |
| `ms` | میلی‌ثانیه |
| `m` | دقیقه |
| `h` | ساعت |

## عملیات روی زمان

```python
# مقایسه مقادیر ET یک تایمر
if timer_et > T#120s:
    alarm = True

# مجموع/تفاضل زمان
total = T#5s + T#3s
half  = T#8s / 2
```

## شمارنده‌ها

```python
done = count_up(part_in_sensor, reset, 10)     # CTU
```

| نام QPLC | نام IEC | پارامترها |
|----------|---------|-----------|
| `count_up` | CTU | (in, reset, preset) |
| `count_down` | CTD | (in, load, preset) |
| `count_updown` | CTUD | (up, down, reset, load, preset) |

```python
batch_done = count_up(piece_sensor, reset_batch, 10)
start      = count_down(close_btn, load_btn, 3)
bidi       = count_updown(up, down, reset, load, 5)
```

## تشخیص لبه

| نام QPLC | نام IEC | توضیح |
|----------|---------|-------|
| `rising_edge` | R_TRIG | فعال در False→True |
| `falling_edge` | F_TRIG | فعال در True→False |

```python
start_pressed = rising_edge(start_btn)
```

لبه‌ها را می‌توانید داخل `if` استفاده کنید:

```python
if rising_edge(start_btn):
    cycle_count = cycle_count + 1
```

> **نکته**: از استفاده از لبه در وسط یک عبارت بولی که به چند شبکه تبدیل می‌شود خودداری کنید — روش تمیز، قرار دادن لبه در یک متغیر جدا و سپس استفاده از آن است.

---

[ادامه → فصل ۶، struct و enum](ch06-struct-enum)
