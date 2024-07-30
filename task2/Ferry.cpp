#include "Ferry.h"
#include <ctime>
#include <cerrno>
#include "helper.h"

using namespace std;

map<int, Ferry*> Ferry::ferries;

Ferry::Ferry(int id, int travelTime, int maxWaitTime, int capacity)
    : id(id), travelTime(travelTime), maxWaitTime(maxWaitTime), capacity(capacity),
      carsLoaded{0, 0},isDeparting{false, false}, condition{Condition(this), Condition(this)}, canPass{false, false}, totalCars{0, 0 },
       time{new timespec(), new timespec()}{

    ferries[id] = this;
}

Ferry::~Ferry() {}

Ferry* Ferry::getByID(int id) {
    auto it = ferries.find(id);
    return it != ferries.end() ? it->second : nullptr;
}

void Ferry::Pass(int carID, int direction) {
    __synchronized__;

    // wait while ferry is in the process of departing
    while (isDeparting[direction]) {
        condition[direction].wait();
    }

    // check again if departure started to prevent race conditions
    if (isDeparting[direction]) {
        condition[direction].wait();
    } else {
        carsLoaded[direction]++;

        if (carsLoaded[direction] == capacity) {
            //printf("capacity %d \n", capacity);
            //printf("Ferry is full, departing\n");  
            isDeparting[direction] = true;  // Ensure no more cars load
            canPass[direction] = true;
            totalCars[direction] = carsLoaded[direction];
            condition[direction].notifyAll(); // Notify all cars they can start passing
        } else {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += maxWaitTime / 1000;
            ts.tv_nsec += (maxWaitTime % 1000) * 1000000LL;
            while (!canPass[direction]) {
                if (condition[direction].timedwait(&ts) == ETIMEDOUT) {
                    //printf("Timed out waiting for more cars, ferry departing\n");
                    isDeparting[direction] = true;
                    canPass[direction] = true;
                    totalCars[direction] = carsLoaded[direction];
                    condition[direction].notifyAll();
                    break;
                }
            }
        }

        // wait for the signal to pass if not already passing
        while (!canPass[direction]) {
            condition[direction].wait();
        }

        WriteOutput(carID, 'F', id, START_PASSING);
        mutex.unlock();  
        sleep_milli(travelTime);
        mutex.lock();
        WriteOutput(carID, 'F', id, FINISH_PASSING);

        totalCars[direction]--;
        if (totalCars[direction] == 0) {
            canPass[direction] = false;
            isDeparting[direction] = false; //allow loading for next coming cars
            carsLoaded[direction] = 0;  // reset 
            condition[direction].notifyAll(); // notify to wake up waiting cars
        }
    }
}


