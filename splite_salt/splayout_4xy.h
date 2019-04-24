/***** splayout4xy.h *****/
#pragma once

#include "splayout.h"

class ZoneLayout_4XY : public SPLayout {
public:
	ZoneLayout_4XY() {
		zoneT_[0].zone_=0;
		zoneT_[1].zone_=1;
		zoneT_[2].zone_=2;
		zoneT_[3].zone_=3;
	}

	unsigned signature() override { return calcSignature(1,2);}

	void touch(SPTouch& t) override {
	    // t.z_=t.z_;
	    float startZone=0.0f;
	    float endZone=30.0f;
        if(t.x_ < zone1_) {
			t.zone_=0;
			startZone = 0.0f;
			endZone = zone1_;
        } else if( t.x_ < zone2_) {
			t.zone_=1;
			startZone = zone1_;
			endZone = zone2_;
        } else if( t.x_ < zone3_) {
			t.zone_=2;
			startZone = zone2_;
			endZone = zone3_;
        } else {
			t.zone_=3;
		    startZone=zone3_;
		    endZone=30.0f;
        }
        
		checkAndReleaseOldTouch(t);

		if(t.zone_< 3 )   {
		    t.x_=gridX(t.x_, startZone, endZone);
		    t.y_=gridY(t.y_, 0.0f, 5.0f);
		} else {
			switch(pitchMode()) {
				case PitchMode::NONE : {
				    t.y_ = gridY(t.y_, 0.0f, 5.0f);
					t.x_ = gridX(t.x_, startZone, endZone );
					output(t);				
					break;
				}
				case PitchMode::SINGLE : {
				    t.y_ = gridY(t.y_, 0.0f, 5.0f);
					t.pitch_= (t.x_ - startZone);
					t.x_ = gridX(t.x_, startZone, endZone );
					if(!processTouch(t)) return;
					break;
				}
				case PitchMode::FOURTHS : {
					int row = t.y_;
					t.y_ = ((t.y_ - row) * 2.0f) - 1.0f;
					t.pitch_= (t.x_ - startZone) + ((row - 2)* 5.0f) ;
					t.x_ = gridX(t.x_, startZone, endZone);
					if(!processTouch(t)) return;
					break;
				}
			}
		}

		SPTouch& lT = lastTouch_[t.tId_];
		lT=t;
		output(t);
	}


	void output(const SPTouch& t) override {
		zoneT_[t.zone_] = t;
	}

	
	void render(BelaContext *context) override {
		//unused
		for(int i=0;i<4;i++) renderTouch(context,zoneT_[i]);
	}

	void renderTouch(BelaContext *context,const SPTouch& t) {
		switch(t.zone_) {
			case 0 : {
				float x = scaleX(t.x_,analogRead(context, 0, 2)* 2);
				float y = scaleY(t.y_,analogRead(context, 0, 3)* 2);
				
				float ampX = audioAmp((t.x_ + 1.0) / 2.0f, analogRead(context, 0, 2) * 2);
				float ampY = audioAmp((t.y_ + 1.0) / 2.0f, analogRead(context, 0, 3) * 2);

				for(unsigned int n = 0; n < context->digitalFrames; n++) {
					digitalWriteOnce(context, n,trigOut2 ,t.active_);	
				}
				for(unsigned int n = 0; n < context->analogFrames; n++) {
					analogWriteOnce(context, n, 2,x);
					analogWriteOnce(context, n, 3,y);
				}
		
				for(unsigned int n = 0; n < context->audioFrames; n++) {
					float v0 = audioRead(context, n, 0) * ampX;
					audioWrite(context, n, 0, v0);
					float v1 = audioRead(context, n, 1) * ampY;
					audioWrite(context, n, 1, v1);
				}
				break;	
			}
			case 1 : {
				float x = scaleX(t.x_,analogRead(context, 0, 4)* 2);
				float y = scaleY(t.y_,analogRead(context, 0, 5)* 2);
				for(unsigned int n = 0; n < context->digitalFrames; n++) {
					digitalWriteOnce(context, n,trigOut3 ,t.active_);	
				}
				for(unsigned int n = 0; n < context->analogFrames; n++) {
					analogWriteOnce(context, n, 4,x);
					analogWriteOnce(context, n, 5,y);
				}
				break;	
			}
			case 2 : {
				float x = scaleX(t.x_,analogRead(context, 0, 6)* 2);
				float y = scaleY(t.y_,analogRead(context, 0, 7)* 2);
				for(unsigned int n = 0; n < context->digitalFrames; n++) {
					digitalWriteOnce(context, n,trigOut4 ,t.active_);	
				}
				for(unsigned int n = 0; n < context->analogFrames; n++) {
					analogWriteOnce(context, n, 6,x);
					analogWriteOnce(context, n, 7,y);
				}
				break;	
			}
			case 3 : {
				float pitch = t.x_;
				if(pitchMode()!=PitchMode::NONE) {
					pitch = transpose(t.pitch_, int((analogRead(context, 0, 0) - 0.5) * 6) ,0);
				}
				float y = scaleY(t.y_,analogRead(context, 0, 1)* 2);

				for(unsigned int n = 0; n < context->digitalFrames; n++) {
					digitalWriteOnce(context, n,trigOut1 ,t.active_);	
				}
		
				for(unsigned int n = 0; n < context->analogFrames; n++) {
					analogWriteOnce(context, n, 0 ,pitch);
					analogWriteOnce(context, n, 1 ,y);
				}
				break;	
			}
			default:
				break;
		}
	}
	
	
	SPTouch zoneT_[4];

	static constexpr float zone1_ = 5.0f;
	static constexpr float zone2_ = 10.0f;
	static constexpr float zone3_ = 15.0f;
};