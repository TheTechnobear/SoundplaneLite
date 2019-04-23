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

		switch(pitchMode()) {
			case PitchMode::NONE : {
			    t.y_ = (t.y_ - 2.5f) / 2.5f; // -1...1
				t.x_ = partialX(t.x_, startZone, endZone );
				checkAndReleaseOldTouch(t);
				output(t);				
				break;
			}
			case PitchMode::SINGLE : {
			    t.y_ = (t.y_ - 2.5f) / 2.5f; // -1...1
				t.pitch_= (t.x_ - startZone);
				t.x_ = partialX(t.x_, startZone, endZone );
				checkAndReleaseOldTouch(t);
				processTouch(t);
				break;
			}
			case PitchMode::FOURTHS : {
				int row = t.y_;
				t.y_= ((t.y_ - row) * 2.0f) - 1.0f;
				t.pitch_= (t.x_ - startZone) + ((row - 2) * 5.0f) ;
				t.x_ = partialX(t.x_, startZone, endZone );
				checkAndReleaseOldTouch(t);
				processTouch(t);
				break;
			}
		}

	}
	
	void output(const SPTouch& t) override {
		int zoneOffset = t.zone_ * 4;

    	// pitch, y, z, x
		float pitch = t.x_;
		if(pitchMode()!=PitchMode::NONE) {
			pitch = transpose(t.pitch_, int((belaio_.analogIn(0+zoneOffset) - 0.5) * 6) ,-3);
		}
		float y = scaleY(t.y_,belaio_.analogIn(1+zoneOffset) * 2);
		float z = pressure(t.z_,belaio_.analogIn(2+zoneOffset) * 2);
		float amp = audioAmp(t.z_,belaio_.analogIn(2+zoneOffset) * 2);

        belaio_.digitalOut(t.zone_ * 2, t.active_); // 0 and 2
        
        belaio_.analogOut(0+zoneOffset, pitch);
        belaio_.analogOut(1+zoneOffset, y);
        belaio_.analogOut(2+zoneOffset, z);
        belaio_.analogOut(3+zoneOffset, t.x_);
        
        belaio_.audioOut(t.zone_, amp);
	}
	static constexpr float zone1_ = 12.0f;
};

