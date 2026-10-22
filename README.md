# home-iot

هوشمندسازی خانه — مرحله ۱: کنترل روشنایی با ESP8266 + رله + MQTT + Home Assistant

## معماری
```
اپ Home Assistant (گوشی) ──► Home Assistant ──► Mosquitto (MQTT) ──► ESP8266 ──► رله ──► لامپ
```

## ساختار
- `firmware/esp8266-light/` کد ESP8266 (PlatformIO)
- `server/` فایل‌های Docker برای Mosquitto و Home Assistant
- `docs/` مستندات و نقشه سیم‌کشی

## سیم‌کشی تست (بدون برق شهری)
| ماژول رله | ESP8266 (NodeMCU) |
|-----------|-------------------|
| VCC       | 3V3 (یا VIN اگر رله ۵ ولتی است) |
| GND       | GND |
| IN        | D1 (GPIO5) |

برای تست: LED (با مقاومت ۲۲۰ تا ۴۷۰ اهم) و یک منبع کم‌ولتاژ را به پایه‌های COM و NO رله وصل کن.

## راه‌اندازی سرور (روی کامپیوتر همیشه روشن)
```bash
cd server
# ساخت کاربر MQTT (رمز را عوض کن)
docker run --rm -v "$PWD/mosquitto/config:/mosquitto/config" eclipse-mosquitto:2 \
  mosquitto_passwd -c -b /mosquitto/config/passwd iot YOUR_PASSWORD
docker compose up -d
```
سپس `http://IP-کامپیوتر:8123` را باز کن و Home Assistant را راه‌اندازی کن.
در HA: Settings → Devices & Services → Add Integration → **MQTT** → host=`localhost`، کاربر `iot` و رمزت.

> `network_mode: host` فقط روی لینوکس کار می‌کند. روی ویندوز/مک آن را بردار و پورت `8123:8123` بده، و در تنظیم MQTT به‌جای localhost از `mosquitto` استفاده کن.

## آپلود فرم‌ور
1. `include/secrets.example.h` را کپی کن به `include/secrets.h` و مقادیر را پر کن.
2. VS Code + افزونه PlatformIO → ESP8266 را با USB وصل کن → Upload.
3. پس از اتصال، دستگاه **Room Light** خودکار در Home Assistant ظاهر می‌شود.
4. اپ **Home Assistant** را روی گوشی نصب کن و به همان آدرس وصل شو.

## ایمنی
وصل کردن رله به برق ۲۳۰ ولت فقط با رعایت اصول ایمنی (قطع فیوز، جعبه عایق، سیم مناسب) یا توسط برق‌کار انجام شود.

## اپ PWA گوشی
پوشه `app/` یک اپ سبک برای کنترل چراغ است که Home Assistant آن را در آدرس زیر سرو می‌کند:

`http://IP-کامپیوتر:8123/local/lightapp/`

1. بعد از تغییر docker-compose: `docker compose up -d --force-recreate homeassistant`
2. در Home Assistant: پروفایل ← امنیت ← **Long-lived access tokens** ← ساخت توکن.
3. آدرس بالا را در Chrome گوشی باز کن، ⚙ را بزن، توکن و Entity ID (پیش‌فرض `light.room_light`) را وارد کن.
4. منو ← **Add to Home screen / Install app**.

> نصب کامل PWA (با service worker) فقط روی **HTTPS** یا localhost فعال می‌شود. روی `http://` معمولی فقط میان‌بر صفحه اصلی ساخته می‌شود که برای کار روزمره کافی است. برای HTTPS و دسترسی از بیرون خانه: Tailscale یا Cloudflare Tunnel (یا Nabu Casa) مناسب است.
