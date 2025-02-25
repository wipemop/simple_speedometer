#include "Settings.h"
#include "shared.h"
#include <filesystem>
#include <fstream>

const char* IS_READ_ME_VISIBLE = "IsReadMeVisible";
const char* READ_ME_POSITION_H = "ReadMePositionH";
const char* READ_ME_POSITION_V = "ReadMePositionV";
const char* IS_SPEEDOMETER_DIAL_VISIBLE = "IsSpeedometerDialVisible";
const char* IS_SPEEDOMETER_TABLE_VISIBLE = "IsSpeedometerTableVisible";
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
const char* IS_SPEEDOMETER_TIMER_VISIBLE = "IsSpeedometerTimerVisible";
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
				APIDefs->Log(ELogLevel_WARNING, "Compass", "Settings.json could not be parsed.");
				APIDefs->Log(ELogLevel_WARNING, "Compass", ex.what());
				APIDefs->Log(ELogLevel_DEBUG, "My First addon", "<c=#ff0000>Signing off</c>, it was an honor commander.");
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
		if (!Settings[IS_SPEEDOMETER_TIMER_VISIBLE].is_null())
		{
			Settings[IS_SPEEDOMETER_TIMER_VISIBLE].get_to<bool>(IsTimerEnabled);
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

	//void DoSomething(const char* aIdentifier, bool aIsRelease)
	//{
	//	if ((strcmp(aIdentifier, "KB_DO_SOMETHING") == 0) && !aIsRelease)
	//	{
	//
	//	}
	//}

	bool IsReadMeEnabled = false;
	float ReadMePositionH = 450.0f;
	float ReadMePositionV = 250.0f;
	bool IsDialEnabled = true;
	bool IsTableEnabled = false;
	bool option2D = true;
	bool option3D = false;
	bool optionUnits = true;
	bool optionFeetPercent = false;
	bool optionBeetlePercent = false;
	float amplitudeUnits = 391.0f;
	float amplitudeFeetPercent = 100.0f;
	float amplitudeBeetlePercent = 99.0f;
	float DialPositionH = 250.0f;
	float DialPositionV = 250.0f;
	float DialScale = 60.0f;
	bool IsTimerEnabled = true;
	float TimerPositionH = 250.0f;
	float TimerPositionV = 100.0f;
	float TimerScale = 60.0f;
	bool restoreDefaults = false;
}