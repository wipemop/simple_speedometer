#include "Settings.h"
#include "shared.h"
#include <filesystem>
#include <fstream>

const char* IS_READ_ME_VISIBLE = "IsReadMeVisible";
const char* READ_ME_POSITION_H = "ReadMePositionH";
const char* READ_ME_POSITION_V = "ReadMePositionV";
const char* IS_SPEEDOMETER_DIAL_VISIBLE = "IsSpeedometerDialVisible";
const char* IS_SPEEDOMETER_TABLE_VISIBLE = "IsSpeedometerTableVisible";
const char* SPEEDOMETER_VISIBLE_ALWAYS = "SpeedometerVisibleAlways";
const char* SPEEDOMETER_VISIBLE_ON_FOOT = "SpeedometerVisibleOnFoot";
const char* SPEEDOMETER_VISIBLE_MOUNTED = "SpeedometerVisibleMounted";
const char* FANCY_SPEEDOMETER_DIAL_VISIBLE = "FancySpeedometerDialVisible";
const char* MUTED_SPEEDOMETER_DIAL_VISIBLE = "MutedSpeedometerDialVisible";
const char* IS_OPTION_2D_ENABLED = "IsOption2DEnabled";
const char* IS_OPTION_3D_ENABLED = "IsOption3DEnabled";
const char* IS_OPTION_UNITS_ENABLED = "IsOptionUnitsEnabled";
const char* IS_OPTION_FEETPERCENT_ENABLED = "IsOptionFeetPercentEnabled";
const char* IS_OPTION_BEETLEPERCENT_ENABLED = "IsOptionBeetlePercentEnabled";
const char* NEEDLE_AMPLITUDE_UNITS = "NeedleAmplitudeUnits";
const char* NEEDLE_AMPLITUDE_FEETPERCENT = "NeedleAmplitudeFeetPercent";
const char* NEEDLE_AMPLITUDE_BEETLEPERCENT = "NeedleAmplitudeBeetlePercent";
const char* SPEEDOMETER_DIAL_POSITION_H = "SpeedometerDialPositionH";
const char* SPEEDOMETER_DIAL_POSITION_V = "SpeedometerDialPositionV";
const char* SPEEDOMETER_DIAL_SCALE = "SpeedometerDialScale";
const char* MUTED_SPEEDOMETER_FONT_SIZE = "MutedFontSize";
const char* TIMER_AUX_FONT_SIZE = "AuxWindowFontSize";
const char* COORDINATE_EDITOR_FONT_SIZE = "CoordEditFontSize";
const char* IS_SPEEDOMETER_TIMER_VISIBLE = "IsSpeedometerTimerVisible";
const char* START_FADING_DISTANCE = "StartFadingDistance";
const char* FINISH_FADING_DISTANCE = "FinishFadingDistance";
const char* SELF_FADING_DISTANCE = "SelfFadingDistance";
const char* CIRCLE_STYLE_FLAT = "CircleStyleFlat";
const char* CIRCLE_STYLE_ARC = "CircleStyleArc";
const char* CIRCLE_STYLE_RING = "CircleStyleRing";
const char* SPEEDOMETER_CIRCLE_OFFSET = "SpeedometerCircleOffset";
const char* TIMER_AUX_RIGHT_ALIGNED = "TimerAuxRightAligned";
const char* TIMER_AUX_LEFT_ALIGNED = "TimerAuxLeftAligned";
const char* TIMER_AUX_TOP_ALIGNED = "TimerAuxTopAligned";
const char* TIMER_AUX_BOTTOM_ALIGNED = "TimerAuxBottomAligned";
const char* IS_OPTION_PAUSE_ENABLED = "IsOptionPauseEnabled";
const char* IS_OPTION_STOP_ENABLED = "IsOptionStopEnabled";
const char* MANUAL_START_DIAMETER_UNITS = "ManualStartDiameterUnits";
const char* IS_OPTION_MANUAL_ENABLED = "IsOptionManualEnabled";
const char* IS_OPTION_PREDEFINED_ENABLED = "IsOptionPredefinedEnabled";
const char* IS_OPTION_CUSTOM_ENABLED = "IsOptionCustomEnabled";
const char* IS_OPTION_EDITOR_ENABLED = "IsOptionEditorEnabled";
const char* START_DIAMETER_UNITS = "StartDiameterUnits";
const char* FINISH_DIAMETER_UNITS = "FinishDiameterUnits";
const char* PREDEF_TIMER_SET = "PredefTimerSet";
const char* START_X_OFFSET = "StartXOffset";
const char* START_Z_OFFSET = "StartZOffset";
const char* START_Y_OFFSET = "StartYOffset";
const char* FINISH_X_OFFSET = "FinishXOffset";
const char* FINISH_Z_OFFSET = "FinishZOffset";
const char* FINISH_Y_OFFSET = "FinishYOffset";
const char* SPEEDOMETER_TIMER_POSITION_H = "SpeedometerTimerPositionH";
const char* SPEEDOMETER_TIMER_POSITION_V = "SpeedometerTimerPositionV";
const char* SPEEDOMETER_TIMER_SCALE = "SpeedometerTimerScale";
const char* RESTORING_DEFAULTS = "RestoringDefaults";

namespace Settings
{
	std::mutex	Mutex;
	json		Settings = json::object();

	void Load(std::filesystem::path aPath)
	{
		if (!std::filesystem::exists(aPath)) { return; }

		Settings::Mutex.lock();
		{
			try
			{
				std::ifstream file(aPath);
				Settings = json::parse(file);
				file.close();
			}
			catch (json::parse_error& ex)
			{
				APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Settings.json could not be parsed.");
				APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", ex.what());
				APIDefs->Log(ELogLevel_DEBUG, "Simple Speedometer", "I am no longer <c=#ff0000>speed</c>.");
			}
		}
		Settings::Mutex.unlock();

		if (!Settings[IS_READ_ME_VISIBLE].is_null())
		{
			Settings[IS_READ_ME_VISIBLE].get_to<bool>(IsReadMeEnabled);
		}
		if (!Settings[IS_SPEEDOMETER_DIAL_VISIBLE].is_null())
		{
			Settings[IS_SPEEDOMETER_DIAL_VISIBLE].get_to<bool>(IsDialEnabled);
		}
		if (!Settings[IS_SPEEDOMETER_TABLE_VISIBLE].is_null())
		{
			Settings[IS_SPEEDOMETER_TABLE_VISIBLE].get_to<bool>(IsTableEnabled);
		}
		if (!Settings[FANCY_SPEEDOMETER_DIAL_VISIBLE].is_null())
		{
			Settings[FANCY_SPEEDOMETER_DIAL_VISIBLE].get_to<bool>(FancyDialEnabled);
		}
		if (!Settings[MUTED_SPEEDOMETER_DIAL_VISIBLE].is_null())
		{
			Settings[MUTED_SPEEDOMETER_DIAL_VISIBLE].get_to<bool>(MutedDialEnabled);
		}
		if (!Settings[SPEEDOMETER_VISIBLE_ALWAYS].is_null())
		{
			Settings[SPEEDOMETER_VISIBLE_ALWAYS].get_to<bool>(optionSpeedoVisAlways);
		}
		if (!Settings[SPEEDOMETER_VISIBLE_ON_FOOT].is_null())
		{
			Settings[SPEEDOMETER_VISIBLE_ON_FOOT].get_to<bool>(optionSpeedoVisFoot);
		}
		if (!Settings[SPEEDOMETER_VISIBLE_MOUNTED].is_null())
		{
			Settings[SPEEDOMETER_VISIBLE_MOUNTED].get_to<bool>(optionSpeedoVisMount);
		}
		if (!Settings[IS_OPTION_2D_ENABLED].is_null())
		{
			Settings[IS_OPTION_2D_ENABLED].get_to<bool>(option2D);
		}
		if (!Settings[IS_OPTION_3D_ENABLED].is_null())
		{
			Settings[IS_OPTION_3D_ENABLED].get_to<bool>(option3D);
		}
		if (!Settings[IS_OPTION_UNITS_ENABLED].is_null())
		{
			Settings[IS_OPTION_UNITS_ENABLED].get_to<bool>(optionUnits);
		}
		if (!Settings[IS_OPTION_FEETPERCENT_ENABLED].is_null())
		{
			Settings[IS_OPTION_FEETPERCENT_ENABLED].get_to<bool>(optionFeetPercent);
		}
		if (!Settings[IS_OPTION_BEETLEPERCENT_ENABLED].is_null())
		{
			Settings[IS_OPTION_BEETLEPERCENT_ENABLED].get_to<bool>(optionBeetlePercent);
		}
		if (!Settings[NEEDLE_AMPLITUDE_UNITS].is_null())
		{
			Settings[NEEDLE_AMPLITUDE_UNITS].get_to<float>(amplitudeUnits);
		}
		if (!Settings[NEEDLE_AMPLITUDE_FEETPERCENT].is_null())
		{
			Settings[NEEDLE_AMPLITUDE_FEETPERCENT].get_to<float>(amplitudeFeetPercent);
		}
		if (!Settings[NEEDLE_AMPLITUDE_BEETLEPERCENT].is_null())
		{
			Settings[NEEDLE_AMPLITUDE_BEETLEPERCENT].get_to<float>(amplitudeBeetlePercent);
		}
		if (!Settings[SPEEDOMETER_DIAL_POSITION_H].is_null())
		{
			Settings[SPEEDOMETER_DIAL_POSITION_H].get_to<float>(DialPositionH);
		}
		if (!Settings[SPEEDOMETER_DIAL_POSITION_V].is_null())
		{
			Settings[SPEEDOMETER_DIAL_POSITION_V].get_to<float>(DialPositionV);
		}
		if (!Settings[SPEEDOMETER_DIAL_SCALE].is_null())
		{
			Settings[SPEEDOMETER_DIAL_SCALE].get_to<float>(DialScale);
		}
		if (!Settings[MUTED_SPEEDOMETER_FONT_SIZE].is_null())
		{
			Settings[MUTED_SPEEDOMETER_FONT_SIZE].get_to<float>(MutedFontSize);
		}
		if (!Settings[TIMER_AUX_FONT_SIZE].is_null())
		{
			Settings[TIMER_AUX_FONT_SIZE].get_to<float>(AuxWindowFontSize);
		}
		if (!Settings[COORDINATE_EDITOR_FONT_SIZE].is_null())
		{
			Settings[COORDINATE_EDITOR_FONT_SIZE].get_to<float>(CoordEditFontSize);
		}
		if (!Settings[IS_SPEEDOMETER_TIMER_VISIBLE].is_null())
		{
			Settings[IS_SPEEDOMETER_TIMER_VISIBLE].get_to<bool>(IsTimerEnabled);
		}
		if (!Settings[START_FADING_DISTANCE].is_null())
		{
			Settings[START_FADING_DISTANCE].get_to<float>(startFadingDistance);
		}
		if (!Settings[SELF_FADING_DISTANCE].is_null())
		{
			Settings[SELF_FADING_DISTANCE].get_to<float>(selfFadingDistance);
		}
		if (!Settings[FINISH_FADING_DISTANCE].is_null())
		{
			Settings[FINISH_FADING_DISTANCE].get_to<float>(finishFadingDistance);
		}
		if (!Settings[CIRCLE_STYLE_FLAT].is_null())
		{
			Settings[CIRCLE_STYLE_FLAT].get_to<bool>(optionFlat);
		}
		if (!Settings[CIRCLE_STYLE_ARC].is_null())
		{
			Settings[CIRCLE_STYLE_ARC].get_to<bool>(optionArc);
		}
		if (!Settings[CIRCLE_STYLE_RING].is_null())
		{
			Settings[CIRCLE_STYLE_RING].get_to<bool>(optionRing);
		}
		if (!Settings[SPEEDOMETER_CIRCLE_OFFSET].is_null())
		{
			Settings[SPEEDOMETER_CIRCLE_OFFSET].get_to<float>(CircleOffset);
		}
		if (!Settings[TIMER_AUX_RIGHT_ALIGNED].is_null())
		{
			Settings[TIMER_AUX_RIGHT_ALIGNED].get_to<bool>(optionTimerRight);
		}
		if (!Settings[TIMER_AUX_LEFT_ALIGNED].is_null())
		{
			Settings[TIMER_AUX_LEFT_ALIGNED].get_to<bool>(optionTimerLeft);
		}
		if (!Settings[TIMER_AUX_TOP_ALIGNED].is_null())
		{
			Settings[TIMER_AUX_TOP_ALIGNED].get_to<bool>(optionTimerTop);
		}
		if (!Settings[TIMER_AUX_BOTTOM_ALIGNED].is_null())
		{
			Settings[TIMER_AUX_BOTTOM_ALIGNED].get_to<bool>(optionTimerBottom);
		}
		if (!Settings[IS_OPTION_PAUSE_ENABLED].is_null())
		{
			Settings[IS_OPTION_PAUSE_ENABLED].get_to<bool>(optionPause);
		}
		if (!Settings[IS_OPTION_STOP_ENABLED].is_null())
		{
			Settings[IS_OPTION_STOP_ENABLED].get_to<bool>(optionStop);
		}
		if (!Settings[MANUAL_START_DIAMETER_UNITS].is_null())
		{
			Settings[MANUAL_START_DIAMETER_UNITS].get_to<float>(manualstartDiameter);
		}
		if (!Settings[IS_OPTION_MANUAL_ENABLED].is_null())
		{
			Settings[IS_OPTION_MANUAL_ENABLED].get_to<bool>(optionManual);
		}
		if (!Settings[IS_OPTION_PREDEFINED_ENABLED].is_null())
		{
			Settings[IS_OPTION_PREDEFINED_ENABLED].get_to<bool>(optionPredefined);
		}
		if (!Settings[IS_OPTION_CUSTOM_ENABLED].is_null())
		{
			Settings[IS_OPTION_CUSTOM_ENABLED].get_to<bool>(optionCustom);
		}
		if (!Settings[IS_OPTION_EDITOR_ENABLED].is_null())
		{
			Settings[IS_OPTION_EDITOR_ENABLED].get_to<bool>(optionEditor);
		}
		if (!Settings[START_DIAMETER_UNITS].is_null())
		{
			Settings[START_DIAMETER_UNITS].get_to<float>(startDiameter);
		}
		if (!Settings[FINISH_DIAMETER_UNITS].is_null())
		{
			Settings[FINISH_DIAMETER_UNITS].get_to<float>(finishDiameter);
		}
		if (!Settings[PREDEF_TIMER_SET].is_null())
		{
			Settings[PREDEF_TIMER_SET].get_to<int>(selectedPredefSet);
		}
		if (!Settings[SPEEDOMETER_TIMER_POSITION_H].is_null())
		{
			Settings[SPEEDOMETER_TIMER_POSITION_H].get_to<float>(TimerPositionH);
		}
		if (!Settings[SPEEDOMETER_TIMER_POSITION_V].is_null())
		{
			Settings[SPEEDOMETER_TIMER_POSITION_V].get_to<float>(TimerPositionV);
		}
		if (!Settings[SPEEDOMETER_TIMER_SCALE].is_null())
		{
			Settings[SPEEDOMETER_TIMER_SCALE].get_to<float>(TimerScale);
		}
		if (!Settings[RESTORING_DEFAULTS].is_null())
		{
			Settings[RESTORING_DEFAULTS].get_to<bool>(restoreDefaults);
		}
	}

	void Save(std::filesystem::path aPath)
	{
		Settings::Mutex.lock();
		{
			std::ofstream file(aPath);
			file << Settings.dump(1, '\t') << std::endl;
			file.close();
		}
		Settings::Mutex.unlock();
	}

	/*
	void DoSomething(const char* aIdentifier, bool aIsRelease)
	{
		if ((strcmp(aIdentifier, "KB_DO_SOMETHING") == 0) && !aIsRelease)
		{
	
		}
	}
	*/

	bool IsReadMeEnabled = false;
	float ReadMePositionH = 450.0f;
	float ReadMePositionV = 250.0f;
	bool IsDialEnabled = true;
	bool IsTableEnabled = false;
	bool optionSpeedoVisAlways = true;
	bool optionSpeedoVisFoot = false;
	bool optionSpeedoVisMount = false;
	bool FancyDialEnabled = true;
	bool MutedDialEnabled = false;
	bool option2D = true;
	bool option3D = false;
	bool optionUnits = true;
	bool optionFeetPercent = false;
	bool optionBeetlePercent = false;
	float amplitudeUnits = 391.0f;
	float amplitudeFeetPercent = 100.0f;
	float amplitudeBeetlePercent = 99.0f;
	float DialPositionH = 320.0f;
	float DialPositionV = 300.0f;
	float DialScale = 60.0f;
	float MutedFontSize = 100.0f;
	float AuxWindowFontSize = 100.0f;
	float CoordEditFontSize = 100.0f;
	bool IsTimerEnabled = true;
	float startFadingDistance = 2500.0f;
	float finishFadingDistance = 2500.0f;
	float selfFadingDistance = 200.0f;
	bool optionFlat = true;
	bool optionArc = false;
	bool optionRing = false;
	float CircleOffset = 0.0f;
	bool optionPause = true;
	bool optionStop = false;
	bool optionTimerRight = true;
	bool optionTimerLeft = false;
	bool optionTimerTop = false;
	bool optionTimerBottom = true;
	float manualstartDiameter = 18.0f;
	bool optionManual = true;
	bool optionPredefined = false;
	bool optionCustom = false;
	bool optionEditor = false;
	float startDiameter = 100.0f;
	float finishDiameter = 200.0f;
	int selectedPredefSet = 0;
	float TimerPositionH = 260.0f;
	float TimerPositionV = 560.0f;
	float TimerScale = 60.0f;
	bool restoreDefaults = false;
}