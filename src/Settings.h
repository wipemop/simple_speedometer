#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

extern const char* IS_READ_ME_VISIBLE;
extern const char* READ_ME_POSITION_H;
extern const char* READ_ME_POSITION_V;
extern const char* IS_SPEEDOMETER_DIAL_VISIBLE;
extern const char* IS_SPEEDOMETER_TABLE_VISIBLE;
extern const char* IS_OPTION_2D_ENABLED;
extern const char* IS_OPTION_3D_ENABLED;
extern const char* IS_OPTION_UNITS_ENABLED;
extern const char* IS_OPTION_FEETPERCENT_ENABLED;
extern const char* IS_OPTION_BEETLEPERCENT_ENABLED;
extern const char* NEEDLE_AMPLITUDE_UNITS;
extern const char* NEEDLE_AMPLITUDE_FEETPERCENT;
extern const char* NEEDLE_AMPLITUDE_BEETLEPERCENT;
extern const char* SPEEDOMETER_DIAL_POSITION_H;
extern const char* SPEEDOMETER_DIAL_POSITION_V;
extern const char* SPEEDOMETER_DIAL_SCALE;
extern const char* IS_SPEEDOMETER_TIMER_VISIBLE;
extern const char* START_FADING_DISTANCE;
extern const char* FINISH_FADING_DISTANCE;
extern const char* IS_OPTION_PAUSE_ENABLED;
extern const char* IS_OPTION_STOP_ENABLED;
extern const char* MANUAL_START_DIAMETER_UNITS;
extern const char* IS_OPTION_MANUAL_ENABLED;
extern const char* IS_OPTION_PREDEFINED_ENABLED;
extern const char* IS_OPTION_CUSTOM_ENABLED;
extern const char* START_DIAMETER_UNITS;
extern const char* FINISH_DIAMETER_UNITS;
extern const char* START_X_OFFSET;
extern const char* START_Z_OFFSET;
extern const char* START_Y_OFFSET;
extern const char* FINISH_X_OFFSET;
extern const char* FINISH_Z_OFFSET;
extern const char* FINISH_Y_OFFSET;
extern const char* SPEEDOMETER_TIMER_POSITION_H;
extern const char* SPEEDOMETER_TIMER_POSITION_V;
extern const char* SPEEDOMETER_TIMER_SCALE;
extern const char* RESTORING_DEFAULTS;

extern const char* IS_CLOUDCONFIG_ENABLED;
extern const char* CLOUDCONFIG_ID;


namespace Settings
{
	extern std::mutex	Mutex;
	extern json			Settings;

	/* Loads the settings. */
	void Load(std::filesystem::path aPath);
	/* Saves the settings. */
	void Save(std::filesystem::path aPath);

	extern bool IsReadMeEnabled;
	extern float ReadMePositionH;
	extern float ReadMePositionV;
	extern bool IsDialEnabled;
	extern bool IsTableEnabled;
	extern bool option2D;
	extern bool option3D;
	extern bool optionUnits;
	extern bool optionFeetPercent;
	extern bool optionBeetlePercent;
	extern float amplitudeUnits;
	extern float amplitudeFeetPercent;
	extern float amplitudeBeetlePercent;
	extern float DialPositionH;
	extern float DialPositionV;
	extern float DialScale;
	extern bool IsTimerEnabled;
	extern float startFadingDistance;
	extern float finishFadingDistance;
	extern bool optionPause;
	extern bool optionStop;
	extern float manualstartDiameter;
	extern bool optionManual;
	extern bool optionPredefined;
	extern bool optionCustom;
	extern float startDiameter;
	extern float finishDiameter;
	extern float startXoffset;
	extern float startZoffset;
	extern float startYoffset;
	extern float finishXoffset;
	extern float finishZoffset;
	extern float finishYoffset;
	extern float TimerPositionH;
	extern float TimerPositionV;
	extern float TimerScale;
	extern bool restoreDefaults;

	extern bool IsCloudConfigEnabled;
	extern std::string cloudConfigID;

	//void DoSomething(const char* aIdentifier, bool aIsRelease);
}

#endif
