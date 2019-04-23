/***** splayout4xy.h *****/
#pragma once

#include "splayout.h"

class ZoneLayout_4XY : public SPLayout {
public:
	void touch(SPTouch& t) override {
	    // t.z_=t.z_;
        if(t.x_ < zone1_) {
			t.zone_=0;
		    t.x_=gridX(t.x_, 0.0f, zone1_);
		    t.y_=gridY(t.y_, 0.0f, 5.0f);
			checkAndReleaseOldTouch(t);
			output(t);
        } else if( t.x_ < zone2_) {
			t.zone_=1;
		    t.x_=gridX(t.x_, zone1_, zone2_);
		    t.y_=gridY(t.y_, 0.0f, 5.0f);
			checkAndReleaseOldTouch(t);
			output(t);
        } else if( t.x_ < zone3_) {
			t.zone_=2;
		    t.x_=gridX(t.x_, zone2_, zone3_);
		    t.y_=gridY(t.y_, 0.0f, 5.0f);
			checkAndReleaseOldTouch(t);
			output(t);
        } else {
			t.zone_=3;
		    float startZone=zone3_;
		    float endZone=30.0f;
			switch(pitchMode()) {
				case PitchMode::NONE : {
				    t.y_ = gridY(t.y_, 0.0f, 5.0f);
					t.x_ = gridX(t.x_, startZone, endZone );
					checkAndReleaseOldTouch(t);
					output(t);				
					break;
				}
				case PitchMode::SINGLE : {
				    t.y_ = gridY(t.y_, 0.0f, 5.0f);
					t.pitch_= (t.x_ - startZone);
					t.x_ = gridX(t.x_, startZone, endZone );
					checkAndReleaseOldTouch(t);
					processTouch(t);
					break;
				}
				case PitchMode::FOURTHS : {
					int row = t.y_;
					t.y_ = ((t.y_ - row) * 2.0f) - 1.0f;
					t.pitch_= (t.x_ - startZone) + ((row - 2)* 5.0f) ;
					t.x_ = gridX(t.x_, startZone, endZone);
					checkAndReleaseOldTouch(t);
					processTouch(t);
					break;
				}
			}
        }
	}

	void output(const SPTouch& t) override {
		switch(t.zone_) {
			case 0 : {
				float x = scaleX(t.x_,belaio_.analogIn(2) * 2);
				float y = scaleY(t.y_,belaio_.analogIn(3) * 2);
	            belaio_.digitalOut(1, t.active_);
                belaio_.analogOut(2, x);
                belaio_.analogOut(3, y);
		        belaio_.audioOut(0, audioAmp((t.x_ + 1.0) / 2.0f,belaio_.analogIn(2) * 2));
		        belaio_.audioOut(1, audioAmp((t.y_ + 1.0) / 2.0f,belaio_.analogIn(3) * 2));
				break;	
			}
			case 1 : {
				float x = scaleX(t.x_,belaio_.analogIn(4) * 2);
				float y = scaleY(t.y_,belaio_.analogIn(5) * 2);
	            belaio_.digitalOut(2, t.active_);
                belaio_.analogOut(4, x);
                belaio_.analogOut(5, y);
				break;	
			}
			case 2 : {
				float x = scaleX(t.x_,belaio_.analogIn(6) * 2);
				float y = scaleY(t.y_,belaio_.analogIn(7) * 2);
	            belaio_.digitalOut(3, t.active_);
                belaio_.analogOut(6, x);
                belaio_.analogOut(7, y);
				break;	
			}
			case 3 : {
		    	// pitch, y
				float pitch = t.x_;
				if(pitchMode()!=PitchMode::NONE) {
					pitch = transpose(t.pitch_, int((belaio_.analogIn(0) - 0.5) * 6) , 0);
				}
				float y = scaleY(t.y_,belaio_.analogIn(1) * 2);

		        belaio_.digitalOut(0, t.active_); // 0 and 2
		        belaio_.analogOut(0, pitch);
		        belaio_.analogOut(1, y);
				break;	
			}
			default:
				break;
		}
	}
	
	static constexpr float zone1_ = 5.0f;
	static constexpr float zone2_ = 10.0f;
	static constexpr float zone3_ = 15.0f;
};