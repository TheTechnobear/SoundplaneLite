#include <Bela.h>

#include <SPLiteDevice.h>


AuxiliaryTask gSPLiteProcessTask;


class BelaSPCallback : public SPLiteCallback {
public:
	BelaMidiMecCallback() :	pitchbendRange_(48.0) {
		;
	}
	
    virtual void touchOn(int touchId, float x, float y, float z) {
    }

    virtual void touchContinue(int touchId, float x, float y, float z) {
    }

    virtual void touchOff(int touchId, float x, float y, float z) {
    }
    
    
private:
    float pitchbendRange_;
};

SPLiteDevice *gpDevice = nullptr;
BelaSPCallback gCallback;

bool setup(BelaContext *context, void *userData)
{

	gpDevice = new SPLiteDevice();
    gpDevice->maxTouches(4);
    gpDevice->start();
    gpDevice->addCallback(gCallback)
	
    // Initialise auxiliary tasks

	// if((gSPLiteProcessTask = Bela_createAuxiliaryTask( xxxxxxx, BELA_AUDIO_PRIORITY - 1, "SPLiteProcessTask", gpDevice)) == 0)
		// return false;

	return true;
}

// render is called 2750 per second (44000/16)
const int decimation = 5;  // = 550/seconds
long renderFrame = 0;
void render(BelaContext *context, void *userData)
{

	Bela_scheduleAuxiliaryTask(gSPLiteProcessTask);
	
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
	// gpDevice->removeCallback(gCallback);
    gpDevice->stop();
	delete gpDevice;
}