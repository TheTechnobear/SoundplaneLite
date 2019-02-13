#include "SPLiteDevice.h"

#include <iostream>
#include <unistd.h>
#include <iomanip>

#include <strstream>

#include <SoundplaneDriver.h>
#include "SoundplaneModelA.h"
#include "TouchTracker.h"


const int kModelDefaultCarriersSize = 40;
static const int kStandardCarrierSets = 16;
static void makeStandardCarrierSet(SoundplaneDriver::Carriers &carriers, int set);

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


static void makeStandardCarrierSet(SoundplaneDriver::Carriers &carriers, int set) {
    int startOffset = 2;
    int skipSize = 2;
    int gapSize = 4;
    int gapStart = set * skipSize + startOffset;
    carriers[0] = carriers[1] = 0;
    for (int i = startOffset; i < gapStart; ++i) {
        carriers[i] = kModelDefaultCarriers[i];
    }
    for (int i = gapStart; i < kSoundplaneNumCarriers; ++i) {
        carriers[i] = kModelDefaultCarriers[i + gapSize];
    }
}


class SPLiteImpl_ {
public:
    SPLiteImpl_(void);

    ~SPLiteImpl_(void) = default;
    void start(void);
    void stop(void);

    void addCallback(std::shared_ptr<SPLiteCallback> cb) { listener_.addCallback(cb); }

    void maxTouches(unsigned t) { listener_.maxTouches(t); }


private:
    class Listener : public SoundplaneDriverListener {
    public:
        Listener(SPLiteImpl_ *parent) : parent_(parent) { ; }

        friend class SPLiteImpl_;

        void addCallback(std::shared_ptr<SPLiteCallback> cb);

        void maxTouches(unsigned t) { maxTouches_ = t; }

        //SoundplaneDriverListener
        void onStartup(void) override;
        void onFrame(const SensorFrame &frame) override;
        void onError(int err, const char *errStr) override;
        void onClose(void) override;


    private:
        void dumpTouches(TouchArray &t);
        void onTouches(TouchArray &t);

        bool isCal_ = false;
        bool isCarrierSet_ = false;
        SoundplaneDriver::Carriers carriers_;
        SensorFrameStats stats_;
        SensorFrame calibrateMeanInv_{};
        SensorFrame calibratedFrame_{};
        unsigned maxTouches_ = kMaxTouches;
        TouchTracker tracker_;
        SPLiteImpl_ *parent_;
        std::vector<std::shared_ptr<SPLiteCallback>> callbacks_;
    } listener_;

    std::unique_ptr<SoundplaneDriver> driver_ = nullptr;
};

//---------------------
SPLiteImpl_::SPLiteImpl_() : listener_(this), driver_(nullptr) {
    driver_ = SoundplaneDriver::create(listener_);
}

void SPLiteImpl_::start() {
    driver_->start();
}

void SPLiteImpl_::stop() {
    delete driver_.release();
    driver_ = nullptr;
}

void SPLiteImpl_::Listener::onStartup(void) {
    isCarrierSet_ = false;
    isCal_ = false;

    tracker_.setRotate(false);
    tracker_.setThresh(0.05f);
    tracker_.setLopassZ(100.0f);
}


void SPLiteImpl_::Listener::onFrame(const SensorFrame &frame) {
    if (!parent_ || !parent_->driver_) return;
    auto state = parent_->driver_->getDeviceState();

    if (state == kDeviceHasIsochSync) {
        if (!isCarrierSet_) {
            makeStandardCarrierSet(carriers_, kStandardCarrierSets);
            parent_->driver_->setCarriers(carriers_);
            unsigned long carrierMask = 0xFFFFFFFF;
            parent_->driver_->enableCarriers(~carrierMask);
            isCarrierSet_ = true;

            // start calibration
            stats_.clear();
            isCal_ = false;
            tracker_.clear();
        } else if (!isCal_) {
            stats_.accumulate(frame);
            if (stats_.getCount() >= kSoundplaneCalibrateSize) {
                SensorFrame mean = clamp(stats_.mean(), 0.0001f, 1.f);
                calibrateMeanInv_ = divide(fill(1.f), mean);
                isCal_ = true;
            }

            for (auto cb : callbacks_) {
                cb->onInit();
            }
        } else {
            for (auto cb : callbacks_) {
                cb->onFrame();
            }
            calibratedFrame_ = subtract(multiply(frame, calibrateMeanInv_), 1.0f);
            SensorFrame curvature = tracker_.preprocess(calibratedFrame_);
            TouchArray t = tracker_.process(curvature, maxTouches_);
            onTouches(t);
        }
    }
}

void SPLiteImpl_::Listener::onError(int err, const char *errStr) {
    std::strstream errstr;
    switch (err) {
        case kDevDataDiffTooLarge:
            errstr << "error: frame difference too large: " << errStr;
            stats_.clear();
            isCal_ = false;
            break;
        case kDevGapInSequence:
            errstr << "note: gap in sequence " << errStr;
            break;
        case kDevReset:
            errstr << "isoch stalled, resetting " << errStr;
            break;
        case kDevPayloadFailed:
            errstr << "payload failed at sequence " << errStr;
            break;
    }
    for (auto cb : callbacks_) {
        cb->onError(static_cast<unsigned int>(err), errstr.str());
    }
}

void SPLiteImpl_::Listener::onClose(void) {
    for (auto cb : callbacks_) {
        cb->onDeinit();
    }
}


void SPLiteImpl_::Listener::onTouches(TouchArray &ta) {
//    dumpTouches(ta);
    for (unsigned idx = 0; idx < maxTouches_; idx++) {
        Touch &t = ta[idx];
        switch (t.state) {
            case kTouchStateOn : {
                for (auto cb : callbacks_) {
                    cb->touchOn(idx, t.x, t.y, t.z);
                }
                break;
            };
            case kTouchStateContinue : {
                for (auto cb : callbacks_) {
                    cb->touchContinue(idx, t.x, t.y, t.z);
                }
                break;
            };
            case kTouchStateOff : {
                for (auto cb : callbacks_) {
                    cb->touchOff(idx, t.x, t.y, t.z);
                }
                break;
            };
            default :
                break;
        }
    }

}


void SPLiteImpl_::Listener::dumpTouches(TouchArray &ta) {
    std::cout << std::fixed << std::setw(6) << std::setprecision(4);
    for (int i = 0; i < maxTouches_; ++i) {
        if (touchIsActive(ta[i])) {
            std::cout << " i:" << i;
            std::cout << " x:" << ta[i].x;
            std::cout << " y:" << ta[i].y;
            std::cout << " z:" << ta[i].z;
//                std::cout << " n:" << t[i].note;
            std::cout << std::endl;
        }
    }
}

void SPLiteImpl_::Listener::addCallback(std::shared_ptr<SPLiteCallback> cb) {
    callbacks_.push_back(cb);
}

//---------------------

SPLiteDevice::SPLiteDevice() : impl_(new SPLiteImpl_()) {
}

SPLiteDevice::~SPLiteDevice() {
    delete impl_;
    impl_ = nullptr;
}

void SPLiteDevice::start() {
    impl_->start();
}

void SPLiteDevice::stop() {
    impl_->stop();
}

void SPLiteDevice::addCallback(std::shared_ptr<SPLiteCallback> cb) {
    impl_->addCallback(cb);
}

void SPLiteDevice::maxTouches(unsigned t) {
    impl_->maxTouches(t);
}
