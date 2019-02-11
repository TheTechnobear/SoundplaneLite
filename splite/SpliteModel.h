// based off SoundplaneModel

#pragma once 

#include <list>
#include <map>
#include <thread>
#include <stdint.h>

#include "MLModel.h"
#include "SoundplaneModelA.h"
#include "SoundplaneDriver.h"
#include "TouchTracker.h"
#include "MLSymbol.h"
#include "cJSON/cJSON.h"
#include "Controller.h"
//#include "Zone.h"
#include "MLQueue.h"

using namespace ml;
using namespace std::chrono;


#define MLConsole() std::cerr

MLSignal sensorFrameToSignal(const SensorFrame &f);

typedef enum
{
	xColumn = 0,
	yColumn = 1,
	zColumn = 2,
	dzColumn = 3,
	ageColumn = 4
} TouchSignalColumns;

const int kSensorFrameQueueSize = 16;

class SpliteModel :
public SoundplaneDriverListener,
public MLModel
{
public:
	
	SpliteModel();
	~SpliteModel();
	
	// SoundplaneDriverListener
	void onStartup() override;
	void onFrame(const SensorFrame& frame) override;
	void onError(int error, const char* errStr) override;
	void onClose() override;
	

	void setAllPropertiesToDefaults();
	

	float getSampleHistory(int x, int y);
	
	void getHistoryStats(float& mean, float& stdDev);
	int getWidth() { return mSurface.getWidth(); }
	int getHeight() { return mSurface.getHeight(); }
	
	void setDefaultCarriers();
	void setCarriers(const SoundplaneDriver::Carriers& c);
	int enableCarriers(unsigned long mask);
	int getNumCarriers() { return kSoundplaneNumCarriers; }
	void dumpCarriers(const SoundplaneDriver::Carriers& carriers);
	
	void enableOutput(bool b);
	
	int getStateIndex();
	const char* getHardwareStr();
	const char* getStatusStr();
	const char* getClientStr();
	
	int getSerialNumber() const {return mSerialNumber;}
	
	void clear();
	
	void setRaw(bool b);
	bool getRaw(){ return mRaw; }
	
	void beginCalibrate();
	bool isCalibrating() { return mCalibrating; }
	float getCalibrateProgress();
	void endCalibrate();
	
	void beginSelectCarriers();
	bool isSelectingCarriers() { return mSelectingCarriers; }
	float getSelectCarriersProgress();
	void nextSelectCarriersStep();
	void endSelectCarriers();
	
	void setFilter(bool b);
	
	void getMinMaxHistory(int n);
	
	const MLSignal& getTouchFrame() { return mTouchFrame; }
	const MLSignal& getTouchHistory() { return mTouchHistory; }
	const MLSignal getRawSignal() { std::lock_guard<std::mutex> lock(mRawSignalMutex); return mRawSignal; }
	const MLSignal getCalibratedSignal() { std::lock_guard<std::mutex> lock(mCalibratedSignalMutex); return sensorFrameToSignal(mCalibratedFrame); }
	
	const MLSignal getSmoothedSignal() { std::lock_guard<std::mutex> lock(mSmoothedSignalMutex); return mSmoothedSignal; }
	
	const TouchArray& getTouchArray() { return mTouchArray1; }
	
	bool isWithinTrackerCalibrateArea(int i, int j);
	const int getHistoryCtr() { return mHistoryCtr; }
	
//	const std::vector< Zone >::const_iterator getZonesBegin(){ return mZones.begin(); }
//	const std::vector< Zone >::const_iterator getZonesEnd(){ return mZones.end(); }
	
	void setStateFromJSON(cJSON* pNode, int depth);
	bool loadZonePresetByName(const std::string& name);
	
	int getDeviceState(void);
	int getClientState(void);
	

private:
	TouchArray mTouchArray1{};
	TouchArray mZoneOutputTouches{};
	
	std::unique_ptr< SoundplaneDriver > mpDriver;
	std::unique_ptr< Queue< SensorFrame > > mSensorFrameQueue;
	
	// TODO order!
	void process(time_point<system_clock> now);
	void outputTouches(TouchArray touches, time_point<system_clock> now);
	void dumpOutputsByZone();
	
	TouchArray trackTouches(const SensorFrame& frame);
	TouchArray getTestTouchesFromTracker(time_point<system_clock> now);
	void saveTouchHistory(const TouchArray& t);

	void initialize();
	bool findNoteChanges(TouchArray t0, TouchArray t1);
	TouchArray scaleTouchPressureData(TouchArray in);
	
	void sendTouchesToZones(TouchArray touches);
	
	void sendFrameToOutputs(time_point<system_clock> now);
	void beginOutputFrame(time_point<system_clock> now);
	void sendTouchToOutputs(int i, int offset, const Touch& t);
	void sendControllerToOutputs(int zoneID, int offset, const Controller& m);
	void endOutputFrame();
	
	void clearZones();
	void sendParametersToZones();
	
//	std::vector< Zone > mZones;
	MLSignal mZoneIndexMap;
	
	bool mOutputEnabled;
	
	static const int kMiscStringSize{256};
	void loadZonesFromString(const std::string& zoneStr);
	
	void doInfrequentTasks();
	uint64_t mLastInfrequentTaskTime;
	
	int mSerialNumber;

	SensorFrame mSensorFrame{};
	SensorFrame mCalibratedFrame{};
	
	MLSignal mSurface;
	
	int	mMaxTouches;
	
	MLSignal mTouchFrame;
	std::mutex mTouchFrameMutex;
	MLSignal mTouchHistory;
	
	bool mCalibrating;
	bool mTestTouchesOn;
	bool mTestTouchesWasOn;
	bool mRequireSendNextFrame{false};
	bool mSelectingCarriers;
	bool mRaw;
	bool mSendMatrixData;
	
	SoundplaneDriver::Carriers mCarriers;
	
	bool mHasCalibration;
	
	SensorFrameStats mStats;
	SensorFrame mCalibrateMeanInv{};
	
	MLSignal mRawSignal;
	std::mutex mRawSignalMutex;
	
	MLSignal mCalibratedSignal;
	std::mutex mCalibratedSignalMutex;
	
	SensorFrame mSmoothedFrame{};
	MLSignal mSmoothedSignal;
	std::mutex mSmoothedSignalMutex;
	
	int mCalibrateStep; // calibrate step from 0 - end
	int mTotalCalibrateSteps;
	int mSelectCarriersStep;
	
	float mSurfaceWidthInv;
	float mSurfaceHeightInv;
	
	// store current key for each touch to implement hysteresis.
	int mCurrentKeyX[kMaxTouches];
	int mCurrentKeyY[kMaxTouches];
	
	char mHardwareStr[kMiscStringSize];
	char mStatusStr[kMiscStringSize];
	char mClientStr[kMiscStringSize];
	
	TouchTracker mTracker;
	
	int mHistoryCtr;
	bool mCarrierMaskDirty;
	bool mNeedsCarriersSet;
	bool mNeedsCalibrate;
	unsigned long mCarriersMask;
	
	bool mDoOverrideCarriers;
	SoundplaneDriver::Carriers mOverrideCarriers;
	
	std::vector<float> mMaxNoiseByCarrierSet;
	std::vector<float> mMaxNoiseFreqByCarrierSet;
	
	bool mVerbose;
	
	bool mTerminating{false};
	int mProcessCounter{0};
	void processThread();
	std::thread mProcessThread;
	
	size_t mMaxRecentQueueSize{0};
	
	int mDataRate{100};
	time_point<system_clock> mPrevProcessTouchesTime{};

	float propHysteresis;
	float propZ_scale;
	float propZ_curve;

};


