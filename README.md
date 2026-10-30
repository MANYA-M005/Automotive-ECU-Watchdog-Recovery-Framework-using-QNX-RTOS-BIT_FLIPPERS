</p>---# 🚗 Automotive ECU Watchdog & Recovery Framework

### A QNX-Based Process Monitoring and Recovery Prototype

**👥 Team: Bit-Flippers**

![C](https://img.shields.io/badge/Language-C-blue?style=for-the-badge&logo=c)
![QNX](https://img.shields.io/badge/Platform-QNX-green?style=for-the-badge)
![Automotive](https://img.shields.io/badge/Domain-Automotive_Embedded_Systems-orange?style=for-the-badge)

---

📌 1. Project Overview

Modern automotive systems rely on multiple Electronic Control Units (ECUs) to perform functions such as braking, engine management and infotainment. A failure or unresponsive ECU can interrupt its associated service and affect system reliability.

The Automotive ECU Watchdog & Recovery Framework demonstrates a process-level monitoring and recovery approach using QNX Neutrino RTOS.

The framework consists of three simulated ECU processes monitored by a central watchdog supervisor. Each ECU periodically sends heartbeat messages to indicate that it is operational.

If an ECU stops communicating within the configured timeout, the supervisor detects the failure, attempts to restart the affected process and checks whether heartbeat communication resumes.

⚠️ 2. Problem Statement

An unresponsive ECU can interrupt its service. Restarting the entire system to recover an individual process may unnecessarily affect other running services.

This project explores a targeted recovery mechanism that aims to:

- Monitor multiple ECU processes.
- Detect failures using heartbeat-based monitoring.
- Identify unresponsive processes through timeout detection.
- Attempt recovery of the affected ECU.
- Verify recovery through resumed heartbeat communication.
- Minimize unnecessary disruption to unaffected processes.

🎯 3. Project Objectives

- Develop a framework to monitor multiple simulated ECU processes.
- Implement heartbeat-based monitoring using QNX inter-process communication.
- Detect ECU unresponsiveness through timeout-based logic.
- Attempt targeted recovery of affected processes.
- Verify recovery through subsequent heartbeat messages.
- Evaluate system behaviour under normal and fault-injection scenarios.
- Examine process execution and timing behaviour.

🏗️ 4. System Architecture

The framework consists of three simulated ECU processes and one central watchdog supervisor.

Block Diagram
<img width="792" height="399" alt="image" src="https://github.com/user-attachments/assets/89304ccd-e40c-406f-bc45-c6f98cbdfe9f" />

<img width="778" height="471" alt="image" src="https://github.com/user-attachments/assets/7b6a33eb-8a61-4025-8397-fcaf31f10ddd" />


System Components

Component| Function
🛑 Brakes ECU| Simulates a braking-related controller
⚙️ Engine ECU| Simulates an engine-management controller
📺 Infotainment ECU| Simulates an infotainment controller
🛡️ Watchdog Supervisor| Monitors ECU health and detects timeouts
🔄 Recovery Mechanism| Attempts to restart the affected process
🔗 QNX IPC| Enables inter-process communication

⚙️ 5. Working Principle

The framework operates through the following stages:

1. Initialization: The watchdog supervisor and ECU processes are started.
2. Heartbeat Generation: Each ECU periodically sends heartbeat messages.
3. Health Monitoring: The supervisor tracks the latest heartbeat received from each ECU.
4. Failure Detection: The supervisor checks whether the heartbeat timeout has been exceeded.
5. Targeted Recovery: The supervisor attempts to restart the affected ECU process.
6. Recovery Verification: The supervisor checks whether heartbeat communication resumes.
7. Logging: Relevant execution and recovery information is recorded where logging is implemented.

Overall Workflow:

"Initialization → Heartbeat Monitoring → Failure Detection → Recovery Attempt → Recovery Verification"

⏱️ 6. Heartbeat Monitoring & Timing Configuration

The framework uses the following configured timing parameters:

Parameter| Configured Value| Purpose
Heartbeat Interval| 100 ms| Expected interval between heartbeat messages
Watchdog Check Interval| 50 ms| Interval between health checks
Failure Timeout| 300 ms| Threshold for identifying an unresponsive ECU

The elapsed time since the last heartbeat is calculated as:

Elapsed Time = Current Time − Last Heartbeat Time

When the configured timeout condition is reached, the supervisor identifies the corresponding ECU as unresponsive.

Actual failure-detection time may vary depending on heartbeat timing, scheduling and the timeout comparison implemented.

💻 7. Implementation & Technologies

The project is implemented in C using the QNX Neutrino RTOS environment.

Technologies Used

Technology| Application
C Programming Language| ECU processes and watchdog implementation
QNX Neutrino RTOS| Real-time operating-system environment
QNX IPC| Communication between independent processes
QNX Momentics IDE| Development, debugging and testing
QNX System Profiler| Execution and thread analysis, where trace data is available

Repository Structure

Automotive-ECU-Watchdog-QNX/
│
├── Supervisor/
├── Brakes_ECU/
├── Engine_ECU/
├── Infotainment_ECU/
│
├── Results/
│
├── .gitignore
└── README.md

🧪 8. Testing & Performance Evaluation

The framework can be evaluated under different operating conditions.

Test Scenarios

Test Scenario| Expected Observation
Normal Operation| All ECU processes send heartbeat messages
ECU Failure| An ECU becomes unresponsive
Failure Detection| Supervisor identifies the unresponsive ECU
Recovery Attempt| Supervisor attempts to restart the affected process
Recovery Verification| Heartbeat communication is checked again
System Monitoring| Process and thread activity can be examined

Performance Metrics

The following metrics can be used to evaluate the framework:

- Failure Detection Time: Time required to identify an unresponsive ECU.
- Process Restart Duration: Time taken for the restart operation.
- Total Recovery Time: Time between fault injection and the defined recovery endpoint.
- Restart Success Rate: Frequency of successful process recovery.
- CPU Utilization: Resource consumption during monitoring and recovery.
- Process Continuity: Whether unaffected ECU processes continue operating.

📊 9. Results & Analysis

Detailed experimental results, screenshots, CPU utilization, thread execution timelines, process activity and fault-recovery observations are documented separately.

The results document will contain the observations and measurements obtained during testing.

📁 "View Detailed Results" (Results/RESULTS.md)

⚠️ 10. Limitations

- The current implementation focuses on simulated ECU processes.
- Missing heartbeats do not necessarily indicate a process crash.
- Recovery depends on successful process restart and initialization.
- Process isolation does not guarantee that every failure is contained.
- Performance claims require actual experimental measurements.
- The prototype is not a certified automotive safety system.

🚀 11. Future Scope

The framework can be extended through:

- Hardware Integration: Integration with supported physical hardware platforms.
- Advanced Fault Injection: Testing crashes, hangs, missed heartbeats and repeated failures.
- Enhanced Recovery: Exploring QNX High Availability Manager.
- Resource Management: Evaluating CPU affinity and real-time scheduling priorities.
- Scalability: Supporting additional ECU processes.
- Performance Analysis: Measuring detection latency, restart duration and system availability.
- Repeated-Failure Management: Implementing restart limits and fault escalation.

🏁 12. Conclusion

The Automotive ECU Watchdog & Recovery Framework demonstrates a QNX-based approach to monitoring simulated automotive ECU processes through heartbeat communication and timeout-based failure detection.

By attempting to recover only the affected process and verifying the resumption of heartbeat communication, the project explores a modular approach to improving process-level reliability in automotive embedded systems.

The framework provides a foundation for further exploration of fault detection, process recovery and real-time behaviour in automotive embedded applications.

---

<p align="center">👥 Team Bit-Flippers

🚗 Automotive Embedded Systems | QNX Neutrino RTOS | C Programming

</p>
