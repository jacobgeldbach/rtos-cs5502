# CS 5502 — Real-Time Operating Systems

Coursework for CS 5502-71 (Real-Time Operating Systems), M.S. Artificial
Intelligence, University of Idaho — Jacob A. Geldbach.

All work targets an ESP32 (Valduino) on a hand-soldered I/O development board,
built with ESP-IDF against FreeRTOS.

---

## Final Project — Real-Time Sonar Mapping System

A rotating ultrasonic rangefinder that sweeps a 120° arc and streams what it
detects to a live web display, served from the ESP32 itself over local WiFi.

An ultrasonic sensor rides on a stepper motor that sweeps back and forth. A
sensor task fires the rangefinder and records distance; a motor task drives the
sweep and records the current angle; together they maintain a shared
position-and-distance state. A web server task hosts a page on the local network
and a broadcaster task pushes each new reading to every connected browser, which
plots the target on a polar display in real time. Three physical buttons control
the sweep — direction, speed, and stop.

### Concurrency Design

The interesting part of this project is the RTOS design rather than the
peripherals.

| Task | Core | Stack | Role |
|------|------|-------|------|
| `drive_stepper_motor` | 0 | 2048 | Drives the half-step sequence, tracks sweep angle, handles button events |
| `v_hcsr04_task` | 0 | 8192 | Triggers the ultrasonic sensor and times the echo pulse |
| `v_web_server` | 1 | 8192 | Accepts TCP connections and serves the page |
| `v_stream_radar_data_task` | 1 | 8192 | Broadcasts readings to connected clients |

Work is split deliberately across the ESP32's two cores: timing-sensitive motor
and sensor work on core 0, networking on core 1, so a blocking socket operation
can never disturb step timing.

The four tasks share a single `radar_data_t` struct holding current angle,
measured distance, and an in-range flag, protected by a mutex. The sensor and
broadcaster tasks block on that mutex normally, but the motor task takes it
**non-blocking** (`xSemaphoreTake(..., 0)`) and simply skips the angle update if
it is contended — a missed update is harmless, a stalled step is not.

Two further real-time details:

- **Buttons are interrupt-driven, not polled.** Each GPIO ISR calls
  `xTaskNotifyFromISR` with `eSetValueWithOverwrite`, passing the button's ID as
  the notification value directly to the motor task. This replaced an earlier
  polling task (still present in `buttons.c` as `v_button_detection`). Debounce
  is deferred out of the ISR into the receiving task.
- **Step timing bypasses the scheduler.** Even at a 1000 Hz tick, `vTaskDelay`
  cannot express a delay shorter than 1 ms, which is too coarse for stepper pulse
  intervals. The motor loop uses `esp_rom_delay_us` for its 2500/1750/1200 µs
  step delays instead.

### Web Interface

The server is written directly against BSD sockets rather than an HTTP library.
It serves a self-contained HTML/JS page, then holds open a Server-Sent Events
stream (up to 4 concurrent clients) pushing one JSON message per reading:

```
data: {"angle":73.5,"distance":2.81,"valid":true}
```

The browser renders these as a polar sweep display. Angle is inverted before
transmission so the on-screen sweep runs left-to-right regardless of the motor's
physical winding order.

### Hardware

| Component | Connection |
|-----------|------------|
| HC-SR04 ultrasonic rangefinder | `TRIG` GPIO22, `ECHO` GPIO23 |
| 4-wire unipolar stepper + driver board | GPIO16, GPIO17, GPIO21, GPIO14 |
| Buttons ×3 | GPIO32, GPIO15, GPIO18 |

The stepper takes 4096 half-steps per revolution; the sweep is limited to one
third of that (1365 half-steps, 120°). Distance is derived from echo pulse width
at 148 µs/inch, and a target is flagged in range at 6 feet or nearer.

**Controls:** button 2 sweeps in one direction, button 3 reverses; pressing
either again cycles through three speeds. Button 1 stops the sweep, and pressing
it again at rest walks the motor back to its home position and de-energizes the
coils.

### Building

Requires ESP-IDF 4.1 or newer.

```sh
cd final_project
cp main/include/wifi_secrets.h.example main/include/wifi_secrets.h
# edit wifi_secrets.h with your network name and password
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

The device connects in station mode and logs its IP address on boot. Open that
address in a browser on the same network to view the display.

`wifi_secrets.h` is gitignored — credentials stay out of the repository.

---

## Homework

| | Topic |
|---|---|
| [hw-1](hw-1) | Introduction and toolchain setup |
| [hw-2](hw-2) | |
| [hw-3](hw-3) | |
| [hw-4](hw-4) | Buttons and seven-segment display |
| [hw-5](hw-5) | |
| [hw-6](hw-6) | |
| [hw-7](hw-7) | |

## Layout

```
final_project/
  main/
    src/        implementation
    include/    headers
hw-N/           weekly assignments
```
