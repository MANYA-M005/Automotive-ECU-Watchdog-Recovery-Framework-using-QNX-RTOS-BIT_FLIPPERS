
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <spawn.h>
#include <process.h>
#include <sys/wait.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>

// ---------------- tunables ----------------
#define N_ECU              3
#define TICK_MS            100      // watchdog period (POSIX timer)
#define MISS_LIMIT         3        // 3 x 100 ms = 300 ms heartbeat timeout
#define OVERRUN_BUDGET_US  30000    // an ECU cycle must finish within 30 ms
#define OVERRUN_LIMIT      3        // consecutive overruns before a restart
#define SETTLE_MS          200      // decision window: collect simultaneous failures first
#define DEMO_HOLD_MS       2000     // ONLY for the audience (shows FAILED); set 0 for raw timing
#define SHOW_RECOVERED_MS  3000     // how long the RECOVERED state stays on screen
#define PULSE_HB           1
#define PULSE_TICK         2
#define PRIO_WATCHDOG      50
#define PRIO_RECOVERY      50
#define PRIO_HEALTH        30
#define PRIO_INPUT         10
#define PLAIN_CONSOLE      0
#define TL_N               8
#define TL_LEN             128

typedef enum { ST_HEALTHY, ST_OVERRUN, ST_FAILED, ST_RESTARTING, ST_RECOVERED, ST_UNAVAILABLE } State;
static const char *LABEL[] = { "[ HEALTHY ]", "[ OVERRUN ]", "[ FAILED  ]", "[RESTARTING]", "[RECOVERED]", "[ NO START ]" };
static const char *COLOR[] = { "\033[32m", "\033[33m", "\033[31m", "\033[35m", "\033[36m", "\033[31m" };

typedef struct {
    const char *name;
    int prio;                       // 1 = most critical
    const char *path;
    pid_t pid;
    int gen;
    State st;
    int beat, missed, overruns, await_first, done;
    long last_ok_ms, cycle_us, fault_ref_ms, hold_at_fail, recovered_at_ms;
    long detect_ms, restart_ms, downtime_ms;
} ECU;

static ECU ecu[N_ECU] = {
    { .name = "Brakes ECU",       .prio = 1, .path = "/tmp/pi_brakes_ecu" },
    { .name = "Engine ECU",       .prio = 2, .path = "/tmp/pi_engine_ecu" },
    { .name = "Infotainment ECU", .prio = 3, .path = "/tmp/pi_infotainment_ecu" },
};

static pthread_mutex_t lock;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static volatile int dirty = 1;
static long t_base, hold_total, hold_start = -1;
static int use_color, prio_ok = 1;
static name_attach_t *attach;
static FILE *logfile;
static char tl[TL_N][TL_LEN];
static int tl_count;

static long ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000 - t_base;
}

static long hold_now(void) { return hold_total + (hold_start >= 0 ? ms() - hold_start : 0); }

static void set_prio(int prio) {
    struct sched_param sp = { .sched_priority = prio };
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) prio_ok = 0;
}

static void logev(const char *fmt, ...) {
    char *line = tl[tl_count % TL_N];
    int n = snprintf(line, TL_LEN, "+%7.3fs  ", ms() / 1000.0);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line + n, TL_LEN - n, fmt, ap);
    va_end(ap);
    if (logfile) { fprintf(logfile, "%s\n", line); fflush(logfile); }
    tl_count++;
    dirty = 1;
}

// ---------------- Hardware Pin Mappings ----------------
// Single Status LED per ECU: Brakes=17, Engine=22, Infotainment=24
static const int LED_PIN[N_ECU] = { 17, 22, 24 };
// Single Push Button connected to GPIO 5 (defaults to targeting Brakes ECU index 0)
#define BTN_PIN 5
static int target_ecu_index = 0;

static int leds_ok = 1;
static volatile int led_busy = 0;
static int led_last[N_ECU];

static void gpio_set(int gpio, int on) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "gpio-bcm2711 set %d op %s", gpio, on ? "dh" : "dl");
    system(cmd);
}

// Helper to read push button state using gpio-bcm2711 utility
static int gpio_get(int gpio) {
    char cmd[128], buf[64];
    snprintf(cmd, sizeof(cmd), "gpio-bcm2711 get %d", gpio);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 1;
    int val = 1;
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, "0") != NULL || strstr(buf, "low") != NULL) {
            val = 0; // Pressed (pulled to ground)
        }
    }
    pclose(fp);
    return val;
}

static void leds_init(void) {
    for (int i = 0; i < N_ECU && leds_ok; i++) {
        gpio_set(LED_PIN[i], 0);
        led_last[i] = 0;
    }
    if (leds_ok) {
        pthread_mutex_lock(&lock);
        logev("Hardware ready: LEDs (17,22,24) & Single Push Button (GPIO 5)");
        pthread_mutex_unlock(&lock);
    }
}

static void update_leds(const ECU *s) {
    if (!leds_ok || led_busy) return;
    int phase = (int)((ms() / 500) & 1);
    for (int i = 0; i < N_ECU && leds_ok; i++) {
        int state_on = 0;
        switch (s[i].st) {
            case ST_HEALTHY:    state_on = 1; break;
            case ST_OVERRUN:    state_on = phase; break;
            case ST_FAILED:     state_on = 0; break;
            case ST_RESTARTING: state_on = phase; break;
            case ST_RECOVERED:  state_on = phase; break;
            default:            state_on = phase; break;
        }
        if (state_on != led_last[i]) {
            gpio_set(LED_PIN[i], state_on);
            led_last[i] = state_on;
        }
    }
}

static void leds_all_off(void) {
    if (!leds_ok) return;
    for (int i = 0; i < N_ECU; i++) { gpio_set(LED_PIN[i], 0); }
}

static void led_test(void) {
    if (!leds_ok) return;
    led_busy = 1;
    for (int i = 0; i < N_ECU && leds_ok; i++) {
        gpio_set(LED_PIN[i], 1); usleep(400000); gpio_set(LED_PIN[i], 0);
    }
    for (int i = 0; i < N_ECU; i++) led_last[i] = -1;
    led_busy = 0;
}

// ---------------- process control ----------------
static void spawn_ecu(ECU *e, int initial) {
    char g[8];
    e->gen = e->gen % 15 + 1;
    snprintf(g, sizeof g, "%d", e->gen);
    e->pid = spawnl(P_NOWAIT, e->path, e->path, g, (char *)NULL);
    e->missed = e->overruns = e->beat = 0;
    e->last_ok_ms = e->recovered_at_ms = ms();
    e->await_first = !initial;
    if (e->pid == -1) {
        e->st = ST_UNAVAILABLE;
        logev("%-16s cannot start %s: %s", e->name, e->path, strerror(errno));
    } else {
        e->st = initial ? ST_HEALTHY : ST_RECOVERED;
    }
    dirty = 1;
}

static void fail(ECU *e, const char *why) {
    e->st = ST_FAILED;
    e->done = 0;
    e->restart_ms = e->downtime_ms = 0;
    e->detect_ms = ms() - e->last_ok_ms;
    e->fault_ref_ms = e->last_ok_ms;
    e->hold_at_fail = hold_now();
    logev("%-16s FAULT DETECTED: %s (%ld ms after last good beat)", e->name, why, e->detect_ms);
    pthread_cond_signal(&cv);
}

static int next_failed(void) {
    for (int i = 0; i < N_ECU; i++) if (ecu[i].st == ST_FAILED) return i;
    return -1;
}

static void recover(ECU *e) {
    e->st = ST_RESTARTING;
    logev("%-16s RESTARTING (priority level %d)", e->name, e->prio);
    hold_start = ms();
    pthread_mutex_unlock(&lock);
    usleep(DEMO_HOLD_MS * 1000);
    pthread_mutex_lock(&lock);
    hold_total += ms() - hold_start;
    hold_start = -1;
    long t0 = ms();
    if (e->pid > 0) { kill(e->pid, SIGKILL); waitpid(e->pid, NULL, 0); }
    spawn_ecu(e, 0);
    e->restart_ms = ms() - t0;
}

// ---------------- watchdog: pulses ----------------
static void on_heartbeat(unsigned v) {
    int id = v >> 28, gen = (v >> 24) & 0xF;
    long cyc = v & 0xFFFFFF;
    if (id >= N_ECU) return;
    pthread_mutex_lock(&lock);
    ECU *e = &ecu[id];
    if (gen == e->gen && (e->st == ST_HEALTHY || e->st == ST_OVERRUN || e->st == ST_RECOVERED)) {
        e->beat = 1;
        e->cycle_us = cyc;
        if (e->await_first) {
            e->await_first = 0;
            e->done = 1;
            e->downtime_ms = ms() - e->fault_ref_ms - (hold_now() - e->hold_at_fail);
            if (e->downtime_ms < 0) e->downtime_ms = 0;
            logev("%-16s RECOVERED: restart %ld ms, downtime %ld ms (demo hold excluded)",
                  e->name, e->restart_ms, e->downtime_ms);
        }
        if (cyc > OVERRUN_BUDGET_US) {
            int k = ++e->overruns;
            logev("%-16s OVERRUN: cycle %ld ms > budget %d ms (%d/%d)",
                  e->name, cyc / 1000, OVERRUN_BUDGET_US / 1000, k, OVERRUN_LIMIT);
            if (k >= OVERRUN_LIMIT) fail(e, "repeated execution overrun");
            else e->st = ST_OVERRUN;
        } else {
            e->last_ok_ms = ms();
            if (e->overruns) {
                e->overruns = 0;
                if (e->st == ST_OVERRUN) { e->st = ST_HEALTHY; logev("%-16s overrun cleared", e->name); }
            }
        }
    }
    pthread_mutex_unlock(&lock);
}

static void on_tick(void) {
    pthread_mutex_lock(&lock);
    long now = ms();
    for (int i = 0; i < N_ECU; i++) {
        ECU *e = &ecu[i];
        if (e->st == ST_HEALTHY || e->st == ST_OVERRUN || e->st == ST_RECOVERED) {
            if (e->beat) { e->beat = 0; e->missed = 0; }
            else if (++e->missed >= MISS_LIMIT) fail(e, "missed 3 heartbeats (crash/deadlock)");
        }
        if (e->st == ST_RECOVERED && now - e->recovered_at_ms >= SHOW_RECOVERED_MS) {
            e->st = ST_HEALTHY;
            dirty = 1;
        }
    }
    pthread_mutex_unlock(&lock);
}

// ---------------- recovery manager ----------------
static void *recovery_thread(void *arg) {
    (void)arg;
    set_prio(PRIO_RECOVERY);
    pthread_mutex_lock(&lock);
    for (;;) {
        while (next_failed() < 0) pthread_cond_wait(&cv, &lock);
        pthread_mutex_unlock(&lock);
        usleep(SETTLE_MS * 1000);
        pthread_mutex_lock(&lock);
        int i;
        while ((i = next_failed()) >= 0) recover(&ecu[i]);
    }
    return NULL;
}

// ---------------- health monitor (dashboard) ----------------
static void num(char *b, int ok, long v) {
    if (ok) snprintf(b, 16, "%ld ms", v); else strcpy(b, "-");
}

static void render(const ECU *s, char t[][TL_LEN], int cnt) {
    static int first = 1;
    const char *R = use_color ? "\033[0m" : "";
    if (use_color) printf("\033[2J\033[H"); else printf("\n");
    printf("=====================================================================================\n");
    printf("         AUTOMOTIVE ECU WATCHDOG & RECOVERY SYSTEM (QNX RTOS)\n");
    printf("=====================================================================================\n");
    printf(" ECU Name          | Priority | Status       | Cycle     | Detect  | Restart | Downtime\n");
    printf("-------------------------------------------------------------------------------------\n");
    for (int i = 0; i < N_ECU; i++) {
        char d[16], r[16], w[16];
        num(d, s[i].detect_ms > 0, s[i].detect_ms);
        num(r, s[i].done, s[i].restart_ms);
        num(w, s[i].done, s[i].downtime_ms);
        printf(" %-17s | Level %d  | %s%-12s%s | %6ld us | %-7s | %-7s | %-7s\n",
               s[i].name, s[i].prio, use_color ? COLOR[s[i].st] : "", LABEL[s[i].st], R,
               s[i].cycle_us, d, r, w);
    }
    printf("=====================================================================================\n");
    printf("[FAILURE TIMELINE]  (full log: /tmp/ecu_timeline.log)\n");
    for (int k = cnt > TL_N ? cnt - TL_N : 0; k < cnt; k++) printf(" %s\n", t[k % TL_N]);
    printf("-------------------------------------------------------------------------------------\n");
    printf("[SCHEDULING] %s\n", prio_ok ? "SCHED_FIFO: watchdog 50, recovery 50, health monitor 30; ECUs 20/15/10"
                                       : "default priorities (real-time priorities need root)");
    if (use_color || first) {
        printf("[FAULT INJECTION CONTROLS]:\n");
        printf(" Press Push Button (GPIO 5) to crash target ECU [%s] (Press 'b' to cycle target)\n", ecu[target_ecu_index].name);
        printf(" Keyboard: 1-3 -> Crash Brakes/Engine/Infotainment, q -> Quit\n");
        first = 0;
    }
    printf(" Choice: ");
    fflush(stdout);
}

static void *health_thread(void *arg) {
    (void)arg;
    set_prio(PRIO_HEALTH);
    ECU s[N_ECU];
    char t[TL_N][TL_LEN];
    int cnt = 0;
    for (;;) {
        pthread_mutex_lock(&lock);
        int draw = dirty;
        memcpy(s, ecu, sizeof s);
        if (draw) { dirty = 0; memcpy(t, tl, sizeof t); cnt = tl_count; }
        pthread_mutex_unlock(&lock);
        if (draw) render(s, t, cnt);
        update_leds(s);
        usleep(100000);
    }
    return NULL;
}

// ---------------- fault injection ----------------
static void inject(int i, int sig) {
    pthread_mutex_lock(&lock);
    ECU *e = &ecu[i];
    if (e->pid > 0 && (e->st == ST_HEALTHY || e->st == ST_OVERRUN || e->st == ST_RECOVERED)) {
        kill(e->pid, sig);
        logev("%-16s FAULT INJECTED: %s", e->name,
              sig == SIGKILL ? "crash (process killed)" : sig == SIGUSR1 ? "deadlock" : "execution overrun");
    }
    pthread_mutex_unlock(&lock);
}

// ---------------- Single Push Button Polling Thread ----------------
static void *button_thread(void *arg) {
    (void)arg;
    set_prio(PRIO_INPUT);
    int last_btn_state = 1; // Default unpressed (HIGH)

    for (;;) {
        usleep(100000); // Check every 100 ms
        int current_state = gpio_get(BTN_PIN);
        // Detect falling edge (button pressed: transition from 1 to 0)
        if (last_btn_state == 1 && current_state == 0) {
            inject(target_ecu_index, SIGKILL); // Trigger crash on currently selected target ECU
            usleep(400000); // Debounce delay
        }
        last_btn_state = current_state;
    }
    return NULL;
}

static void *input_thread(void *arg) {
    (void)arg;
    set_prio(PRIO_INPUT);
    char line[64];
    while (fgets(line, sizeof line, stdin)) {
        char c = 0, d = 0;
        line[strcspn(line, "\r\n")] = 0;
        int n = sscanf(line, " %c %c", &c, &d);
        if (n < 1) continue;
        if (n == 1 && (c == 'q' || c == 'Q')) {
            pthread_mutex_lock(&lock);
            for (int i = 0; i < N_ECU; i++) if (ecu[i].pid > 0) kill(ecu[i].pid, SIGKILL);
            pthread_mutex_unlock(&lock);
            leds_all_off();
            name_detach(attach, 0);
            printf("\nShutting down Watchdog Supervisor...\n");
            exit(0);
        } else if (n == 1 && c >= '1' && c <= '3') {
            inject(c - '1', SIGKILL);
        } else if (n == 1 && (c == 'b' || c == 'B')) {
            // Cycle target ECU for the physical button: 0 -> 1 -> 2 -> 0
            pthread_mutex_lock(&lock);
            target_ecu_index = (target_ecu_index + 1) % N_ECU;
            logev("Physical button target changed to: %s", ecu[target_ecu_index].name);
            pthread_mutex_unlock(&lock);
        } else if (n == 1 && (c == 't' || c == 'T')) {
            led_test();
        } else if (n == 1 && c == '4') {
            for (int i = 0; i < N_ECU; i++) inject(i, SIGKILL);
        } else if (n == 2 && (c == 'h' || c == 'H' || c == 'o' || c == 'O') && d >= '1' && d <= '3') {
            inject(d - '1', (c == 'h' || c == 'H') ? SIGUSR1 : SIGUSR2);
        }
    }
    return NULL;
}

// ---------------- main = watchdog ----------------
int main(void) {
    t_base = 0;
    t_base = ms();
    use_color = !PLAIN_CONSOLE && isatty(STDOUT_FILENO);
    logfile = fopen("/tmp/ecu_timeline.log", "w");

    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setprotocol(&ma, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&lock, &ma);

    attach = name_attach(NULL, "ecu_watchdog", 0);
    if (!attach) { perror("name_attach"); return 1; }
    set_prio(PRIO_WATCHDOG);

    leds_init();

    pthread_mutex_lock(&lock);
    for (int i = 0; i < N_ECU; i++) spawn_ecu(&ecu[i], 1);
    logev("Supervisor started: %d ECUs, heartbeat %d ms, timeout %d ms", N_ECU, TICK_MS, TICK_MS * MISS_LIMIT);
    pthread_mutex_unlock(&lock);

    pthread_t th;
    pthread_create(&th, NULL, recovery_thread, NULL);
    pthread_create(&th, NULL, health_thread, NULL);
    pthread_create(&th, NULL, button_thread, NULL);   // Single push button thread
    pthread_create(&th, NULL, input_thread, NULL);    // Keyboard backup thread

    struct sigevent ev;
    struct itimerspec its;
    timer_t tid;
    int self = ConnectAttach(0, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);
    SIGEV_PULSE_INIT(&ev, self, SIGEV_PULSE_PRIO_INHERIT, PULSE_TICK, 0);
    timer_create(CLOCK_MONOTONIC, &ev, &tid);
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = TICK_MS * 1000000L;
    its.it_interval = its.it_value;
     timer_settime(tid, 0, &its, NULL);

    struct _pulse p;
    for (;;) {
        if (MsgReceivePulse(attach->chid, &p, sizeof p, NULL) == -1) continue;
        if (p.code == PULSE_HB) on_heartbeat((unsigned)p.value.sival_int);
        else if (p.code == PULSE_TICK) on_tick();
        else if (p.code == _PULSE_CODE_DISCONNECT) ConnectDetach(p.scoid);
    }
    return 0;
}
