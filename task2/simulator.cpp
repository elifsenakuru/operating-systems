#include <iostream>
#include <vector>
#include <pthread.h>
#include "Car.h"
#include "WriteOutput.h"
#include "NarrowBridge.h"
#include "Ferry.h"
#include "Crossroad.h"

using namespace std;

extern vector<Crossroad> crossroads;
extern vector<Ferry> ferries;
extern vector<NarrowBridge> narrowBridges;

void* threadStart(void* s) {
    Car* car = static_cast<Car*>(s);
    car->goToConnector();
    delete car;    
    return nullptr;
}

int main() {
    InitWriteOutput();

    int numOfCrossroads, numOfFerries, numOfBridges;

    cin >> numOfBridges;
    for (int i = 0; i < numOfBridges; ++i) {
        int travelTime, maxWaitTime;
        cin >> travelTime >> maxWaitTime;
        narrowBridges.emplace_back(i, travelTime, maxWaitTime);
    }

    cin >> numOfFerries;
    for (int i = 0; i < numOfFerries; ++i) {
        int travelTime, maxWaitTime, capacity;
        cin >> travelTime >> maxWaitTime >> capacity;
        ferries.emplace_back(i, travelTime, maxWaitTime, capacity);
    }

    cin >> numOfCrossroads;
    for (int i = 0; i < numOfCrossroads; ++i) {
        int travelTime, maxWaitTime;
        cin >> travelTime >> maxWaitTime;
        crossroads.emplace_back(i, travelTime, maxWaitTime);
    }

    int numOfCars;
    cin >> numOfCars;
    vector<pthread_t> carThreads(numOfCars);

    for (int i = 0; i < numOfCars; ++i) {
        int travelTime, pathLength;
        cin >> travelTime >> pathLength;
        vector<Path> path;

        for(int j = 0; j < pathLength; j++) {
            string typeID;
            int from, to;
            cin >> typeID >> from >> to;
            char type = typeID[0];
            int id = stoi(typeID.substr(1));
            path.emplace_back(type, id, from, to);
        }

        Car* car = new Car(i, travelTime, path);

        if (pthread_create(&carThreads[i], nullptr, threadStart, car) != 0) {
            cerr << "Error" << i << endl;
            delete car;
        }
    }

    for (auto& thread : carThreads) {
        pthread_join(thread, nullptr);
    }

    return 0;
}
