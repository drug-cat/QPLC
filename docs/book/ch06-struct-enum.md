# فصل ۶ — struct و enum: سیستم نوع شما

> این فصل قلب پیشرفت بزرگ QPLC v0.2.0 است. structها داده‌های مرتبط را در یک بسته می‌چینند، enumها مجموعه‌های وضعیت گسسته را مدل می‌کنند و `match` انتخاب الگو را آسان می‌کند.

## تعریف struct

```python
struct Motor:
    name:      STRING
    base_speed: INT
    running:   BOOL
    temp:      REAL

def main():
    m = Motor(name="M1", base_speed=1000, running=False, temp=20.0)
```

گزاره `def main():` خودش یک struct مخفی است — تمام متغیرهای سطح تابع، فیلدهای آن هستند! (بنابراین QPLC در نهایت یک زبان «همه‌چیز struct» است.)

### دسترسی به فیلد

```python
motor_speed = m.base_speed
m.running   = True
m.temp      = m.temp + 1.0
```

فیلدها را می‌توان خواند (`speed = m.speed`)، نوشت (`m.speed = 100`) و در عبارت محاسباتی به کار برد.

### struct تو‌در‌تو

```python
struct Axle:
    position: REAL
    broken:   BOOL

struct Joint:
    angle:      REAL
    motor:      Motor   # struct دیگر به‌عنوان فیلد
    axle:       Axle

j = Joint(angle=0.0, motor=Mot..., axle=Axle(position=30.5, broken=False))
j.axle.position = 45.0     # دسترسی زنجیره‌ای
```

## enum

```python
enum State:
    Idle
    Running
    Fault
    Stopping

def main():
    current = Idle
    current = Running
```

مقادیر enum مقادیر صحیح هستند که در پشت پرده با `0، 1، 2 …` جایگزین می‌شوند — برای مقایسه:

```python
if current == Running:
    lamp = True
```

> در خروجی XML، enumها به‌صورت INT با لیبل نمایش داده می‌شوند تا HMI/SCADA بتواند مقدار را به نام وضعیت نگاشت کند.

## match — انتخاب الگو

`match` مثل `switch` هست ولی قوی‌تر — با structها و enumها جفت می‌شود:

```python
def main():
    match current:
        case Idle:
            ready_lamp = True
        case Running:
            running_lamp = True
        case Fault:
            alarm_lamp = True
        case _:
            unknown_lamp = True
```

هر branch بعد از `=` می‌تواند `break` یا مقدار داشته باشد:

```python
def classify(speed):
    match speed:
        case 0:
            label = "STOPPED"
        case 1000:
            label = "FULL"
        case _:
            label = "NA"
```

### عبارت match به‌جای دستور

```python
priority = match ch_state:
    case Idle:    1
    case Running: 2
    case Fault:   3
    case _:       0
```

## نکته طراحی: enum به‌جای BOOL

به‌جای این:

```python
valve_1 = True      # مبهم است
```

استفاده کنید:

```python
valve = OPEN
```

حالا کد خودش را مستند می‌کند و اگر روزی «نیمه‌باز» اضافه شود، جا دارد.

---

[بعدی → فصل ۷، رشته‌ها](ch07-strings)
