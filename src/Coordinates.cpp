#include "Coordinates.h"
#include "Settings.h"
#include "shared.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include "httpclient.hpp"

const char* TEST_TEST_DING_DONG = "TestTestDingDong";

namespace Coordinates
{
    std::mutex Mutex;
    std::unordered_map<std::string, CoordinateSet> CoordinateSets;
    std::vector<std::string> SetNames;

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

    // Updating the filered list of names based on Map ID with either internal or external data
    void UpdateFilteredSetNames(int currentMapID)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        FilteredSetNames.clear();

        if (!SetNames.empty())
        {
            for (const auto& name : SetNames)
            {
                if (CoordinateSets.at(name).MapID == currentMapID)
                    FilteredSetNames.push_back(name);
            }
        }

        if (FilteredSetNames.empty())
        {
            for (const auto& name : InternalSetNames)
            {
                if (InternalCoordinateSets.at(name).MapID == currentMapID)
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

    void LoadCloudConfig() {
        APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Loading Cloud Config.");
        std::string url = "https://speedometer.cloudflare8462.workers.dev/api/config/" + Settings::cloudConfigID;
        const char* cUrl = url.c_str();
        APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", cUrl);
        std::wstring wUrl(cUrl, cUrl + strlen(cUrl));
        const std::string response = HTTPClient::GetRequest(wUrl.c_str());
        try {
            nlohmann::ordered_json cloudJson = nlohmann::ordered_json::parse(response, nullptr, false);
            CoordinateSets.clear();
            SetNames.clear();

            for (auto& [key, value] : cloudJson["Sets"].items())
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
        catch (std::exception& ex)
        {
            APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Error parsing cloud config");
        }
        APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Cloud Config loaded");
    }


    void Load(const std::filesystem::path& aPath)
    {
        std::lock_guard<std::mutex> lock(Mutex);
        APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "Loading File Config.");
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

        try
        {
            std::ifstream file(aPath);
            nlohmann::ordered_json j;
            file >> j;
            file.close();

            CoordinateSets.clear();
            SetNames.clear();

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

    bool TestedTheDingDong = true;
}
