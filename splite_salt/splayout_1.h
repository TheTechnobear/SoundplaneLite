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
				output(t);	
				break;
			}
			case PitchMode::SINGLE : {
				t.pitch_= t.x_ ;
			    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
				t.x_ = t.x_ / 30.0f;
				processTouch(t);
				break;
			}
			case PitchMode::FOURTHS : {
				int row = t.y_;
				t.y_= ((t.y_ - row) * 2.0f) - 1.0f;
				t.pitch_= t.x_ + ((row - 2) * 5.0f) ;
				t.x_ = t.x_ / 30.0f;
				processTouch(t);
				break;
			}
		}
	}
	
	void output(const SPTouch& t) override {
    	// pitch, y, z, x 
    	
		float pitch = t.x_;
		if(pitchMode()!=PitchMode::NONE) {
			pitch = transpose(t.pitch_, int((belaio_.analogIn(0) - 0.5) * 6) ,-3);
		}
		
		float y = scaleY(t.y_,belaio_.analogIn(1) * 2);
		float z = pressure(t.z_,belaio_.analogIn(2) * 2);
		float amp = audioAmp(t.z_,belaio_.analogIn(2) * 2);

        belaio_.digitalOut(0, t.active_);
        
        belaio_.analogOut(0, pitch); // C=3
        belaio_.analogOut(1, y);
        belaio_.analogOut(2, z);
        belaio_.analogOut(3, t.x_);
        
        belaio_.audioOut(0, amp);
        belaio_.audioOut(1, amp);
	}
};