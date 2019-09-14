#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <signal.h>

#include <iomanip>
#include <string.h>

#include <SoundplaneDriver.h>
#include "SoundplaneModelA.h"

namespace {

class HelloSoundplaneDriverListener : public SoundplaneDriverListener {
public:
    HelloSoundplaneDriverListener() {
    }

    ~HelloSoundplaneDriverListener() = default;

    void onStartup(void) override {
        ;
    }

    void onFrame(const SensorFrame &frame) override {
        static unsigned count;
        if ((count++ % 1000) == 0) {
            dumpFrameStats(std::cout, frame);
            dumpFrameAsASCII(std::cout,frame);
//            dumpFrame(std::cout, frame);
        }
    }

    void onError(int err, const char *errStr) override {
        switch (err) {
            case kDevDataDiffTooLarge:
                std::cerr << "error: frame difference too large: " << errStr << "\n";
//                beginCalibrate();
                break;
            case kDevGapInSequence:
//                    std::cerr << "note: gap in sequence " << errStr << "\n";
                break;
            case kDevReset:
                std::cerr << "isoch stalled, resetting " << errStr << "\n";
                break;
            case kDevPayloadFailed:
                std::cerr << "payload failed at sequence " << errStr << "\n";
                break;
        }
    }

    void onClose(void) override {
        ;
    }
}; // listener class

} // namespace

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    std::cerr  << "int handler called";
    if(keepRunning==0) {
        sleep(1);
        exit(-1);
    }
    keepRunning = 0;
}

const int kModelDefaultCarriersSize = 40;
const unsigned char kModelDefaultCarriers[kModelDefaultCarriersSize] =
    {
        // 40 default carriers.  avoiding 32 (gets aliasing from 16)
        3, 4, 5, 6, 7,
        8, 9, 10, 11, 12,
        13, 14, 15, 16, 17,
        18, 19, 20, 21, 22,
        23, 24, 25, 26, 27,
        28, 29, 30, 31, 33,
        34, 35, 36, 37, 38,
        39, 40, 41, 42, 43
    };

static const int kStandardCarrierSets = 16;
static void makeStandardCarrierSet(SoundplaneDriver::Carriers &carriers, int set)
{
    int startOffset = 2;
    int skipSize = 2;
    int gapSize = 4;
    int gapStart = set*skipSize + startOffset;
    carriers[0] = carriers[1] = 0;
    for(int i=startOffset; i<gapStart && i<kSoundplaneNumCarriers; ++i)
    {
        carriers[i] = kModelDefaultCarriers[i];
    }
    for(int i=gapStart; i<kSoundplaneNumCarriers; ++i)
    {
        carriers[i] = kModelDefaultCarriers[i + gapSize];
    }
}


int main(int argc, const char * argv[])
{
    signal(SIGINT, intHandler);
    HelloSoundplaneDriverListener listener;
    auto driver = SoundplaneDriver::create(listener);

    std::cout << "Hello, Soundplane?\n";
    driver->start();
    while( driver->getDeviceState() != kDeviceHasIsochSync) {
        sleep(1);
        std::cout << "device state: " << driver->getDeviceState() << std::endl;
    }

    SoundplaneDriver::Carriers c;
    makeStandardCarrierSet(c,kStandardCarrierSets);
    driver->setCarriers(c);
    unsigned long carrierMask = 0xFFFFFFFF;
    driver->enableCarriers(~carrierMask);


    while(keepRunning)
    {
        sleep(1);
    }
    delete driver.release();


    return 0;
}
