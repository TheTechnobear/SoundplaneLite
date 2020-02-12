#pragma once

#include <memory>
#include <vector>

class SPLiteCallback {
public:
    virtual ~SPLiteCallback() = default;
    virtual void onInit()   {;}
    virtual void onFrame()  {;}
    virtual void onDeinit() {;}
    virtual void onError(unsigned err, const char *errStr) {;}

    virtual void touchOn(unsigned voice, float x,float y, float z) = 0;
    virtual void touchContinue(unsigned voice, float x,float y, float z) = 0;
    virtual void touchOff(unsigned voice, float x,float y, float z) = 0;
};

class SPLiteImpl_;

class SPLiteDevice {
public:
    SPLiteDevice();
    virtual ~SPLiteDevice();
    void start();
    void stop();
    unsigned process(); // call periodically
    void addCallback(std::shared_ptr<SPLiteCallback>);
    void maxTouches(unsigned);
    void overrideCarrierSet(int carrier);
private:
    SPLiteImpl_* impl_;
};

