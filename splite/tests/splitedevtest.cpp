#include "SPLiteDevice.h"
#include <iostream>
#include <unistd.h>
#include <iomanip>

static volatile bool keepRunning = 1;

class TestCallback: public SPLiteCallaback {
public:
    virtual ~TestCallback() = default;
private:
    void touchOn(unsigned voice, float x, float y, float z) override {
        std::cout << " touchOn:" << voice;
        std::cout << " x:" << x;
        std::cout << " y:" << y;
        std::cout << " z:" << z;
        std::cout << std::endl;
    }

    void touchContinue(unsigned voice, float x, float y, float z) override {
        std::cout << " touchContinue:" << voice;
        std::cout << " x:" << x;
        std::cout << " y:" << y;
        std::cout << " z:" << z;
        std::cout << std::endl;
    }

    void touchOff(unsigned voice, float x, float y, float z) override {
        std::cout << " touchOff:" << voice;
        std::cout << " x:" << x;
        std::cout << " y:" << y;
        std::cout << " z:" << z;
        std::cout << std::endl;
    }
};

SPLiteDevice device;

void intHandler(int dummy) {
    std::cerr << "TouchTrackerTest intHandler called" << std::endl;
    if(!keepRunning) {
        sleep(1);
        exit(-1);
    }
    keepRunning = false;
    device.stop();
}

int main(int argc, const char * argv[]) {
    signal(SIGINT, intHandler);

    device.addCallback(std::make_shared<TestCallback>());

    device.start();
    device.maxTouches(4);

    while(keepRunning) {
        sleep(1);
    }
    device.stop();
    return 0;
}
