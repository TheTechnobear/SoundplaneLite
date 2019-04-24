/***** splayout1.h *****/
#pragma once

#include "splayout.h"

class ZoneLayout_1 : public SPLayout {
public:
	void touch(SPTouch& t) override{
		t.zone_=0;

		switch(pitchMode()) {
			case PitchMode::NONE : {
				t.x_ = t.x_ / 30.0f;
				break;
			}
			case PitchMode::SINGLE : {
				t.pitch_= t.x_ ;
			    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
				t.x_ = t.x_ / 30.0f;
				if(!processTouch(t)) return;
				break;
			}
			case PitchMode::FOURTHS : {
				int row = t.y_;
				t.y_= ((t.y_ - row) * 2.0f) - 1.0f;
				t.pitch_= t.x_ + ((row - 2) * 5.0f) ;
				t.x_ = t.x_ / 30.0f;
				if(!processTouch(t)) return;
				break;
			}
		}
		output(t)
	}
	
	void output(const SPTouch& t) override {
    	// pitch, y, z, x 
    	t_ = t;
	}

	void render(BelaContext *context) {
		render(context,t_);
	}

	void render(BelaContext *context, const SPTouch& t) {

		float pitch = t.x_;
		if(pitchMode()!=PitchMode::NONE) {
			pitch = transpose(t.pitch_, int((analogRead(context, 0, 0) - 0.5) * 6) ,-3);
		}
		
		float y = scaleY(t.y_,analogRead(context, 0, 0) * 2);
		float z = pressure(t.z_,analogRead(context, 0, 1) * 2);
		float amp = audioAmp(t.z_,analogRead(context, 0, 2) * 2);

		for(unsigned int n = 0; n < context->digitalFrames; n++) {
			digitalWriteOnce(context, n, 0 ,t.active_);	
		}

		for(unsigned int n = 0; n < context->analogFrames; n++) {
			analogWriteOnce(context, n, 0,pitch);
			analogWriteOnce(context, n, 1,x);
			analogWriteOnce(context, n, 2,y);
			analogWriteOnce(context, n, 3,z);
		}

		for(unsigned int n = 0; n < context->audioFrames; n++) {
			float v = audioRead(context, n, channel) * amp;
			audioWrite(context, n, channel, v);
		}
	}

private:
	SPTouch t_;
};