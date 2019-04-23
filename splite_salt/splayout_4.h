/***** splayout4.h *****/
#pragma once

#include "splayout.h"

class ZoneLayout_4 : public SPLayout {
public:
	void touch(SPTouch& t) override {
	    // t.z_=t.z_;
        if(t.x_ < zone1_) {
		    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
			t.zone_=0;
			checkAndReleaseOldTouch(t);
			output(t);
        } else if(t.x_ < zone3_) {
        	// pitch zones
		    float startZone=0.0f;
		    float endZone=30.0f;
			if(t.x_ < zone2_) {
				t.zone_=1;
				startZone=zone1_;
				endZone=zone2_;
			} else {
				t.zone_=2;
				startZone=zone2_;
				endZone=zone3_;
			}
			
			switch(pitchMode()) {
				case PitchMode::NONE : {
				    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
					t.x_ = partialX(t.x_, startZone, endZone );
					checkAndReleaseOldTouch(t);
					output(t);				
					break;
				}
				case PitchMode::SINGLE : {
				    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
					t.pitch_= (t.x_ - startZone);
					t.x_ = partialX(t.x_, startZone, endZone );
					checkAndReleaseOldTouch(t);
					processTouch(t);
					break;
				}
				case PitchMode::FOURTHS : {
					int row = t.y_;
					t.y_= ((t.y_ - row) * 2.0f) - 1.0f;
					t.pitch_= (t.x_ - startZone) + ((row - 2)* 5.0f) ;
					t.x_ = partialX(t.x_, startZone, endZone );
					checkAndReleaseOldTouch(t);
					processTouch(t);
					break;
				}
			}
        } else {
		    t.y_=(t.y_ - 2.5f) / 2.5f; // -1...1
			t.zone_=3;
			checkAndReleaseOldTouch(t);
			output(t);
        }
	}

	void output(const SPTouch& t) override {
		switch(t.zone_) {
			case 0 : {
				float y = scaleY(t.y_,belaio_.analogIn(3) * 2);
				
                belaio_.digitalOut(1, t.active_);
                belaio_.analogOut(3, y);
				break;	
			}
			case 1 : 
			case 2 : {
				int zoneOffset = (t.zone_ - 1)  * 4;
				int zoneTranspose = t.zone_ == 1 ? -1 : -3;
		    	// pitch, y, z, x
				float pitch = t.x_;
				if(pitchMode()!=PitchMode::NONE) {
					pitch = transpose(t.pitch_, int((belaio_.analogIn(0+zoneOffset) - 0.5) * 6) , zoneTranspose);
				}
				float y = scaleY(t.y_,belaio_.analogIn(1+zoneOffset) * 2);
				float z = pressure(t.z_,belaio_.analogIn(2+zoneOffset) * 2);
				float amp = audioAmp(t.z_,belaio_.analogIn(2+zoneOffset) * 2);
		
		        belaio_.digitalOut((t.zone_ - 1) * 2, t.active_); // 0 and 2
		        
		        belaio_.analogOut(0+zoneOffset, pitch);
		        belaio_.analogOut(1+zoneOffset, y);
		        belaio_.analogOut(2+zoneOffset, z);
		        belaio_.analogOut(3+zoneOffset, t.x_);
		        
		        belaio_.audioOut(t.zone_ - 1, amp);
				break;	
			}
			case 3 : {
				float y = scaleY(t.y_,belaio_.analogIn(7) * 2);

                belaio_.digitalOut(3, t.active_);
                belaio_.analogOut(7, y);
				break;	
			}
			default: ;
		}
	}
	
	static constexpr float zone1_ = 2.0f;
	static constexpr float zone2_ = 12.0f;
	static constexpr float zone3_ = 28.0f;
};
