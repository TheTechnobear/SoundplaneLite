#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <signal.h>

#include <SoundplaneDriver.h>
#include "SoundplaneModelA.h"
#include "TouchTracker.h"

static volatile bool keepRunning = 1;

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
    for(int i=startOffset; i<gapStart; ++i)
    {
        carriers[i] = kModelDefaultCarriers[i];
    }
    for(int i=gapStart; i<kSoundplaneNumCarriers; ++i)
    {
        carriers[i] = kModelDefaultCarriers[i + gapSize];
    }
}



namespace {

const int kMaxTouch = 8;
class TouchTrackerTest : public SoundplaneDriverListener {
public:
    TouchTrackerTest() : driver_(nullptr) {
        driver_ = SoundplaneDriver::create(*this);
    }

    ~TouchTrackerTest() = default;
    void start() {
        keepRunning = true;
        driver_->start();
    }

    void stop() {
        keepRunning = false;
        delete driver_.release();
        driver_ = nullptr;
    }

    void onStartup(void) override {
        isCarrierSet_ = false;
        isCal_ = false;

        tracker_.setRotate(false);
        tracker_.setThresh( 0.05f);
        tracker_.setLopassZ(100.0f);
    }


    void onFrame(const SensorFrame &frame) override {
        if(!driver_) return;
        auto state = driver_->getDeviceState();

        if(state == kDeviceHasIsochSync) {
            if (!isCarrierSet_) {
                makeStandardCarrierSet(carriers_, kStandardCarrierSets);
                driver_->setCarriers(carriers_);
                unsigned long carrierMask = 0xFFFFFFFF;
                driver_->enableCarriers(~carrierMask);
                isCarrierSet_ = true;

                // start calibration
                stats_.clear();
                isCal_ =false;
                tracker_.clear();
            } else if (!isCal_) {
                stats_.accumulate(frame);
                if (stats_.getCount() >= kSoundplaneCalibrateSize) {
                    SensorFrame mean = clamp(stats_.mean(), 0.0001f, 1.f);
                    calibrateMeanInv_ = divide(fill(1.f), mean);
                    isCal_ = true;
                }
            } else {
                calibratedFrame_ = subtract(multiply(frame, calibrateMeanInv_), 1.0f);
                SensorFrame curvature = tracker_.preprocess(calibratedFrame_);
                TouchArray t = tracker_.process(curvature, maxTouches_);
                outputTouches(t);
            }
        }
    }

    void onError(int err, const char *errStr) override {
        switch (err) {
            case kDevDataDiffTooLarge:
                std::cerr << "error: frame difference too large: " << errStr << std::endl;
                stats_.clear();
                isCal_ =false;
                break;
            case kDevGapInSequence:
                std::cerr << "note: gap in sequence " << errStr << std::endl;
                break;
            case kDevReset:
                std::cerr << "isoch stalled, resetting " << errStr << std::endl;
                break;
            case kDevPayloadFailed:
                std::cerr << "payload failed at sequence " << errStr << std::endl;
                break;
        }
    }

    void onClose(void) override {
        keepRunning = false;
    }

    void outputTouches(TouchArray& t) {
        std::cout << std::fixed << std::setw(6) << std::setprecision(4);
        for(int i=0; i<kMaxTouch; ++i)
        {
            if(t[i].state != kTouchStateInactive) {
                std::cout << " i:" << i;
                std::cout << " x:" << t[i].x;
                std::cout << " y:" << t[i].y;
                std::cout << " z:" << t[i].z;
//                std::cout << " n:" << t[i].note;
                std::cout << std::endl;
            }
        }
    }


private:
    bool isCal_ = false;
    bool isCarrierSet_ = false;

    std::unique_ptr<SoundplaneDriver> driver_ =nullptr;
    SoundplaneDriver::Carriers carriers_;

    SensorFrameStats stats_;
    SensorFrame calibrateMeanInv_{};
    SensorFrame calibratedFrame_{};
    unsigned maxTouches_ = kMaxTouches;
    TouchTracker tracker_;

};

} // namespace

void intHandler(int dummy) {
    std::cerr << "TouchTrackerTest intHandler called" << std::endl;
    if(!keepRunning) {
        sleep(1);
        exit(-1);
    }
    keepRunning = false;
}

int main(int argc, const char * argv[]) {
    signal(SIGINT, intHandler);

    TouchTrackerTest test;
    test.start();
    while(keepRunning) {
        sleep(1);
    }
    test.stop();
    return 0;
}
