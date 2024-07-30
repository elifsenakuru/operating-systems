#ifndef NARROW_BRIDGE_H
#define NARROW_BRIDGE_H

#include <pthread.h>
#include <queue>
#include <map>
#include "WriteOutput.h"
#include "helper.h"
#include "monitor.h"
#include <WriteOutput.h>
#include <sys/time.h>
#include <cerrno>



class NarrowBridge : public Monitor {
public:
    NarrowBridge(int id, int travelTime, int maxWaitTime);
    ~NarrowBridge();
    void Pass(int carID, int from, int to);
    static NarrowBridge* getByID(int id);

private:
    int id;
    int travelTime;
    int maxWaitTime;
    int flowDirection; 
    int carsPassing;
    Condition condition[2]; 
    std::queue<int> linequeue1;
    std::queue<int> linequeue0;
    static std::map<int, NarrowBridge*> bridges;
    struct timespec ts;
    int carBeforeExists;
};

std::map<int, NarrowBridge*> NarrowBridge::bridges;

NarrowBridge::NarrowBridge(int id, int travelTime, int maxWaitTime)
    : Monitor(), id(id), travelTime(travelTime), maxWaitTime(maxWaitTime),
      flowDirection(-1), carsPassing(0),
      condition{Condition(this), Condition(this)}, carBeforeExists(-1)
       {
    bridges[id] = this;
    
}

NarrowBridge::~NarrowBridge() {
    bridges.erase(id);
}

NarrowBridge* NarrowBridge::getByID(int id) {
    auto it = bridges.find(id);
    return it != bridges.end() ? it->second : nullptr;
}

void NarrowBridge::Pass(int carID, int from, int to) {
    __synchronized__;
    //check carDirection
    int carDirection = from;

    //queue push
    if(carDirection == 0){
        linequeue0.push(carID);
    }else{
        linequeue1.push(carID);
    }

    if(carBeforeExists == -1){
        //carBeforeExists = carID;
        //carDirection belirle
        flowDirection = carDirection;
    }
    //printf("flowDirection: %d carBeforeExists: %d\n", flowDirection, carBeforeExists);

    while (true){
        //if carDirection aynıysa
        if (flowDirection == carDirection){

            // if car queueda öndeyse
            //printf("carID: %d, linequeue0.front(): %d\n", carID, linequeue0.front());
            if (carDirection == 0 && linequeue0.front() == carID){ 
                 //check carBeforeExists
                if(carBeforeExists != -1){
                    mutex.unlock();
                    sleep_milli(PASS_DELAY);
                    mutex.lock();
                    //pass delay 
                }

                if(carDirection != flowDirection){
                    continue;
                }
                carBeforeExists = carID;
                // işlemleri yap start passing finish passing starttan önce queue pop
                linequeue0.pop();
                condition[0].notifyAll();
                WriteOutput(carID, 'N', id, START_PASSING);
                carsPassing++;
                mutex.unlock();
                sleep_milli(travelTime);
                mutex.lock();
                carsPassing--;
                WriteOutput(carID, 'N', id, FINISH_PASSING);
                if(carsPassing == 0 && linequeue0.empty() && !linequeue1.empty()){
                    flowDirection = 1;
                    carBeforeExists = -1;
                    condition[1].notifyAll();
                }
                break;
            }
            else if (carDirection == 1 && linequeue1.front() == carID){
                //check carBeforeExists
                if(carBeforeExists != -1){
                    mutex.unlock();
                    sleep_milli(PASS_DELAY);
                    mutex.lock();
                    //pass delay 
                }
                if(carDirection != flowDirection){
                    continue;
                }
                // işlemleri yap start passing finish passing starttan önce carDirectionqueue pop
                linequeue1.pop();
                carBeforeExists = carID;
                condition[1].notifyAll();
                WriteOutput(carID, 'N', id, START_PASSING);
                carsPassing++;
                mutex.unlock();
                sleep_milli(travelTime);
                mutex.lock();
                WriteOutput(carID, 'N', id, FINISH_PASSING);
                carsPassing--;
                if(carsPassing == 0 && !linequeue0.empty() && linequeue1.empty()){
                    flowDirection = 0;
                    carBeforeExists = -1;
                    condition[0].notifyAll();
                }
                break;
                
            } 
            // else wait
            else {
                //wait
                condition[flowDirection].wait();
            }
        }

        //else if current carDirectiondaki queue boş öbürü boş değilse
        /*else if (flowDirection == 0 && linequeue0.empty() && !linequeue1.empty()){
            flowDirection = 1;
            // BEFORE ANY CAR   - 1
            carBeforeExists = -1;
            // notify
            condition[1].notifyAll();
        }else if (flowDirection == 1 && linequeue1.empty() && !linequeue0.empty()){
            flowDirection = 0;
            // BEFORE ANY CAR   - 1
            carBeforeExists = -1;
            // notify
            condition[0].notifyAll();
        }*/

        //else
        else {
            // wait
            condition[1 - flowDirection].wait();
        }

    }
    
}




#endif // NARROW_BRIDGE_H