# فصل ۱ — شروع کار

> در این فصل یاد می‌گیرید برنامه اول QPLC خود را بنویسید و اجرا کنید. این فصل برای کسانی است که هیچ تجربه‌ای با PLC یا QPLC ندارند.

## نصب

QPLC یک **کامپایلر خط فرمان** (`qplc`) + یک **شبیه‌ساز** است. هر دو از `build.qplc` ساخته می‌شوند:

```bash
# Windows (پوشه build را بسازید)
cmake -S . -B build -G Ninja
cmake --build build

# مطمئن شوید کار می‌کند
./build/qplc.exe --version
```

## ساختار پروژه

یک پروژه QPLC حداقل دو فایل دارد:

```
project/
├── conf.qplc      # سخت‌افزار + IO + ثابت‌ها
└── main.q         # کد برنامه
```

### فایل پیکربندی (`conf.qplc`)

```ini
[hardware]
cpu = S7-1200
ip = 192.168.0.1

[io]
start_button = I0.0:BOOL
motor_run    = Q0.0:BOOL

[constants]
MAX_TEMP = 80.0
```

### فایل برنامه (`main.q`)

```python
def main():
    motor_run = start_button and not stop_button
    if temperature > MAX_TEMP:
        cooling_valve = True
```

## کامپایل

```bash
qplc conf.qplc main.q -o output.xml
```

خروجی، فایل XML استاندارد IEC 61131-3 است که توسط شبیه‌ساز قابل اجرا و توسط QPLC.Studio قابل ویرایش است.

## اولین برنامه شما: Hello Motor

ساده‌ترین راه برای دیدن خروجی، اجرای مثال آماده است:

```bash
qplc examples/conf.qplc examples/hello_motor.q -o hello.xml
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc hello.xml
```

در REPL تایپ کنید:

```
set enable True
run
show
```

خروجی را می‌بینید: موتور روشن می‌شود و خروجی `motor_run` فعال است.

## تمرین

1. در `conf.qplc` یک متغیر `stop_button` اضافه کنید.
2. در `main.q` تماس اولیه را `motor_run = start_button and not stop_button` کنید.
3. دوباره کامپایل و اجرا کنید. ببینید وقتی `stop_button` را روی True بگذارید چه می‌شود.

---

[ادامه → فصل ۲، مفاهیم پایه](ch02-basic-concepts)
