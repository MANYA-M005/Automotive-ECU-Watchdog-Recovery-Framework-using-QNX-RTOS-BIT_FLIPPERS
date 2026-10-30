🚗 Automotive ECU Watchdog & Recovery Framework 

Team Name: Bit-Flippers
Project Domain: Automotive 
Platform: QNX Neutrino RTOS
Programming Language: C

---

📌 1. Project Overview

Modern automotive systems rely on multiple Electronic Control Units (ECUs) to perform different functions, including braking, engine management, and infotainment. The failure or unresponsiveness of an ECU can interrupt its associated service and affect the overall system's reliability.

The Automotive ECU Watchdog & Recovery Framework aims to address this challenge through continuous monitoring of individual ECU processes, heartbeat-based failure detection, and targeted process recovery.

The framework uses QNX Neutrino RTOS to simulate multiple independent ECU processes and a watchdog supervisor. Each ECU periodically sends heartbeat messages to indicate that it is communicating with the supervisor.

The watchdog tracks heartbeat reception and identifies an ECU as unresponsive when the configured timeout is exceeded. It then attempts to restart the affected ECU process and verifies whether heartbeat communication resumes.

The project demonstrates a modular approach to process-level fault detection and recovery in an automotive embedded software environment.

---

⚠️ 2. Problem Statement

Automotive systems depend on multiple ECUs operating reliably and coordinating their respective functions. When an ECU crashes, hangs, or becomes unresponsive, its service may be interrupted.

Without continuous monitoring and an effective recovery mechanism, such failures may remain undetected, delaying service restoration.

Restarting the entire system to recover a single failed process may also cause unnecessary disruption to other services.

Therefore, an automotive ECU watchdog and recovery framework is needed to:

- Monitor the health of multiple ECU processes.
- Detect unresponsive processes using heartbeat monitoring.
- Identify failures within a defined timeout.
- Attempt recovery of the affected ECU process.
- Verify whether the recovered process resumes normal communication.
- Minimize unnecessary disruption to unaffected processes.
- Support performance evaluation through timing measurements and system logs.

---

🎯 3. Project Objectives

The main objectives of this project are:

1. Develop a framework for monitoring multiple simulated ECU processes.
2. Implement heartbeat-based health monitoring using QNX inter-process communication.
3. Detect ECU unresponsiveness using timeout-based logic.
4. Implement targeted recovery by attempting to restart the affected ECU process.
5. Verify recovery by checking for subsequent heartbeat messages.
6. Evaluate the framework under normal and fault-injection scenarios.
7. Analyze process execution and timing behavior using QNX development tools.

---

🏗️ 4. Proposed System Architecture

The proposed architecture consists of three simulated ECU processes and a watchdog supervisor.

Block Diagram



<img width="792" height="399" alt="image" src="https://github.com/user-attachments/assets/bc9c8d07-b47a-458a-845b-a764e63ff8c0" />











<img width="778" height="471" alt="image" src="https://github.com/user-attachments/assets/86844163-7484-42fa-8207-515421bbc2f2" />



ECU Processes

- ECU Brakes: Simulates a braking-related controller.
- ECU Engine: Simulates an engine-management controller.
- ECU Infotainment: Simulates an infotainment controller.

Watchdog Supervisor

The watchdog supervisor monitors the health of the ECU processes by receiving heartbeat messages and tracking their latest reception timestamps.

When an ECU exceeds the configured heartbeat timeout, the supervisor identifies the affected process and initiates the recovery procedure.

Recovery Mechanism

The recovery mechanism attempts to restart the affected ECU process. Following the restart attempt, the framework checks whether the process resumes sending heartbeat messages.

Communication

QNX inter-process communication (IPC) supports communication between the ECU processes and the watchdog supervisor.

The architecture aims to isolate individual process failures and recover the affected service without unnecessarily restarting the entire operating system.

---

🧩 5. System Components

Component| Description
ECU Brakes| Simulated ECU process responsible for generating heartbeat messages
ECU Engine| Simulated ECU process responsible for generating heartbeat messages
ECU Infotainment| Simulated ECU process responsible for generating heartbeat messages
Watchdog Supervisor| Monitors ECU health and detects heartbeat timeouts
Recovery Mechanism| Attempts to restart the affected ECU process
QNX IPC| Provides inter-process communication
QNX Momentics IDE| Supports development, debugging, and testing
QNX System Profiler| Helps analyze system execution and thread activity using collected trace data

---

⚙️ 6. Working Principle

The framework operates through the following stages.

Step 1: ECU Initialization

The simulated ECU processes and watchdog supervisor are started in the QNX environment.

Step 2: Heartbeat Generation

Each ECU periodically sends heartbeat messages to the watchdog supervisor to indicate that it is communicating.

Step 3: Health Monitoring

The watchdog receives heartbeat messages and records the latest heartbeat information for each ECU.

Step 4: Failure Detection

The watchdog periodically checks the elapsed time since each ECU's last heartbeat.

If the elapsed time exceeds the configured failure timeout, the corresponding ECU is marked as unresponsive.

Step 5: Targeted Recovery

The supervisor identifies the affected ECU and attempts to restart its process.

Step 6: Recovery Verification

The supervisor checks whether the restarted process resumes sending heartbeat messages.

Step 7: Logging and Evaluation

Failure-detection and recovery information is recorded for debugging and performance evaluation, depending on the logging implemented.

🔄 Overall Workflow

ECU Initialization → Heartbeat Monitoring → Timeout Detection → Failed ECU Identification → Recovery Attempt → Heartbeat Verification → Status Logging

---

⏱️ 7. Heartbeat Monitoring and Timing Configuration

The current design uses the following configured timing parameters.

Parameter| Value| Purpose
Heartbeat Interval| 100 ms| Expected interval between ECU heartbeat messages
Watchdog Check Interval| 50 ms| Interval between watchdog health checks
Failure Timeout| 300 ms| Threshold for identifying an unresponsive ECU

The watchdog calculates the elapsed time since the last heartbeat:

Elapsed Time = Current Time − Last Heartbeat Time

When the configured timeout condition is met, the supervisor identifies the corresponding ECU as unresponsive and initiates the recovery procedure.

The actual failure-detection time depends on heartbeat timing, watchdog scheduling, and the timeout comparison used in the implementation.

---

🛡️ 8. Fault Detection and Recovery Strategy

The framework can be evaluated by introducing faults into the simulated ECU processes.

A failure scenario may involve stopping heartbeat generation or terminating an ECU process.

When the watchdog detects that the heartbeat timeout has been exceeded, it identifies the affected ECU and attempts to restart that process.

The framework then checks whether heartbeat communication resumes.

The recovery strategy focuses on the following goals:

- Detect failures through heartbeat monitoring.
- Identify the affected ECU process.
- Attempt targeted process recovery.
- Verify recovery through subsequent heartbeat messages.
- Check whether unaffected ECU processes continue operating.

The effectiveness of these mechanisms must be verified using the actual simulation output and test logs.

---

🧪 9. Simulation and Implementation

The framework is developed in C using the QNX Neutrino RTOS environment.

The implementation is designed around separate ECU processes and a watchdog supervisor.

The simulation can be evaluated through the following scenarios:

Scenario| Description
1. Normal Operation| All configured ECU processes run and periodically send heartbeat messages to the watchdog.
2. ECU Failure| One ECU becomes unresponsive or stops sending heartbeats.
3. Failure Detection| The watchdog identifies the ECU whose heartbeat timeout has been exceeded.
4. Recovery Attempt| The recovery mechanism attempts to restart the affected ECU process.
5. Recovery Verification| The framework checks whether the affected ECU resumes heartbeat communication.
6. System Monitoring| QNX Momentics tools can be used to examine execution activity and timing behavior.

---

📊 10. Performance Evaluation

The framework can be evaluated using the following metrics.

Metric| Purpose
Failure Detection Time| Measures the time required to identify an unresponsive ECU
Process Restart Duration| Measures the duration of the restart operation
Total Recovery Time| Measures the time from a defined failure event to a defined recovery endpoint
Restart Success Rate| Measures how often restart attempts successfully restore the service
Heartbeat Recovery| Checks whether the restarted ECU resumes communication
Process Continuity| Checks whether unaffected ECU processes continue operating
CPU Usage| Helps evaluate resource consumption during monitoring and recovery

Performance results should be calculated from actual measurements and logs.

For meaningful comparison, the measurement boundaries should be clearly defined. For example, total recovery time may be measured from fault injection until the first valid heartbeat from the restarted ECU.

Configured timing parameters, target values, and experimentally measured results should be reported separately.

---

💻 11. Technologies Used

- C Programming Language: Implementation of the simulated ECU processes and monitoring logic.
- QNX Neutrino RTOS: Real-time operating-system environment.
- QNX Momentics IDE: Development, debugging, and analysis.
- QNX IPC: Communication between separate processes.
- QNX System Profiler: Analysis of execution activity and timing behavior.
- VMware Workstation: Virtualized development environment, if used in the current setup.

---

✨ 12. Advantages

- Enables monitoring of multiple simulated ECU processes.
- Detects missing heartbeats through timeout-based logic.
- Supports identification of the affected ECU.
- Attempts process-specific recovery.
- Provides a mechanism for checking whether heartbeat communication resumes.
- Supports fault-injection testing.
- Enables timing and execution analysis through QNX development tools.
- Provides a modular foundation for extending the framework.

---

⚠️ 13. Limitations

- The current implementation focuses on simulated ECU processes.
- A missing heartbeat does not necessarily mean that an ECU has crashed.
- Recovery success depends on process restart and initialization behavior.
- Process isolation does not guarantee that every failure is contained.
- A watchdog failure may interrupt the monitoring mechanism.
- Real-time performance claims require experimental measurements.
- The prototype is not a certified automotive safety system.

---

🚀 14. Future Scope

The framework can be extended in the following ways:

1. Hardware Integration: Extend the framework to supported physical hardware platforms.
2. Advanced Fault Injection: Test crashes, hangs, missed heartbeats, and repeated failures.
3. Enhanced Recovery: Investigate QNX High Availability Manager for additional monitoring and recovery mechanisms.
4. Resource Management: Evaluate CPU affinity, CPU sets, and real-time scheduling priorities.
5. Scalability: Add more ECU processes and diagnostic interfaces.
6. Performance Analysis: Measure detection latency, restart duration, recovery success rate, and system availability.
7. Repeated-Failure Management: Investigate restart limits, backoff strategies, and fault escalation.

---

🏁 15. Conclusion

The Automotive ECU Watchdog & Recovery Framework demonstrates an approach to monitoring and recovering simulated automotive ECU processes in a QNX-based environment.

The framework uses heartbeat messages and timeout-based logic to identify unresponsive processes. It then attempts to restart the affected process and verify whether heartbeat communication resumes.

By focusing on targeted process recovery, the project aims to reduce unnecessary disruption when an individual simulated ECU fails.

The framework provides a foundation for further development in automotive embedded systems, including hardware integration, enhanced fault recovery, and systematic performance evaluation.

---

<p align="center">👥 Team Bit-Flippers

🚗 Automotive Embedded Systems | QNX Neutrino RTOS | C Programming

</p>
