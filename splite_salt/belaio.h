/***** belaio.h *****/
#pragma once

#include "defs.h"

class BelaIO {
public: 
	BelaIO() {
		digitalOutPin_[0] = trigOut1;
		digitalOutPin_[1] = trigOut2;
		digitalOutPin_[2] = trigOut3;
		digitalOutPin_[3] = trigOut4;
		audioOut(0,0.0f);
		audioOut(1,0.0f);
	}

	unsigned numDigitalOuts() {return MAX_DIGITAL_OUT;}
	unsigned digitalOutPin(unsigned idx) { return digitalOutPin_[idx];}
	bool 	 digitalOut(unsigned idx) { return digitalOut_[idx];}
	void 	 digitalOut(unsigned idx, bool value) { digitalOut_[idx]=value;}
	
	unsigned numAnalogOut() {return MAX_ANALOG_OUT;}
	float 	 analogOut(unsigned idx) { return analogOut_[idx];}
	void 	 analogOut(unsigned idx, float value) { analogOut_[idx]=value;}

	unsigned numAudioOut() {return MAX_AUDIO_OUT;}
	float 	 audioOut(unsigned idx) { return audioOut_[idx];}
	void 	 audioOut(unsigned idx, float value) { audioOut_[idx]=value;}


	unsigned numAnalogIn() {return MAX_ANALOG_IN;}
	float 	 analogIn(unsigned idx) { return analogIn_[idx];}
	void 	 analogIn(unsigned idx, float value) { analogIn_[idx]=value;}
		
	
private:
	static constexpr unsigned MAX_AUDIO_OUT=2;
	static constexpr unsigned MAX_DIGITAL_OUT=4;
	static constexpr unsigned MAX_ANALOG_OUT=8;
	static constexpr unsigned MAX_ANALOG_IN=8;
	
	bool 		digitalOut_[MAX_DIGITAL_OUT];
	unsigned 	digitalOutPin_[MAX_DIGITAL_OUT];
	float 		analogOut_[MAX_ANALOG_OUT];
	float 		analogIn_[MAX_ANALOG_IN];
	float 		audioOut_[MAX_AUDIO_OUT];
};
