
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>

#define ECU_NAME "Infotainment ECU"
#define ECU_ID   2
#define ECU_PRIO 10
// ===========================================================

#define PERIOD_US   100000   // one cycle + one heartbeat every 100 ms
#define OVERRUN_US  150000   // injected overrun: a cycle takes 150 ms
#define PULSE_HB    1        // pulse code used for heartbeats

static volatile sig_atomic_t deadlock_req = 0;
static volatile sig_atomic_t overrun_cycles = 0;

static void on_usr1(int s) { (void)s; deadlock_req = 1; }    // supervisor: "h" key
static void on_usr2(int s) { (void)s; overrun_cycles = 3; }  // supervisor: "o" key

static long now_us(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000L + t.tv_nsec / 1000;
}

int main(int argc, char **argv) {
    // argv[1] = instance number given by the supervisor (0 when started by hand)
    int gen = (argc > 1) ? atoi(argv[1]) : 0;

    // Priority by criticality: Brakes > Engine > Infotainment
    struct sched_param sp = { .sched_priority = ECU_PRIO };
    int prio_ok = (pthread_setschedparam(pthread_self(), SCHED_RR, &sp) == 0);

    signal(SIGUSR1, on_usr1);
    signal(SIGUSR2, on_usr2);

    // A NORMAL mutex locked twice by the same thread = a real self-deadlock
    pthread_mutex_t m;
    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&m, &ma);

    printf("[%s] Started successfully (PID: %d, priority %d%s)\n",
           ECU_NAME, getpid(), ECU_PRIO, prio_ok ? "" : " not applied");
    fflush(stdout);

    int coid = -1;
    long next = now_us();

    while (1) {
        long t0 = now_us();

        // ---------- ECU work (braking logic / telemetry / media) ----------
        if (overrun_cycles > 0) {                 // injected execution overrun
            overrun_cycles--;
            while (now_us() - t0 < OVERRUN_US) { }
        }
        if (deadlock_req) {                       // injected deadlock
            printf("[%s] DEADLOCK injected - heartbeat will stop\n", ECU_NAME);
            fflush(stdout);
            pthread_mutex_lock(&m);
            pthread_mutex_lock(&m);               // blocks forever
        }
        long cycle = now_us() - t0;               // execution time of this cycle

        // ---------- heartbeat: QNX pulse to the watchdog ----------
        if (coid < 0) coid = name_open("ecu_watchdog", 0);
        if (coid >= 0) {
            // value layout: [ECU id:4][instance:4][cycle time in us:24]
            unsigned v = ((unsigned)ECU_ID << 28) | ((unsigned)(gen & 0xF) << 24) |
                         (unsigned)(cycle > 0xFFFFFF ? 0xFFFFFF : cycle);
            if (MsgSendPulse(coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_HB, (int)v) == -1) {
                name_close(coid);
                coid = -1;
                if (gen > 0) {                    // started by a supervisor that is gone
                    printf("[%s] supervisor gone - exiting\n", ECU_NAME);
                    fflush(stdout);
                    exit(0);
                }
            }
        } else if (gen > 0) {
            exit(0);
        }

        // ---------- wait for the next period (absolute timer, no drift) ----------
        next += PERIOD_US;
        long now = now_us();
        if (next < now) next = now;               // after an overrun: skip missed periods
        struct timespec ts = { .tv_sec = next / 1000000, .tv_nsec = (next % 1000000) * 1000 };
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
    }
    return 0;
}



