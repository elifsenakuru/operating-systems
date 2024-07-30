#ifndef FERRY_H
#define FERRY_H

#include "monitor.h"
#include "WriteOutput.h"
#include <vector>
#include <map>
#include <ctime>
#include "helper.h"

class Ferry : public Monitor {
public:
    Ferry(int id, int travelTime, int maxWaitTime, int capacity);
    ~Ferry();
    void Pass(int carID, int direction);
    void depart( int direction);
    static Ferry* getByID(int id);
    static std::map<int, Ferry*> ferries;

private:
    int id;
    int travelTime;
    int maxWaitTime;
    int capacity;
    std::vector<int> cars[2];
    int carsLoaded[2];
    bool isDeparting[2];
    Condition condition[2];
    bool canPass[2];
    int totalCars[2];
    timespec *time[2];

    void enqueueCar(int carID, int direction);
};

#endif // FERRY_H
