#include "Coordinates.h"
#include "Settings.h"
#include "shared.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace Coordinates
{
    std::mutex Mutex;
    std::unordered_map<std::string, CoordinateSet> CoordinateSets;
    std::vector<std::string> SetNames;

    ordered_json CoordinateData = ordered_json::object();

    // Internal coordinate sets used to provide the user with valid data if the coordinates.json was missing or invalid.
    std::unordered_map<std::string, CoordinateSet> InternalCoordinateSets = {
        {"Homestead - Waypoint to Garden", {
            {117.5147f, 15.6019f, -49.4975f},  // Start
            {98.5192f, 16.7772f, -62.2833f},   // End
            100.0f,  // Start-Radius
            200.0f,  // End-Radius
            1558,    // MapID
            {   // Checkpoints
                {{104.1501f, 15.2401f, -41.4597f}, 150.0f},
                {{133.8676f, 16.8978f, -63.1641f}, 150.0f},
                {{114.5097f, 23.8542f, -95.9121f}, 150.0f}
            }
        }},
        {"Caledon Forest - Morgan's Spiral", {
            {474.8509f, 0.5672f, -709.2817f},   // Start
            {442.0647f, 45.0707f, -626.8521f},    // End
            150.0f,  // Start-Radius
            170.0f,  // End-Radius
            34,      // MapID
            {   // Checkpoints
                {{465.1118f, 19.7697f, -684.5453f}, 71.0f},
                {{467.1163f, 23.4168f, -723.1299f}, 71.0f},
                {{468.1608f, 42.2680f, -715.9031f}, 71.0f},
                {{446.3679f, 46.7695f, -675.4959f}, 71.0f}
            }
        }},
        {"Homestead - Waypoint to other Garden", {
            {117.5147f, 15.6019f, -49.4975f},  // Start
            {135.6939f, 16.8608f, -84.2182f},  // End
            150.0f,  // Start-Radius
            300.0f,  // End-Radius
            1558,    // MapID
            {   // Checkpoints
                {{104.1501f, 15.2401f, -41.4597f}, 150.0f},
                {{99.5831f, 16.7137f, -83.3961f}, 150.0f}
            }
        }}
    };

    // Creating the set names from internal data
    std::vector<std::string> GetInternalSetNames()
    {
        std::vector<std::string> names;
        for (const auto& pair : InternalCoordinateSets)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    std::vector<std::string> InternalSetNames = GetInternalSetNames();

    std::vector<std::string> FilteredSetNames;

    // Filtering by Map Id has a flaw, in that guild halls have different IDs for the different playercount thresholds even though they're geometrically the same. These can be group and assigned to a custom ID in order to make Coordinate Sets creeated in one of them available to all of them.
    const std::unordered_map<int, int> MAP_ID_GROUPS =
    {
    // Gilded Hollow map ID group
    {1068, 99991},
    {1101, 99991},
    {1107, 99991},
    {1108, 99991},
    {1121, 99991},

    // Gilded Hollow map ID group
    {1069, 99992},
    {1071, 99992},
    {1076, 99992},
    {1104, 99992},
    {1124, 99992},

    // Windswept Haven map ID group
    {1214, 99993}, 
    {1215, 99993},
    {1224, 99993},
    {1232, 99993},
    {1243, 99993},
    {1250, 99993},

    // Isle of Reflection map ID group
    {1419, 99994},
    {1426, 99994},
    {1435, 99994},
    {1444, 99994},
    {1462, 99994}
    };

    // Checking if the Map ID is part of a group and returning the group ID if that is the case. Otherwise keep the Map ID
    int GetGroupMapID(int mapID)
    {
        auto it = MAP_ID_GROUPS.find(mapID);
        if (it != MAP_ID_GROUPS.end())
            return it->second;
        return mapID;
    }

    // Updating the filered list of names based on Map ID or group ID with either internal or external data
    void UpdateFilteredSetNames(int currentMapID)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        FilteredSetNames.clear();

        int groupedMapID = GetGroupMapID(currentMapID);

        if (!SetNames.empty())
        {
            for (const auto& name : SetNames)
            {
                if (GetGroupMapID(CoordinateSets.at(name).MapID) == groupedMapID)
                    FilteredSetNames.push_back(name);
            }
        }

        if (FilteredSetNames.empty())
        {
            for (const auto& name : InternalSetNames)
            {
                if (GetGroupMapID(InternalCoordinateSets.at(name).MapID) == groupedMapID)
                    FilteredSetNames.push_back(name);
            }
        }
    }

    const CoordinateSet* GetSelectedCoordinateSet()
    {
        std::lock_guard<std::mutex> lock(Mutex);
        if (FilteredSetNames.empty() || Settings::selectedPredefSet >= FilteredSetNames.size())
            return nullptr;

        const std::string& selectedName = FilteredSetNames[Settings::selectedPredefSet];
        auto it = CoordinateSets.find(selectedName);
        if (it != CoordinateSets.end())
            return &it->second;
        auto itInternal = InternalCoordinateSets.find(selectedName);
        if (itInternal != InternalCoordinateSets.end())
            return &itInternal->second;
        return nullptr;
    }

    // Load function to read the coordinates.json, it will also handle the creation of a coordinates.json or example_coordinates.json if it was not found or was found to be corrupted
    void Load(const std::filesystem::path& aPath)
    {
        // This data block is used to recreate the coordinates.json when it was missing, or the example_coordinates.json if the coordinates.json was invalid.
        nlohmann::ordered_json exampleJson = {
            {"Sets", {
                {"Homestead - Waypoint to Garden", {
                    {"MapID", 1558},
                    {"Start", { {"x", 117.5147}, {"y", 15.6019}, {"z", -49.4975} }},
                    {"Startradius", 100.0},
                    {"Checkpoints", {
                        { {"Position", {{"x", 104.1501}, {"y", 15.2401}, {"z", -41.4597}}}, {"Radius", 150.0} },
                        { {"Position", {{"x", 133.8676}, {"y", 16.8978}, {"z", -63.1641}}}, {"Radius", 150.0} },
                        { {"Position", {{"x", 114.5097}, {"y", 23.8542}, {"z", -95.9121}}}, {"Radius", 150.0} }
                    }},
                    {"End", { {"x", 98.5192}, {"y", 16.7772}, {"z", -62.2833} }},
                    {"Endradius", 200.0}
                }},
                {"Homestead - Waypoint to other Garden", {
                    {"MapID", 1558},
                    {"Start", { {"x", 117.5147}, {"y", 15.6019}, {"z", -49.4975} }},
                    {"Startradius", 150.0},
                    {"Checkpoints", {
                        { {"Position", {{"x", 104.1501}, {"y", 15.2401}, {"z", -41.4597}}}, {"Radius", 150.0} },
                        { {"Position", {{"x", 99.5831}, {"y", 16.7137}, {"z", -83.3961}}}, {"Radius", 150.0} }
                    }},
                    {"End", { {"x", 135.6939}, {"y", 16.8608}, {"z", -84.2182} }},
                    {"Endradius", 300.0}
                }},
                {"Morgan's Spiral", {
                    {"MapID", 34},
                    {"Start", { {"x", 474.8509}, {"y", 0.5672}, {"z", -709.2817} }},
                    {"Startradius", 150.0},
                    {"Checkpoints", {
                        { {"Position", {{"x", 465.1118}, {"y", 19.7697}, {"z", -684.5453}}}, {"Radius", 71.0} },
                        { {"Position", {{"x", 467.1163}, {"y", 23.4168}, {"z", -723.1299}}}, {"Radius", 71.0} },
                        { {"Position", {{"x", 468.1608}, {"y", 42.2680}, {"z", -715.9031}}}, {"Radius", 71.0} },
                        { {"Position", {{"x", 446.3679}, {"y", 46.7695}, {"z", -675.4959}}}, {"Radius", 71.0} }
                    }},
                    {"End", { {"x", 442.0647}, {"y", 45.0707}, {"z", -626.8521} }},
                    {"Endradius", 170.0}
                }}
            }}
        };

        // Check for exisiting coordinates.json
        if (!std::filesystem::exists(aPath))
        {
            APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Missing coordinates.json! Creating coordinates.json with internal data.");
            std::ofstream exampleFile(APIDefs->Paths.GetAddonDirectory("Simple Speedometer/coordinates.json"));
            if (exampleFile.is_open())
            {
                exampleFile << exampleJson.dump(4);
                exampleFile.close();
            }
            return;
        }

        std::lock_guard<std::mutex> lock(Mutex);

        // Attempting to read
        try
        {
            std::ifstream file(aPath);
            nlohmann::ordered_json j;
            file >> j;
            file.close();

            CoordinateSets.clear();
            SetNames.clear();

            // If the "Sets" data block is damaged for example due to missing brackets, create example_coordinates.json
            if (!j.contains("Sets") || !j["Sets"].is_object())
            {
                APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Invalid format in coordinates.json!");
                std::ofstream exampleFile(APIDefs->Paths.GetAddonDirectory("Simple Speedometer/example_coordinates.json"));
                if (exampleFile.is_open())
                {
                    exampleFile << exampleJson.dump(4);
                    exampleFile.close();
                }
                return;
            }

            // Reading the json data
            for (auto& [key, value] : j["Sets"].items())
            {
                CoordinateSet set{};

                if (value.contains("Start") && value["Start"].is_object())
                {
                    set.Start.x = value["Start"].value("x", 0.0f);
                    set.Start.y = value["Start"].value("y", 0.0f);
                    set.Start.z = value["Start"].value("z", 0.0f);
                }

                if (value.contains("End") && value["End"].is_object())
                {
                    set.End.x = value["End"].value("x", 0.0f);
                    set.End.y = value["End"].value("y", 0.0f);
                    set.End.z = value["End"].value("z", 0.0f);
                }

                set.StartRadius = value.value("Startradius", 0.0f);
                set.EndRadius = value.value("Endradius", 0.0f);
                set.MapID = value.value("MapID", 0);

                if (value.contains("Checkpoints") && value["Checkpoints"].is_array())
                {
                    for (const auto& checkpoint : value["Checkpoints"])
                    {
                        Checkpoint cp{};
                        if (checkpoint.contains("Position") && checkpoint["Position"].is_object())
                        {
                            cp.Position.x = checkpoint["Position"].value("x", 0.0f);
                            cp.Position.y = checkpoint["Position"].value("y", 0.0f);
                            cp.Position.z = checkpoint["Position"].value("z", 0.0f);
                        }
                        cp.Radius = checkpoint.value("Radius", 0.0f);
                        set.Checkpoints.push_back(cp);
                    }
                }

                CoordinateSets[key] = set;
                SetNames.push_back(key);
            }
        }

        // Catching parsing issues and creating example_coordinates.json if parsing errors occured
        catch (nlohmann::json::parse_error& ex)
        {
            APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Error parsing coordinates.json!");
            std::ofstream exampleFile(APIDefs->Paths.GetAddonDirectory("Simple Speedometer/example_coordinates.json"));
            if (exampleFile.is_open())
            {
                exampleFile << exampleJson.dump(4);
                exampleFile.close();
            }
        }
    }

    // Save function, possibly oprhaned? It lives here for now until i learn to properly split up my code into different cpp's, my entry.cpp is stuffed to the brim and really needs a code diet
    void Save(const std::filesystem::path& aPath)
    {
        std::lock_guard<std::mutex> lock(Coordinates::Mutex);

        ordered_json coordinateData;
        if (std::filesystem::exists(aPath))
        {
            std::ifstream file(aPath);
            try
            {
                file >> coordinateData;
            }
            catch (const std::exception&)
            {
                coordinateData = ordered_json::object();
            }
            file.close();
        }

        if (!coordinateData.contains("Sets") || !coordinateData["Sets"].is_object())
        {
            coordinateData["Sets"] = ordered_json::object();
        }

        for (const auto& [name, set] : Coordinates::CoordinateSets)
        {
            ordered_json setData = ordered_json::object();

            setData["MapID"] = set.MapID;

            auto round4 = [](double value) {
                return std::round(value * 10000.0) / 10000.0;
                };

            setData["Start"] = {
                { "x", round4(set.Start.x) },
                { "y", round4(set.Start.y) },
                { "z", round4(set.Start.z) }
            };

            setData["Startradius"] = round4(set.StartRadius);

            ordered_json checkpoints = ordered_json::array();
            for (const auto& checkpoint : set.Checkpoints)
            {
                ordered_json checkpointData = ordered_json::object();
                checkpointData["Position"] = {
                    { "x", round4(checkpoint.Position.x) },
                    { "y", round4(checkpoint.Position.y) },
                    { "z", round4(checkpoint.Position.z) }
                };
                checkpointData["Radius"] = round4(checkpoint.Radius);
                checkpoints.push_back(checkpointData);
            }
            setData["Checkpoints"] = checkpoints;

            setData["End"] = {
                { "x", round4(set.End.x) },
                { "y", round4(set.End.y) },
                { "z", round4(set.End.z) }
            };

            setData["Endradius"] = round4(set.EndRadius);

            coordinateData["Sets"][name] = setData;
        }

        std::ofstream outFile(aPath);
        outFile << coordinateData.dump(1, '\t') << std::endl;
        outFile.close();
    }
}
