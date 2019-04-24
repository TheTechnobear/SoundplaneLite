/***** splayout2.h *****/
#pragma once

#include "splayout.h"


class ZoneLayout_2 : public SPLayout {
public:
	void touch(SPTouch& t) override{
	    // t.z_=t.z_;
	    float startZone=0.0f;
	    float endZone=30.0f;
        if(t.x_ < zone1_) {
			t.zone_=0;
			startZone=0.0f;
			endZone=zone1_;
        } else {
			t.zone_=1;
			startZone=zone1_;
			endZone=30.0f;
        } // zone 1

		checkAndReleaseOldTouch(t);

		switch(pitchMode()) {
			case PitchMode::NONE : {
			    t.y_ = (t.y_ - 2.5f) / 2.5f; // -1...1
				t.x_ = partialX(t.x_, startZone, endZone );
				break;
			}
			case PitchMode::SINGLE : {
			    t.y_ = (t.y_ - 2.5f) / 2.5f; // -1...1
				t.pitch_= (t.x_ - startZone);
				t.x_ = partialX(t.x_, startZone, endZone );
				if(!processTouch(t)) return;
				break;
			}
			case PitchMode::FOURTHS : {
				int row = t.y_;
				t.y_= ((t.y_ - row) * 2.0f) - 1.0f;
				t.pitch_= (t.x_ - startZone) + ((row - 2) * 5.0f) ;
				t.x_ = partialX(t.x_, startZone, endZone );
				if(!processTouch(t)) return;
				break;
			}
		}
		output(t);				
	}

	void output(const SPTouch& t) override {
		t_[t.zone_] = t;
	}
	
	void render(BelaContext *context) override {
		for(int i=0;i<2;i++) renderTouch(context,t_[i]);
	}

	void renderTouch(BelaContext *context,const SPTouch& t) override {
		int zoneOffset = t.zone_ * 4;

    	// pitch, y, z, x
		float pitch = t.x_;
		if(pitchMode()!=PitchMode::NONE) {
			pitch = transpose(t.pitch_, int((analogRead(context, 0, 0+zoneOffset) - 0.5) * 6) ,-3);
		}
		float y = scaleY(t.y_,analogRead(context, 0, 1+zoneOffset)* 2);
		float z = pressure(t.z_,analogRead(context, 0, 2+zoneOffset) * 2);
		float amp = audioAmp(t.z_,analogRead(context, 0, 2+zoneOffset) * 2);

		for(unsigned int n = 0; n < context->digitalFrames; n++) {
			digitalWriteOnce(context, n, t.zone_ * 2,t_.active_);	
		}

		for(unsigned int n = 0; n < context->analogFrames; n++) {
			analogWriteOnce(context, n, 0 +,zoneOffset,pitch_);
			analogWriteOnce(context, n, 1 +zoneOffset,y);
			analogWriteOnce(context, n, 2 +zoneOffset,z);
			analogWriteOnce(context, n, 3 +zoneOffset,x);
		}

		for(unsigned int n = 0; n < context->audioFrames; n++) {
			float v = audioRead(context, n, t.zone_) * amp;
			audioWrite(context, n, t.zone_, v);
		}

	}
	static constexpr float zone1_ = 12.0f;

	SPTouch t_[2];
};

