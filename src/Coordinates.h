#ifndef COORDINATES_H
#define COORDINATES_H

#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include "nlohmann/json.hpp"

using ordered_json = nlohmann::ordered_json;

namespace Coordinates
{
	struct Vector3
	{
		float x, y, z;
	};

	struct Checkpoint
	{
		Vector3 Position;
		float Radius;
	};

	struct CoordinateSet
	{
		Vector3 Start;
		Vector3 End;
		float StartRadius;
		float EndRadius;
		int MapID;
		std::vector<Checkpoint> Checkpoints;
	};

	extern std::mutex Mutex;
	extern std::unordered_map<std::string, CoordinateSet> CoordinateSets;
	extern std::vector<std::string> SetNames;

	extern std::unordered_map<std::string, CoordinateSet> InternalCoordinateSets;
	extern std::vector<std::string> InternalSetNames;

	extern std::vector<std::string> FilteredSetNames;

	void Load(const std::filesystem::path& aPath);

	void LoadCloudConfig();

	void UpdateFilteredSetNames(int currentMapID);

	const CoordinateSet* GetSelectedCoordinateSet();
}

#endif