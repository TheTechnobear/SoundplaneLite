#include <Bela.h>

#include <SPLiteDevice.h>


AuxiliaryTask gSPLiteProcessTask;


#include <iostream>

class BelaSPCallback : public SPLiteCallback {
public:
	BelaSPCallback() {
		;
	}
	
   void touchOn(unsigned  tId, float x, float y, float z) override {
        // std::cout << " touchOn:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
    }

    void touchContinue(unsigned tId, float x, float y, float z) override {
        // std::cout << " touchContinue:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
    }

    void touchOff(unsigned tId, float x, float y, float z) override {
        // std::cout << " touchOff:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
    }
};

SPLiteDevice *gpDevice = nullptr;
auto gCallback = std::make_shared<BelaSPCallback>();

bool setup(BelaContext *context, void *userData)
{
	gpDevice = new SPLiteDevice();
    gpDevice->addCallback(gCallback);
    gpDevice->maxTouches(4);
    gpDevice->start();
	
    // Initialise auxiliary tasks

	// if((gSPLiteProcessTask = Bela_createAuxiliaryTask( xxxxxxx, BELA_AUDIO_PRIORITY - 1, "SPLiteProcessTask", gpDevice)) == 0)
		// return false;

	return true;
}

// render is called 2750 per second (44000/16)
// const int decimation = 5;  // = 550/seconds
long renderFrame = 0;
void render(BelaContext *context, void *userData)
{

	// Bela_scheduleAuxiliaryTask(gSPLiteProcessTask);
	
	renderFrame++;
	// silence audio buffer
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, 0.0f);
		}
	}
    // distribute touches to cv
}

void cleanup(BelaContext *context, void *userData)
{
// 	// gpDevice->removeCallback(gCallback);
	gpDevice->stop();
	delete gpDevice;
}