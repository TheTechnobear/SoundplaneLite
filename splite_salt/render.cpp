#include <Bela.h>

#include <SPLiteDevice.h>

#include <chrono>
#include <math.h>
#include <iostream>

// #include <Scope.h>
// Scope scope;

#include "defs.h"
#include "belaio.h"
#include "sptouch.h"
#include "splayout.h"

static BelaIO belaio_;

#include "splayout_1.h"
#include "splayout_2.h"
#include "splayout_4.h"
#include "splayout_4xy.h"


AuxiliaryTask gSPLiteProcessTask;

std::chrono::time_point<std::chrono::system_clock>  gStartTime;
std::chrono::time_point<std::chrono::system_clock>  gLastErrorTime;

//=====================================================================================


class BelaSPCallback : public SPLiteCallback {
public:
	BelaSPCallback() : layoutIdx_(0), 
			quantMode_(QuantMode::GLIDE),
			pitchMode_(PitchMode::SINGLE)  {
		gStartTime = std::chrono::system_clock::now();
		layouts_.push_back(new ZoneLayout_1());
		layouts_.push_back(new ZoneLayout_2());
		layouts_.push_back(new ZoneLayout_4());
		layouts_.push_back(new ZoneLayout_4XY());
		layouts_[layoutIdx_]->quantMode(quantMode_);
		layouts_[layoutIdx_]->pitchMode(pitchMode_);
	}
	
    void onError(unsigned, const char* err) override {
		gLastErrorTime = std::chrono::system_clock::now();
		std::chrono::duration<double> diff = gLastErrorTime-gStartTime;
		std::cerr << diff.count() << " secs " <<err << std::endl;
    }
    
   void touchOn(unsigned  tId, float x, float y, float z) override {
        // std::cout << " touchOn:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
        updateOutput(tId,true,x,y,z);
    }

    void touchContinue(unsigned tId, float x, float y, float z) override {
        // std::cout << " touchContinue:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
        updateOutput(tId,true, x,y,z);

    }

    void touchOff(unsigned tId, float x, float y, float z) override {
        // std::cout << " touchOff:" << tId << " x:" << x  << " y:" << y << " z:" << z << std::endl;
        updateOutput(tId,false, x,y,0.0f);
    }

    unsigned layout() { return layoutIdx_;}

    void switchLayout(unsigned idx) {
    	if(idx< layouts_.size()) layoutIdx_ = idx;
    	// TODO clear values?
    }
    
    void nextLayout() {
    	layoutIdx_ = ++layoutIdx_ % layouts_.size();
		layouts_[layoutIdx_]->quantMode(quantMode_);
		layouts_[layoutIdx_]->pitchMode(pitchMode_);
    }
    
    unsigned quantMode() { return quantMode_;}
    void nextQuantMode() {
    	quantMode_ = ++quantMode_ % 3;
		layouts_[layoutIdx_]->quantMode(quantMode_);
    }

    unsigned pitchMode() { return pitchMode_;}
    void nextPitchMode() {
    	pitchMode_ = ++pitchMode_ % 3;
		layouts_[layoutIdx_]->pitchMode(pitchMode_);
    }

private:
	void updateOutput(unsigned  tId, bool active,float x, float y, float z) {
		SPTouch touch(tId,active,x,y,z * 0.6f);
		layouts_[layoutIdx_]->touch(touch);
	}
	unsigned  layoutIdx_;
	unsigned  quantMode_;
	unsigned  pitchMode_;
	std::vector<SPLayout*> layouts_;
};

//=====================================================================================

SPLiteDevice *gpDevice = nullptr;
auto gCallback = std::make_shared<BelaSPCallback>();

void process_salt(void*) {
	gpDevice->process();
}

bool setup(BelaContext *context, void *userData)
{
	// scope.setup(2, context->audioSampleRate);
	
	pinMode(context,0,trigIn1,INPUT);
	pinMode(context,0,trigIn2,INPUT);
	pinMode(context,0,trigIn3,INPUT);
	pinMode(context,0,trigIn4,INPUT);


	for(unsigned i = 0; i< belaio_.numDigitalOuts(); i++) {
	 	pinMode(context, 0, belaio_.digitalOutPin(i), OUTPUT);
	}

	gpDevice = new SPLiteDevice();
    gpDevice->addCallback(gCallback);
    gpDevice->maxTouches(MAX_TOUCH);
    gpDevice->start();
	
    // Initialise auxiliary tasks
	if((gSPLiteProcessTask = Bela_createAuxiliaryTask( process_salt, BELA_AUDIO_PRIORITY - 5, "SPLiteProcessTask", gpDevice)) == 0)
		return false;

	return true;
}

//=====================================================================================



// render is called 2750 per second (44000/16)
// const int decimation = 5;  // = 550/seconds
long renderFrame = 0;
void render(BelaContext *context, void *userData)
{
#ifdef SALT
	drivePwm(context,pwmOut);

	static unsigned lsw,/*ltr1,*/ ltr2,ltr3,ltr4;
	static unsigned led_mode=0; // 0 = normal
	static unsigned led_counter=0;

	unsigned sw  = digitalRead(context, 0, switch1);  //next layout
	// unsigned tr1 = digitalRead(context, 0, trigIn1);  
	unsigned tr2 = digitalRead(context, 0, trigIn2);  //quantize 
	unsigned tr3 = digitalRead(context, 0, trigIn3);
	unsigned tr4 = digitalRead(context, 0, trigIn4);

	if(sw  && !lsw)  { 
		gCallback->nextLayout(); 
		led_counter=2000;
		led_mode=1;
	}
	
	if(led_mode==1) {
		led_counter--;
		if(led_counter>0) {
			unsigned layout = (gCallback->layout() + 1) % 16 ;
			setLed(context, ledOut1, (layout & 8) > 0 ) ;
			setLed(context, ledOut2, (layout & 4) > 0);
			setLed(context, ledOut3, (layout & 2) > 0);
			setLed(context, ledOut4, (layout & 1) > 0);
		} else {
			led_mode=0;
			led_counter=0;
			setLed(context, ledOut1, 0);
			setLed(context, ledOut2, 0);
			setLed(context, ledOut3, 0);
			setLed(context, ledOut4, 0);
		}
	}

	if(tr2  && !ltr2)  { gCallback->nextQuantMode(); }

	if(tr3  && !ltr3)  { gCallback->nextPitchMode(); }

	if(led_mode==0) {
		setLed(context, ledOut2, gCallback->quantMode() %3);
		setLed(context, ledOut3, gCallback->pitchMode() %3);
	}

	lsw =  sw;
	// ltr1 = tr1;
	ltr2 = tr2;
	ltr3=  tr3;
	ltr4=  tr4;
#endif

		
	
	renderFrame++;
	
	// if(renderFrame % 5 == 0) {
	    // distribute touches to cv
		Bela_scheduleAuxiliaryTask(gSPLiteProcessTask);
	// }
	
	for(unsigned int i=0; i< belaio_.numAnalogIn(); i++) {
		float v = analogRead(context, 0, i);
		belaio_.analogIn(i,v);
	}


	for(unsigned int i = 0; i < belaio_.numDigitalOuts();i++) {
		unsigned pin= belaio_.digitalOutPin(i);
		if(pin < context->digitalChannels) { 
			bool value = belaio_.digitalOut(i);
			for(unsigned int n = 0; n < context->digitalFrames; n++) {
				digitalWriteOnce(context, n, pin ,value);	
			}
		}
	}

	for(unsigned int i = 0; i < belaio_.numAnalogOut();i++) {
		float value = belaio_.analogOut(i);
		for(unsigned int n = 0; n < context->analogFrames; n++) {
			analogWriteOnce(context, n, i,value );
		}
	}

	float scopeOut[2];
	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
		float amp = belaio_.audioOut(channel);

		for(unsigned int n = 0; n < context->audioFrames; n++) {
			float v = audioRead(context, n, channel) * amp;
			audioWrite(context, n, channel, v);
			scopeOut[channel]=v;
		}
	}
	// scope.log(scopeOut[0],scopeOut[1]);
}

//=====================================================================================



void cleanup(BelaContext *context, void *userData)
{
// 	// gpDevice->removeCallback(gCallback);
	gpDevice->stop();
	delete gpDevice;
}