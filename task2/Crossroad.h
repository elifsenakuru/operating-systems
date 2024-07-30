#ifndef CROSSROAD_H
#define CROSSROAD_H

#include <pthread.h>
#include <queue>
#include <vector>
#include "WriteOutput.h"
#include "helper.h"
#include "monitor.h"


class Crossroad : public Monitor {
public:
    Crossroad(int id, int travelTime, int maxWaitTime);
    ~Crossroad();
    void Pass(int carID, int from, int to);
    static Crossroad* getByID(int id);

private:
    int id;
    int travelTime;
    int maxWaitTime;
    int flowDirection; // current active road (0-3)
    int carsPassing;
    int carBeforeExists;
    std::vector<std::queue<int>> queues; // queues for each direction
    Condition conditions[4]; // one condition variable per direction
    static std::map<int, Crossroad*> crossroads;
};

std::map<int, Crossroad*> Crossroad::crossroads;

Crossroad::Crossroad(int id, int travelTime, int maxWaitTime)
    : Monitor(), id(id), travelTime(travelTime), maxWaitTime(maxWaitTime),
      flowDirection(0), carsPassing(0), carBeforeExists(-1), conditions{Condition(this), Condition(this), Condition(this), Condition(this)}{
    for (int i = 0; i < 4; i++) {
        queues.push_back(std::queue<int>());
        
    }
    crossroads[id] = this;
}

Crossroad::~Crossroad() {
    crossroads.erase(id);
}

Crossroad* Crossroad::getByID(int id) {
    auto it = crossroads.find(id);
    return it != crossroads.end() ? it->second : nullptr;
}

void Crossroad::Pass(int carID, int from, int to) {
    __synchronized__;
    //check carDirection
    int carDirection = from;

    //queue push
    queues[carDirection].push(carID);

    if(carBeforeExists == -1){
        //carBeforeExists = carID;
        //carDirection belirle
        flowDirection = carDirection;
    }

    //printf("flowDirection: %d carBeforeExists: %d\n", flowDirection, carBeforeExists);

    while (true){
        //if carDirection aynıysa
        if (flowDirection == carDirection) {
            if (queues[flowDirection].front() == carID) {
                // if the car is not in the front pass delay
                if(carBeforeExists != -1){
                    mutex.unlock();
                    sleep_milli(PASS_DELAY);
                    mutex.lock();
                } 

                if (carDirection != flowDirection) {
                    continue;
                }

                carBeforeExists = carID;

                queues[carDirection].pop();
                conditions[carDirection].notifyAll();
                WriteOutput(carID, 'C', id, START_PASSING);
                carsPassing++;
                mutex.unlock();
                sleep_milli(travelTime);
                mutex.lock();
                WriteOutput(carID, 'C', id, FINISH_PASSING);
                carsPassing--;

                
               if (carsPassing == 0 && queues[flowDirection].empty()) {

                    int nextRoad = (flowDirection + 1) % 4;
                    //printf("Next road: %d\n", nextRoad);
                    int checkedRoads = 0; // Counter to ensure we do not loop indefinitely

                    // Cycle through all roads to find the next one with cars waiting
                    while (queues[nextRoad].empty() && checkedRoads < 4) {
                        nextRoad = (nextRoad + 1) % 4;
                        checkedRoads++;
                    }

                    if (checkedRoads < 4) {
                        flowDirection = nextRoad;
                        carBeforeExists = -1;
                        conditions[flowDirection].notifyAll(); // Notify the next road
                    }
                }

                break;
            } else {
                conditions[flowDirection].wait(); // Wait if not the front car
            }
        } else {
            conditions[1-flowDirection].wait(); // Wait if not the active road
        }
    }
}

#endif 