#ifndef CAR_H
#define CAR_H

#include <pthread.h>
#include <vector>
#include <iostream>
#include <string>
#include "WriteOutput.h"  
#include "helper.h"
#include "Ferry.h"
#include "NarrowBridge.h"
#include "Crossroad.h"

using namespace std;

struct Path {
    char connector;
    int id;
    int from;
    int to;

    Path(char c, int i, int f, int to) : connector(c), id(i), from(f), to(to) {}
};

vector<Crossroad> crossroads;
vector<Ferry> ferries;
vector<NarrowBridge> narrowBridges;

class Car {
    public:
        int carID;
        int travelTime;
        vector<Path> path;
        // Constructor declaration
        Car(int id, int t, const vector<Path>& p);

        // Method declaration
        void goToConnector();
};

Car::Car(int id, int t, const vector<Path>& p) : carID(id), travelTime(t), path(p) {}

void Car::goToConnector() {
    for (const auto& p : path) {
        WriteOutput(carID, p.connector, p.id, TRAVEL);
        sleep_milli(travelTime);
        WriteOutput(carID, p.connector, p.id, ARRIVE);

        switch (p.connector) {
            case 'C': 
                crossroads[p.id].Pass(carID, p.from, p.to);
                break;
            case 'F': 
                ferries[p.id].Pass(carID, p.from);
                break;
            case 'N': 
                narrowBridges[p.id].Pass(carID, p.from, p.to);
                break;
            default:
                cerr << "unknown connector: " << p.connector << endl;
        }
    }
}


#endif 
