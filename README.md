<img width="1908" height="982" alt="Screenshot 2026-10-30 191752" src="https://github.com/user-attachments/assets/d8c51d28-c895-4192-92fa-b34666af42eb" /># Automotive ECU Watchdog & Recovery Framework

**Real-time fault detection and priority-based recovery for simulated automotive ECUs on QNX Neutrino RTOS**

![QNX](https://img.shields.io/badge/RTOS-QNX%20Neutrino%208.0-blue)
![Platform](https://img.shields.io/badge/Target-Raspberry%20Pi%204-c51a4a)
![Language](https://img.shields.io/badge/Language-C-informational)

**Team:** Bit-Flippers  |  **Event:** QNX-eHACK 2026  |  **Domain:** Automotive Embedded Systems & Real-Time Operating Systems

---

## Table of Contents

1. [Overview](#overview)
2. [Key Features](#key-features)
3. [System Architecture](#system-architecture)
4. [How It Works](#how-it-works)
5. [Timing and Scheduling](#timing-and-scheduling)
6. [Fault Injection](#fault-injection)
7. [Results](RESULTS.md)
8. [Build and Run](#build-and-run)
9. [Repository Structure](#repository-structure)
10. [Limitations](#limitations)
11. [Future Scope](#future-scope)
12. [Team](#team)

---

## Overview

Modern vehicles depend on many Electronic Control Units (ECUs) for braking, engine management, infotainment and more. When one ECU crashes, hangs, or runs too slowly, its service is interrupted, and restarting the whole system to fix a single process disrupts everything else.

This project implements a **watchdog supervisor** that monitors several independent ECU processes, detects failures through heartbeats, and restarts only the affected ECU. Safety-critical ECUs are recovered first. Unaffected ECUs keep running without interruption.

The framework was built in C on **QNX Neutrino RTOS 8.0** and runs on a **Raspberry Pi 4**. It uses QNX process isolation, pulse-based IPC, POSIX timers and real-time scheduling.

---

## Key Features

- **Heartbeat-based monitoring** of three simulated ECUs (Brakes, Engine, Infotainment) using QNX pulses
- **Three fault types detected:** process crash, deadlock (heartbeat stops while the process is alive) and execution overrun (cycle exceeds its time budget)
- **Targeted recovery:** only the failed ECU is restarted, other ECUs are not disturbed
- **Priority-based restart order:** Brakes, then Engine, then Infotainment, including when all three fail together
- **Live terminal dashboard** with status, cycle time, detection time, restart time and downtime
- **Failure timeline** on screen and logged to `/tmp/ecu_timeline.log`
- **Real-time scheduling:** the watchdog runs at a higher priority than every ECU, so it can always react
- **Built-in fault injection** from the keyboard for live demonstrations

---

## System Architecture


<img width="792" height="399" alt="image" src="https://github.com/user-attachments/assets/6cae013a-d0fb-4f07-bcd6-cb5686309a97" />




<img width="778" height="471" alt="image" src="https://github.com/user-attachments/assets/e8d31f8b-7d66-4e0f-88ad-f538d8947f05" />



| Component | Role |
|---|---|
| **ECU processes** (Brakes, Engine, Infotainment) | Separate processes that do periodic work and send a heartbeat every 100 ms |
| **Watchdog** (main thread) | Receives heartbeat pulses and a 100 ms timer pulse, and decides when an ECU has failed |
| **Recovery Manager** | Restarts failed ECUs in priority order, then verifies recovery |
| **Health Monitor** | Draws the dashboard and the failure timeline |
| **Fault Injection** | Lets the operator crash, deadlock or overrun any ECU from the keyboard |
| **QNX IPC (pulses)** | Carries heartbeats, the ECU id, instance number and last cycle time |

---

## How It Works

1. **Start-up.** The supervisor creates a named channel and spawns the three ECU processes.
2. **Heartbeats.** Each ECU sends a pulse every 100 ms carrying its ID, instance number and the execution time of its last cycle.
3. **Monitoring.** A POSIX timer ticks every 100 ms. If an ECU did not send a heartbeat since the last tick, its missed-beat counter increases.
4. **Detection.** The ECU is declared failed in one of two ways:
   - **3 missed heartbeats (300 ms timeout):** covers crashes and deadlocks
   - **3 consecutive cycles over the 30 ms budget:** covers execution overruns
5. **Recovery.** The Recovery Manager waits a short settling window to collect simultaneous failures, then restarts failed ECUs **in priority order**.
6. **Verification.** An ECU is only marked recovered when its **first heartbeat from the new process** arrives. Heartbeats from older instances are ignored.
7. **Logging.** Every event is shown on the dashboard and written to `/tmp/ecu_timeline.log`.

---

## Timing and Scheduling

### Configuration

| Parameter | Value | Purpose |
|---|---|---|
| Heartbeat interval | 100 ms | Period of each ECU cycle and heartbeat |
| Watchdog tick | 100 ms | Timer pulse used to check for missed heartbeats |
| Failure timeout | 300 ms (3 missed beats) | Threshold for declaring an ECU unresponsive |
| Overrun budget | 30 ms per cycle | Maximum allowed execution time of one cycle |
| Overrun limit | 3 consecutive cycles | Overruns tolerated before a restart |
| Settling window | 200 ms | Collects simultaneous failures before ordering recovery |

### Thread and process priorities

| Task | Policy | Priority |
|---|---|---|
| Watchdog (main thread) | SCHED_FIFO | 50 |
| Recovery Manager | SCHED_FIFO | 50 |
| Health Monitor (dashboard) | SCHED_FIFO | 30 |
| Fault injection input | SCHED_FIFO | 10 |
| Brakes ECU | SCHED_RR | 20 |
| Engine ECU | SCHED_RR | 15 |
| Infotainment ECU | SCHED_RR | 10 |

Shared state is protected with a priority-inheritance mutex to avoid priority inversion.

---

## Fault Injection

Type a command in the supervisor console and press **Enter**.

| Command | Fault | What it demonstrates |
|---|---|---|
| `1` / `2` / `3` | **Crash** Brakes / Engine / Infotainment (process killed) | Detection by missed heartbeats, then automatic restart |
| `4` | **Dual fault:** all three ECUs crash together | Priority-based recovery order |
| `h1` / `h2` / `h3` | **Deadlock** the chosen ECU (process alive, heartbeat stops) | Detects hangs that a simple "is the process alive" check would miss |
| `o1` / `o2` / `o3` | **Overrun:** cycles take 150 ms against a 30 ms budget | Timing-fault detection with a 3-strike rule |
| `q` | Quit the supervisor | Clean shutdown |

---

## Results

Measured detection and recovery times, CPU usage from the QNX System Profiler, the thread timeline and all screenshots are in **[RESULTS.md](RESULTS.md)**.

| Highlight | Value |
|---|---|
| Crash and deadlock detection | 300 to 400 ms |
| Process restart | 1 to 3 ms |
| System CPU idle during the run | about 99 % |

---

## Build and Run

### Requirements

- QNX Software Center with **QNX Neutrino RTOS 8.0** and **QNX Momentics IDE**
- Raspberry Pi 4 running a QNX image, connected to Momentics over Ethernet
- Root access on the target (needed for real-time priorities)

### Steps

1. **Import** the four projects into Momentics: the supervisor and the three ECU projects.
2. **Set the ECU identity.** All three ECUs use the same source file. Only these three lines change:

   | ECU | `ECU_NAME` | `ECU_ID` | `ECU_PRIO` |
   |---|---|---|---|
   | Brakes | `"Brakes ECU"` | `0` | `20` |
   | Engine | `"Engine ECU"` | `1` | `15` |
   | Infotainment | `"Infotainment ECU"` | `2` | `10` |

3. **Build** all four projects for the **AArch64 little-endian** target.
4. **Deploy** the three ECU binaries to `/tmp` on the Pi. The supervisor starts them from the paths in its `ecu[]` table, so these must match your file names:

   ```c
   { .name = "Brakes ECU",       .prio = 1, .path = "/tmp/pi_brakes_ecu" },
   { .name = "Engine ECU",       .prio = 2, .path = "/tmp/pi_engine_ecu" },
   { .name = "Infotainment ECU", .prio = 3, .path = "/tmp/pi_infotainment_ecu" },
   ```

5. **Run** the supervisor on the target (from Momentics or over `ssh`). It starts all three ECUs and shows the dashboard.
6. **Inject faults** with the commands in the table above.

> **Console tip:** set `PLAIN_CONSOLE` to `1` in the supervisor if you run inside the Momentics console. Set it to `0` in an ssh terminal to get colours and in-place redraw.

---

Console Outputs : 

<img width="1908" height="982" alt="Screenshot 2026-10-30 191752 - Copy" src="https://github.com/user-attachments/assets/6bd29d39-a6a9-4d00-86d2-5e2e632b0554" />

<img width="1917" height="985" alt="Screenshot 2026-10-30 191816" src="https://github.com/user-attachments/assets/4f55eb12-27b1-4491-ad73-096f55cddd2e" />

<img width="1916" height="972" alt="Screenshot 2026-10-30 191833" src="https://github.com/user-attachments/assets/5c8cd0d2-b960-4c9a-9dea-f769f86899ed" />

![Uploading Screenshot 2026-10-30 191850.png…]()



## Repository Structure

```text
.
├── pi_supervisor/        Watchdog supervisor (watchdog, recovery, dashboard, fault injection)
├── pi_brakes_ecu/        Brakes ECU process
├── pi_engine_ecu/        Engine ECU process
├── pi_infotainment_ecu/  Infotainment ECU process
├── docs/images/          Screenshots used in RESULTS.md
├── RESULTS.md            Measurements, profiler output and screenshots
└── README.md
```

Adjust the names above to match your repository.

---

system profiler :

<img width="1911" height="1001" alt="Screenshot 2026-10-31 000517" src="https://github.com/user-attachments/assets/7e0da324-20f3-4cb2-800f-ab8fa5aa5bbe" />

Thread Timeline :

<img width="1912" height="998" alt="Screenshot 2026-10-30 231827" src="https://github.com/user-attachments/assets/da795b6e-5ed1-4624-a3c7-d9faecb25bb9" />



## Limitations

- The ECUs are **simulated processes**, not real automotive hardware or software.
- A missing heartbeat does not always mean a crash. The framework treats crashes, deadlocks and long stalls the same way.
- Detection time depends on the 100 ms tick, so it varies between 300 and 400 ms.
- Recovery depends on the ECU being able to restart and initialise cleanly.
- If the supervisor itself fails, monitoring stops.
- The 2 s demo hold is for display only. Set `DEMO_HOLD_MS` to `0` to get raw timing.
- This is a prototype, **not a certified automotive safety system**.

---

## Future Scope

- Integrate real hardware: GPIO status LEDs, buttons and physical sensors on the Raspberry Pi
- Escalate to a fail-safe mode after repeated failed restarts, with a restart limit and back-off
- Add a persistent black-box event log
- Add health scoring and predictive warnings from heartbeat lateness
- Monitor the watchdog itself with a self-check or hardware watchdog
- Investigate the QNX High Availability Manager for additional recovery options
- Scale to more ECUs and add diagnostic interfaces
- Evaluate CPU affinity and partitioning for stronger isolation


Built for **QNX-eHACK 2026**.
