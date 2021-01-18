typedef void (*timer_callback)(void);
 
class SimpleTimer {
 
public:
    // maximum number of timers
    const static int MAX_TIMERS = 10;
 
    // setTimer() constants
    const static int RUN_FOREVER = 0;
    const static int RUN_ONCE = 1;
 
    // constructor
    SimpleTimer();
 
    // this function must be called inside loop()
    void run();
 
    // call function f every d milliseconds
    int setInterval(long d, timer_callback f);
 
    // call function f once after d milliseconds
    int setTimeout(long d, timer_callback f);
 
    // call function f every d milliseconds for n times
    int setTimer(long d, timer_callback f, int n);
 
    // destroy the specified timer
    void deleteTimer(int numTimer);
 
    // restart the specified timer
    void restartTimer(int numTimer);
 
    // returns true if the specified timer is enabled
    boolean isEnabled(int numTimer);
 
    // enables the specified timer
    void enable(int numTimer);
 
    // disables the specified timer
    void disable(int numTimer);
 
    // enables the specified timer if it's currently disabled,
    // and vice-versa
    void toggle(int numTimer);
 
    // returns the number of used timers
    int getNumTimers();
 
    // returns the number of available timers
    int getNumAvailableTimers() { return MAX_TIMERS - numTimers; };
 
private:
    // deferred call constants
    const static int DEFCALL_DONTRUN = 0;       // don't call the callback function
    const static int DEFCALL_RUNONLY = 1;       // call the callback function but don't delete the timer
    const static int DEFCALL_RUNANDDEL = 2;      // call the callback function and delete the timer
 
    // find the first available slot
    int findFirstFreeSlot();
 
    // value returned by the millis() function
    // in the previous run() call
    unsigned long prev_millis[MAX_TIMERS];
 
    // pointers to the callback functions
    timer_callback callbacks[MAX_TIMERS];
 
    // delay values
    long delays[MAX_TIMERS];
 
    // number of runs to be executed for each timer
    int maxNumRuns[MAX_TIMERS];
 
    // number of executed runs for each timer
    int numRuns[MAX_TIMERS];
 
    // which timers are enabled
    boolean enabled[MAX_TIMERS];
 
    // deferred function call (sort of) - N.B.: this array is only used in run()
    int toBeCalled[MAX_TIMERS];
 
    // actual number of timers in use
    int numTimers;
};

// Select time function:
//static inline unsigned long elapsed() { return micros(); }
static inline unsigned long elapsed() { return millis(); }
 
 
SimpleTimer::SimpleTimer() {
    unsigned long current_millis = elapsed();
 
    for (int i = 0; i < MAX_TIMERS; i++) {
        enabled[i] = false;
        callbacks[i] = 0;                   // if the callback pointer is zero, the slot is free, i.e. doesn't "contain" any timer
        prev_millis[i] = current_millis;
        numRuns[i] = 0;
    }
 
    numTimers = 0;
}
 
 
void SimpleTimer::run() {
    int i;
    unsigned long current_millis;
 
    // get current time
    current_millis = elapsed();
 
    for (i = 0; i < MAX_TIMERS; i++) {
 
        toBeCalled[i] = DEFCALL_DONTRUN;
 
        // no callback == no timer, i.e. jump over empty slots
        if (callbacks[i]) {
 
            // is it time to process this timer ?
            // see https://arduino.cc/forum/index.php/topic,124048.msg932592.html#msg932592
 
            if (current_millis - prev_millis[i] >= delays[i]) {
 
                // update time
                //prev_millis[i] = current_millis;
                prev_millis[i] += delays[i];
 
                // check if the timer callback has to be executed
                if (enabled[i]) {
 
                    // "run forever" timers must always be executed
                    if (maxNumRuns[i] == RUN_FOREVER) {
                        toBeCalled[i] = DEFCALL_RUNONLY;
                    }
                    // other timers get executed the specified number of times
                    else if (numRuns[i] < maxNumRuns[i]) {
                        toBeCalled[i] = DEFCALL_RUNONLY;
                        numRuns[i]++;
 
                        // after the last run, delete the timer
                        if (numRuns[i] >= maxNumRuns[i]) {
                            toBeCalled[i] = DEFCALL_RUNANDDEL;
                        }
                    }
                }
            }
        }
    }
 
    for (i = 0; i < MAX_TIMERS; i++) {
        switch(toBeCalled[i]) {
            case DEFCALL_DONTRUN:
                break;
 
            case DEFCALL_RUNONLY:
                (*callbacks[i])();
                break;
 
            case DEFCALL_RUNANDDEL:
                (*callbacks[i])();
                deleteTimer(i);
                break;
        }
    }
}
 
 
// find the first available slot
// return -1 if none found
int SimpleTimer::findFirstFreeSlot() {
    int i;
 
    // all slots are used
    if (numTimers >= MAX_TIMERS) {
        return -1;
    }
 
    // return the first slot with no callback (i.e. free)
    for (i = 0; i < MAX_TIMERS; i++) {
        if (callbacks[i] == 0) {
            return i;
        }
    }
 
    // no free slots found
    return -1;
}
 
 
int SimpleTimer::setTimer(long d, timer_callback f, int n) {
    int freeTimer;
 
    freeTimer = findFirstFreeSlot();
    if (freeTimer < 0) {
        return -1;
    }
 
    if (f == NULL) {
        return -1;
    }
 
    delays[freeTimer] = d;
    callbacks[freeTimer] = f;
    maxNumRuns[freeTimer] = n;
    enabled[freeTimer] = true;
    prev_millis[freeTimer] = elapsed();
 
    numTimers++;
 
    return freeTimer;
}
 
 
int SimpleTimer::setInterval(long d, timer_callback f) {
    return setTimer(d, f, RUN_FOREVER);
}
 
 
int SimpleTimer::setTimeout(long d, timer_callback f) {
    return setTimer(d, f, RUN_ONCE);
}
 
 
void SimpleTimer::deleteTimer(int timerId) {
    if (timerId >= MAX_TIMERS) {
        return;
    }
 
    // nothing to delete if no timers are in use
    if (numTimers == 0) {
        return;
    }
 
    // don't decrease the number of timers if the
    // specified slot is already empty
    if (callbacks[timerId] != NULL) {
        callbacks[timerId] = 0;
        enabled[timerId] = false;
        toBeCalled[timerId] = DEFCALL_DONTRUN;
        delays[timerId] = 0;
        numRuns[timerId] = 0;
 
        // update number of timers
        numTimers--;
    }
}
 
 
// function contributed by code@rowansimms.com
void SimpleTimer::restartTimer(int numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }
 
    prev_millis[numTimer] = elapsed();
}
 
 
boolean SimpleTimer::isEnabled(int numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return false;
    }
 
    return enabled[numTimer];
}
 
 
void SimpleTimer::enable(int numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }
 
    enabled[numTimer] = true;
}
 
 
void SimpleTimer::disable(int numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }
 
    enabled[numTimer] = false;
}
 
 
void SimpleTimer::toggle(int numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }
 
    enabled[numTimer] = !enabled[numTimer];
}
 
 
int SimpleTimer::getNumTimers() {
    return numTimers;
}

////////////////////////////////////////////////////

int led_red = 7;		// Red LED: Pin 0
int led_yellow = 6; 	// Yellow LED: Pin 1
int led_green = 5; 		// Green LED: Pin 2

volatile int state_red = LOW;
volatile int state_green = LOW;
SimpleTimer timer1;
SimpleTimer timer2;
SimpleTimer timer3;

void setup() {
    pinMode(led_red, OUTPUT);
    pinMode(led_yellow, OUTPUT);
    pinMode(led_green, OUTPUT);
    timer1.setInterval(1000, toggle_red);
    timer2.setTimeout(5000, turn_on);
    timer3.setTimer(1000, toggle_green, 5);
}

void loop() {
    timer1.run();
    timer2.run();
    timer3.run();
}

void toggle_red() {
    state_red = !state_red;
    digitalWrite(led_red, state_red);
}

void turn_on() {
    digitalWrite(led_yellow, HIGH);
}

void toggle_green() {
    state_green = !state_green;
    digitalWrite(led_green, state_green);
}
