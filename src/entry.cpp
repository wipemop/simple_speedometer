// DISCLAIMER: I don't have the slightest clue about C++. This was built with my own ideas, but almost entirely with the help of ChatGPT, using the Raidcore / Nexus Addon-Template as a foundation.
// As such, the code will likely be inefficient and generally not pretty. I tried my best optimizing obvious drawbacks in structure with my very limited expertise, but it does get the job done.

#include <Windows.h>
#include <string>
#include <stdio.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <filesystem>
#include <mutex>
#include <fstream>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "imgui/imgui.h"

#include "shared.h"
#include "settings.h"
#include "Coordinates.h"
#include "resource.h"

/* proto */
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();

void InitializeHighResTimer();

void SpeedometerToggleVisibility(const char* aIdentifier, bool aIsRelease);
void TimerToggleVisibility(const char* aIdentifier, bool aIsRelease);
void TimerPauseReset(const char* aIdentifier, bool aIsRelease);

void ReceiveFont(const char* aIdentifier, void* aFont);

/* globals */
AddonDefinition AddonDef = {};
HMODULE hSelf = nullptr;

std::filesystem::path AddonPath;
std::filesystem::path SettingsPath;
std::filesystem::path CoordinatesPath;

static Vector3 prevPos = { 0.0f, 0.0f, 0.0f };
static ULONGLONG prevTime = 0;
static int zeroCount3D = 0;
static int zeroCount2D = 0;
static float speed3D = 0.0f;
static float speed2D = 0.0f;
static float lastSpeed3D = 0.0f;
static float lastSpeed2D = 0.0f;
static ULONGLONG lastMumbleUpdate = 0;

const float conversionFactor_u_s = 39.36982f;
const float conversionFactor_foot = 13.39111f;
const float conversionFactor_beetle = 2.16507f;

static float imageAlpha = 1.0f;
static float timerimageAlpha = 1.0f;
const float fadeSpeed = 0.3f;

static bool fireAnimationTriggered = false;
static bool fireAnimationFinished = false;
static int fireFrame = 0;
static double fireNextFrameTime = 0.0;

static bool pivotAnimationTriggered = false;
static bool pivotAnimationFinished = false;
static int pivotFrame = 0;
static double pivotNextFrameTime = 0.0;

static bool timerAnimationTriggered = false;
static bool timerAnimationFinished = false;
static int timerFrame = 0;
static int timerFramePaused = 0;
static double timerNextFrameTime = 0.0;

ImVec2 TimerPos = ImVec2(0.0f, 0.0f);
static bool timerRunning = false;
static bool timerPaused = false;
static double pausedElapsedSeconds = 0;
static double currElapsed = 0;
static LARGE_INTEGER timerStart = { 0 };
static LARGE_INTEGER frequency = { 0 };
static bool frequencyInitialized = false;
static Vector3 basePos = { -10000.0f, -10000.0f, -10000.0f };
static bool basePosInitialized = false;
static Vector3 pausePos = { -10000.0f, -10000.0f, -10000.0f };
static bool pausePosInitialized = false;
static LARGE_INTEGER current = { 0 };
static Vector3 currentPos = { 0.0f, 0.0f, 0.0f };
float g_basedistance = 0.0f;
Vector3 g_currentPos = { 0.0f, 0.0f, 0.0f };
float g_pausedistance = 0.0f;
static bool timerStartSet = false;
static bool timerFinishSet = false;
static bool timerStartEntered = false;
static int timerHotkeyCount = 4;
float g_circlethickness = 0.0f;

float startRadius = 0.0f;
float endRadius = 0.0f;
float checkpointRadius = 0.0f;
float nextcheckpointRadius = 0.0f;
int checkpointcount = 0;
static bool timerCheckpointEntered = false;
float g_checkpointdistance = 0.0f;

int selectedIndex = 0;
bool wasJsonMissing = true;
bool useExternalSets = false;
int currentCheckpointIndex = 0;
Vector3 checkpointPos = { 0.0f, 0.0f, 0.0f };
Vector3 nextcheckpointPos = { 0.0f, 0.0f, 0.0f };
static bool checkpointlist = false;
static bool checkpointlistfinished = false;

int currentMapID = 0;
static int lastMapID = -1;
static std::vector<std::string> filteredSetNames;

void* fontptr;


/* Textures */
Texture* speedometerTexture = nullptr;
Texture* needleTexture = nullptr;
Texture* blueneedleTexture = nullptr;
Texture* numberTexture = nullptr;
Texture* indicatorTexture = nullptr;
Texture* fireTexture = nullptr;
Texture* pivotTexture = nullptr;
Texture* timerTexture = nullptr;
Texture* stopwatchTexture = nullptr;
Texture* timerindicatorTexture = nullptr;

extern Texture* speedometerTexture;
extern Texture* needleTexture;
extern Texture* blueneedleTexture;
extern Texture* numberTexture;
extern Texture* indicatorTexture;
extern Texture* fireTexture;
extern Texture* pivotTexture;
extern Texture* timerTexture;
extern Texture* stopwatchTexture;
extern Texture* timerindicatorTexture;

// DllMain: Main entry point for DLL. We are not interested in this, all we get is our own HMODULE in case we need it.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH: hSelf = hModule; break;
        case DLL_PROCESS_DETACH: break;
        case DLL_THREAD_ATTACH: break;
        case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}

// GetAddonDef: Export needed to give Nexus information about the addon.
extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
    AddonDef.Signature = 59330; // set to random unused negative integer
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = "Simple Speedometer";
    AddonDef.Version.Major = 1;
    AddonDef.Version.Minor = 0;
    AddonDef.Version.Build = 0;
    AddonDef.Version.Revision = 1;
    AddonDef.Author = "Toxxa";
    AddonDef.Description = "A lightly customizeable speedometer to show your current velocity. Comes with a functional, movement-triggered timer system.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;

    /* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
    //AddonDef.Provider = EUpdateProvider_GitHub;
    //AddonDef.UpdateLink = "https://github.com/RaidcoreGG/GW2Nexus-AddonTemplate";

    return &AddonDef;
}

// AddonLoad: Load function for the addon, will receive a pointer to the API.
void AddonLoad(AddonAPI* aApi)
{
    APIDefs = aApi;

    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);

    NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
    MumbleLink = (Mumble::Data*)APIDefs->DataLink.Get("DL_MUMBLE_LINK");
    MumbleIdentity = (Mumble::Identity*)APIDefs->DataLink.Get("DL_MUMBLE_LINK_IDENTITY");

    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

    //APIDefs->InputBinds.RegisterWithString("KB_DO_SOMETHING", Settings::DoSomething, "(null)");
    APIDefs->InputBinds.RegisterWithString("Toggle Speedometer visibility", SpeedometerToggleVisibility, "(null)");
    APIDefs->InputBinds.RegisterWithString("Toggle Timer visibility", TimerToggleVisibility, "(null)");
    APIDefs->InputBinds.RegisterWithString("Pause or reset the Timer / set locations / toggle selection window", TimerPauseReset, "(null)");

    AddonPath = APIDefs->Paths.GetAddonDirectory("Simple Speedometer");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("Simple Speedometer/settings.json");
    CoordinatesPath = APIDefs->Paths.GetAddonDirectory("Simple Speedometer/coordinates.json");

    if (std::filesystem::exists(APIDefs->Paths.GetAddonDirectory("Simple Speedometer/coordinates.json")))
    {
        wasJsonMissing = false;
    }

    ImFontConfig fontConfig;
    fontConfig.GlyphMinAdvanceX = 9.5f;  // Abstand zwischen den Zeichen
    APIDefs->Fonts.AddFromResource("crashFont", 20.0f, crash_a_like, hSelf, ReceiveFont, &fontConfig);

    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);
    if (Settings::IsCloudConfigEnabled) {
        Coordinates::LoadCloudConfig();
    }
    else {
        Coordinates::Load(CoordinatesPath);
    }

    APIDefs->Log(ELogLevel_DEBUG, "Load Simple Speedometer", "I am  <c=#00ff00>speed</c>.");
}

void ReceiveFont(const char* aIdentifier, void* aFont) {
    std::string str = aIdentifier;

    if (aFont == nullptr) {
    #ifndef NDEBUG
        APIDefs->Log(ELogLevel_CRITICAL, "ADDON_NAME", ("Received nullptr for font " + std::string(aIdentifier)).c_str());
    #endif // !NDEBUG
        return;
    }

    if (str == "crashFont")
    {
        fontptr = aFont;
    }
}

// AddonUnload: Everything you registered in AddonLoad, you should "undo" here.
void AddonUnload()
{
    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonOptions);

    //APIDefs->InputBinds.Deregister("KB_DO_SOMETHING");
    APIDefs->InputBinds.Deregister("Toggle Speedometer visibility");
    APIDefs->InputBinds.Deregister("Toggle Timer visibility");
    APIDefs->InputBinds.Deregister("Pause or reset the Timer / set locations / toggle selection window");

    MumbleLink = nullptr;
    NexusLink = nullptr;
    MumbleIdentity = nullptr;

    frequencyInitialized = false;
    frequency.QuadPart = 0;
    timerStart.QuadPart = 0;
    current.QuadPart = 0;

    APIDefs->Fonts.Release("crashFont", ReceiveFont);

    Settings::Save(SettingsPath);
    //Coordinates::Save(CoordinatesPath);

    APIDefs->Log(ELogLevel_DEBUG, "Unload Simple Speedometer", "I am no longer <c=#ff0000>speed</c>.");
}

// Getting mumble positional data for X and Z coordinates
float CalculateSpeed2D(const Vector3& prev, const Vector3& current, float deltaTime)
{
    if (NexusLink && MumbleLink && MumbleIdentity)
    {
        if (deltaTime <= 0) return 0.0f;
        float dx = current.X - prev.X;
        float dz = current.Z - prev.Z;
        return sqrtf(dx * dx + dz * dz) / deltaTime;
    }
}

// Getting mumble positional data for X, Y and Z coordinates
float CalculateSpeed3D(const Vector3& prev, const Vector3& current, float deltaTime)
{
    if (NexusLink && MumbleLink && MumbleIdentity)
    {
        if (deltaTime <= 0) return 0.0f;
        float dx = current.X - prev.X;
        float dy = current.Y - prev.Y;
        float dz = current.Z - prev.Z;
        return sqrtf(dx * dx + dy * dy + dz * dz) / deltaTime;
    }
}

// Initializing a high precision timer for the calculations. Using GetTickCount64 proved being inaccurate, as it only gets updated about once every 16 ms on most systems, which led to the timer not being updated on every frame if fps > 60 and made the speedometer readings a bit funky. So instead i opted for QueryPerformanceFrequency as the base for the calculations
void InitializeHighResTimer()
{
    if (!frequencyInitialized)
    {
        QueryPerformanceFrequency(&frequency);
        frequencyInitialized = true;
    }
}

// Calculating speed from mumble player position data
void UpdateSpeed()
{
    if (!frequencyInitialized)
    {
        InitializeHighResTimer();
    }
    const float fixedDeltaTime = 0.04f; // Mumble update frequency, used to properly determine distance per second from the difference between updates, the"Mumble-Units per seconds", if you will, which get translated to ingame useable values with the conversion factors
    QueryPerformanceCounter(&current);

    ULONGLONG currentTime = static_cast<ULONGLONG>((current.QuadPart * 1000) / frequency.QuadPart);
    if (currentTime - lastMumbleUpdate < 14) // polling frequency, how much time in ms must have passed before a new calculation is made with current mumble data vs the previous.
        return;
    lastMumbleUpdate = currentTime;

    if (MumbleLink != nullptr)
    {
        Vector3 currentPos = MumbleLink->AvatarPosition;
        g_currentPos = currentPos; // exporting coordinates for use in a different function

        if (prevTime == 0)
        {
            prevPos = currentPos;
            prevTime = currentTime;
            return;
        }

        float newSpeed2D = CalculateSpeed2D(prevPos, currentPos, fixedDeltaTime);
        float newSpeed3D = CalculateSpeed3D(prevPos, currentPos, fixedDeltaTime);

        // Mitigating speed spikes from intermittent double readings due to polling Mumble slower than it updates - nothing's gonna save Speedometer accuracy from low framerates and frametime spikes when accelerating / decelerating though, need external polling for that, like Killer's beetle speedometer.
        if (newSpeed2D >= 1.95f * lastSpeed2D && newSpeed2D <= 2.05f * lastSpeed2D)
        {
            newSpeed2D = lastSpeed2D;
        }

        // Same here, but for 3D speed instead.
        if (newSpeed3D >= 1.95f * lastSpeed3D && newSpeed3D <= 2.05f * lastSpeed3D)
        {
            newSpeed3D = lastSpeed3D;
        }

        // Mitigating speed spikes from intermittent zero readings due to polling Mumble more often than it updates. This is intentional, as getting rid of intermittent zeroes is far easier to accomplish than accurately interpreting when a positional difference contained two updates and when not.
        if (newSpeed2D == 0.0f)
        {
            zeroCount2D++;
            if (zeroCount2D <= 2 or (currentTime - prevTime) < static_cast<ULONGLONG>(fixedDeltaTime))
            {
                newSpeed2D = lastSpeed2D;
            }
            else
            {
                lastSpeed2D = 0.0f;
            }
        }
        else
        {
            zeroCount2D = 0;
            lastSpeed2D = newSpeed2D;
        }

        // Same here, but for 3D speed instead.
        if (newSpeed3D == 0.0f)
        {
            zeroCount3D++;
            if (zeroCount3D <= 2 or (currentTime - prevTime) < static_cast<ULONGLONG>(fixedDeltaTime))
            {
                newSpeed3D = lastSpeed3D;
            }
            else
            {
                lastSpeed3D = 0.0f;
            }
        }
        else
        {
            zeroCount3D = 0;
            lastSpeed3D = newSpeed3D;
        }

        // Updating speed reading and positional data.
        speed2D = newSpeed2D;
        speed3D = newSpeed3D;

        prevPos = currentPos;
        prevTime = currentTime;
    }
}

// 2D vector struct, adapted from the one found in mumble.h to acquire screen coordinates
struct MyVector2
{
    float x, y;
    MyVector2() : x(0), y(0) {}
    MyVector2(float _x, float _y) : x(_x), y(_y) {}
    MyVector2 operator-(const MyVector2& other) const
    {
        return MyVector2(x - other.x, y - other.y);
    }
    float Length() const
    {
        return std::sqrt(x * x + y * y);
    }
};

// 4D vector struct, used in the calculations to translate world coordinates to screen coordinates
struct Vector4 {
    float x, y, z, w;
};

// 4x4 matrix struct, used in the calculations to translate world coordinates to screen coordinates
struct Matrix4
{
    float m[4][4];
};

// Helper function to convert GW2 coordinate data to math. GW2 uses X Z for map coordinates and Y for the height, instead of X Y for map coordinates and Z for the height.
static Vector3 ConvertGW2ToMath(const Vector3& v)
{
    Vector3 result;
    result.X = v.X;
    result.Y = v.Z;
    result.Z = v.Y;
    return result;
}

// Generating a LookAt matrix from eye, target and up variables
static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    Vector3 z;
    z.X = eye.X - target.X;
    z.Y = eye.Y - target.Y;
    z.Z = eye.Z - target.Z;
    float len = std::sqrt(z.X * z.X + z.Y * z.Y + z.Z * z.Z);
    z.X /= len; z.Y /= len; z.Z /= len;

    Vector3 x;
    x.X = up.Y * z.Z - up.Z * z.Y;
    x.Y = up.Z * z.X - up.X * z.Z;
    x.Z = up.X * z.Y - up.Y * z.X;
    len = std::sqrt(x.X * x.X + x.Y * x.Y + x.Z * x.Z);
    x.X /= len; x.Y /= len; x.Z /= len;

    Vector3 y;
    y.X = z.Y * x.Z - z.Z * x.Y;
    y.Y = z.Z * x.X - z.X * x.Z;
    y.Z = z.X * x.Y - z.Y * x.X;

    Matrix4 result = {};
    result.m[0][0] = x.X;
    result.m[0][1] = x.Y;
    result.m[0][2] = x.Z;
    result.m[0][3] = -(x.X * eye.X + x.Y * eye.Y + x.Z * eye.Z);

    result.m[1][0] = y.X;
    result.m[1][1] = y.Y;
    result.m[1][2] = y.Z;
    result.m[1][3] = -(y.X * eye.X + y.Y * eye.Y + y.Z * eye.Z);

    result.m[2][0] = z.X;
    result.m[2][1] = z.Y;
    result.m[2][2] = z.Z;
    result.m[2][3] = -(z.X * eye.X + z.Y * eye.Y + z.Z * eye.Z);

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}

// Generating a perspective-dependant projection matrix
static Matrix4 Perspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
{
    Matrix4 result = {};
    float fovRad = fovDegrees * 3.14159265f / 180.0f;
    float f = 1.0f / std::tanf(fovRad / 2.0f);
    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.m[2][3] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    result.m[3][2] = -1.0f;
    return result;
}

// Multiplying the 4x4 matrix with the 4D vector
static Vector4 Multiply(const Matrix4& mat, const Vector4& v)
{
    Vector4 result;
    result.x = mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z + mat.m[0][3] * v.w;
    result.y = mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z + mat.m[1][3] * v.w;
    result.z = mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z + mat.m[2][3] * v.w;
    result.w = mat.m[3][0] * v.x + mat.m[3][1] * v.y + mat.m[3][2] * v.z + mat.m[3][3] * v.w;
    return result;
}

// Tranforming 3D world coordinates into 2D screen coordinates
static bool WorldToScreen(const Vector3& worldPos, const Matrix4& view, const Matrix4& projection, float screenWidth, float screenHeight, MyVector2& screen)
{
    Vector4 world = { worldPos.X, worldPos.Y, worldPos.Z, 1.0f };
    Vector4 clip = Multiply(projection, Multiply(view, world));

    if (clip.w < 0.001f)
        return false;

    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;

    screen.x = (ndcX * 0.5f + 0.5f) * screenWidth;
    screen.y = (1.0f - (ndcY * 0.5f + 0.5f)) * screenHeight;
    return true;
}

//Rendering the flat ground circles
static void RenderGroundCircle(const Vector3& worldCenter, float worldRadius, ImU32 colorchoice, int numSegments = 64)
{
    if (NexusLink && MumbleLink && MumbleIdentity && !MumbleLink->Context.IsMapOpen && NexusLink->IsGameplay)
    {
        if (Settings::IsTimerEnabled)
        {
            Vector3 centerMath = ConvertGW2ToMath(worldCenter); // converting center on-screen 3D world coordinate to math

            // Getting and converting the camera positional and directional data from Mumble
            Vector3 camPos = ConvertGW2ToMath(MumbleLink->CameraPosition);
            Vector3 camFront = ConvertGW2ToMath(MumbleLink->CameraFront);
            Vector3 camUp = { 0.0f, 0.0f, 1.0f }; // GW2 camera can only pitch and yaw, so it's always aligned parallel to the ground and we assume the Z coordinate to be fixed

            // Calculating viewing direction of the camera
            Vector3 target = { camPos.X + camFront.X, camPos.Y + camFront.Y, camPos.Z + camFront.Z };
            Matrix4 view = LookAt(camPos, target, camUp);

            // Getting resolution, picture format and the FOV to calculate the perspective of the on-screen projection
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            float screenWidth = displaySize.x;
            float screenHeight = displaySize.y;
            float fov = 70 / 1.2220f * MumbleIdentity->FOV; // Don't ask me why, but 70 degrees FOV in this calculation matches max ingame FOV, which is reported by mumble as 1.2220. To properly adjust calculation for FoV, 70 / 1.2220 * mumble fov is the way. If it works, it works
            Matrix4 projection = Perspective(fov, screenWidth / screenHeight, 0.1f, 1000.0f);

            // Doing math to generate a perspective-accurate circle render that adjusts based on camera angle. Circle area lies in the 2D space of X and Z coordinates, with its axis being Y
            std::vector<MyVector2> screenPoints;
            screenPoints.reserve(numSegments);
            for (int i = 0; i < numSegments; i++)
            {
                float theta = (2.0f * 3.14159265f * i) / numSegments;
                float cosTheta = std::cos(theta);
                float sinTheta = std::sin(theta);
                Vector3 point;
                point.X = centerMath.X + worldRadius * cosTheta;
                point.Z = centerMath.Z;
                point.Y = centerMath.Y + worldRadius * sinTheta;
                MyVector2 sp;
                if (WorldToScreen(point, view, projection, screenWidth, screenHeight, sp))
                {
                    screenPoints.push_back(sp);
                }
            }

            // If none of the calculated points of the circle's circumference match visible screen coordinates, aka circle not visible on screen area, abort rendering
            if (screenPoints.empty())
                return;

            // Rendering the circles one after another
            ImDrawList* drawList = ImGui::GetForegroundDrawList();

            for (size_t i = 0; i < screenPoints.size(); i++)
            {
                size_t nextIndex = (i + 1) % screenPoints.size(); // Kreis schließen
                const MyVector2& p0 = screenPoints[i];
                const MyVector2& p1 = screenPoints[nextIndex];

                // Handling a bug where a circle located to the right and back of the camera, outside of view, would render as a distorted pair of lines from the upper left corner of screen towards its off-screen location. Since this can only happen if the circle is not on screen, but the code thinks it is, if this happens, rendering of the affected segments gets aborted
                if ((p0.x < 10 && p0.y < 10) || (p1.x < 10 && p1.y < 10))
                    continue;
                drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), colorchoice, g_circlethickness);
            }
        }
    }
}

struct BillboardPoint
{
    MyVector2 screen;
    Vector3 world;
};

//Rendering the upright arcs
static void RenderBillboardCircle_Arc(const Vector3& worldCenter, float worldRadius, ImU32 colorchoice, int numSegments = 64)
{
    if (NexusLink && MumbleLink && MumbleIdentity && !MumbleLink->Context.IsMapOpen && NexusLink->IsGameplay)
    {
        if (Settings::IsTimerEnabled)
        {
            Vector3 centerMath = ConvertGW2ToMath(worldCenter);

            Vector3 camPos = ConvertGW2ToMath(MumbleLink->CameraPosition);
            Vector3 camFront = ConvertGW2ToMath(MumbleLink->CameraFront);

            Vector3 toCamera = { camPos.X - centerMath.X, camPos.Y - centerMath.Y, camPos.Z - centerMath.Z };
            float len = std::sqrt(toCamera.X * toCamera.X + toCamera.Y * toCamera.Y + toCamera.Z * toCamera.Z);
            if (len > 0.0001f)
            {
                toCamera.X /= len;
                toCamera.Y /= len;
                toCamera.Z /= len;
            }

            Vector3 camUp = { 0.0f, 0.0f, 1.0f };

            Vector3 target = { camPos.X + camFront.X, camPos.Y + camFront.Y, camPos.Z + camFront.Z };
            Matrix4 view = LookAt(camPos, target, camUp);

            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            float screenWidth = displaySize.x;
            float screenHeight = displaySize.y;
            float fov = 70 / 1.2220f * MumbleIdentity->FOV;
            Matrix4 projection = Perspective(fov, screenWidth / screenHeight, 0.1f, 1000.0f);

            Vector3 right;
 
            right.X = camFront.Y * camUp.Z - camFront.Z * camUp.Y;
            right.Y = camFront.Z * camUp.X - camFront.X * camUp.Z;
            right.Z = camFront.X * camUp.Y - camFront.Y * camUp.X;
            
            // this used to make the plane of the rendered shape point towards the physical center of the camera, causing them to turn after it. Having them just point the same way the camera does makes it a little bit less eratic.
            /*
            right.X = toCamera.Y * camUp.Z - toCamera.Z * camUp.Y;
            right.Y = toCamera.Z * camUp.X - toCamera.X * camUp.Z;
            right.Z = toCamera.X * camUp.Y - toCamera.Y * camUp.X;
            */

            float rLen = std::sqrt(right.X * right.X + right.Y * right.Y + right.Z * right.Z);
            if (rLen > 0.0001f)
            {
                right.X /= rLen; right.Y /= rLen; right.Z /= rLen;
            }

            Vector3 billboardUp;

            
            billboardUp.X = right.Y * 0 - right.Z * camFront.Y;
            billboardUp.Y = right.Z * camFront.X - right.X * 0;
            billboardUp.Z = right.X * camFront.Y - right.Y * camFront.X;
            
            // seemingly no difference between this and using camFront. I'll keep it around for possible later uses.
            /*
            billboardUp.X = right.Y * 0 - right.Z * toCamera.Y;
            billboardUp.Y = right.Z * toCamera.X - right.X * 0;
            billboardUp.Z = right.X * toCamera.Y - right.Y * toCamera.X;
            */

            float upLen = std::sqrt(billboardUp.X * billboardUp.X + billboardUp.Y * billboardUp.Y + billboardUp.Z * billboardUp.Z);
            if (upLen > 0.0001f)
            {
                billboardUp.X /= upLen; billboardUp.Y /= upLen; billboardUp.Z /= upLen;
            }

            std::vector<BillboardPoint> billboardPoints;
            billboardPoints.reserve(numSegments);
            for (int i = 0; i < numSegments; i++)
            {
                float theta = (2.0f * 3.14159265f * i) / numSegments;
                Vector3 offset;
                offset.X = std::cos(theta) * right.X + std::sin(theta) * billboardUp.X;
                offset.Y = std::cos(theta) * right.Y + std::sin(theta) * billboardUp.Y;
                offset.Z = std::cos(theta) * right.Z + std::sin(theta) * billboardUp.Z;

                Vector3 worldPoint;
                worldPoint.X = centerMath.X + worldRadius * offset.X;
                worldPoint.Y = centerMath.Y + worldRadius * offset.Y;
                worldPoint.Z = centerMath.Z + worldRadius * offset.Z;

                MyVector2 sp;
                if (WorldToScreen(worldPoint, view, projection, screenWidth, screenHeight, sp))
                {
                    billboardPoints.push_back(BillboardPoint{ sp, worldPoint });
                }
            }
            if (billboardPoints.empty())
                return;

            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            for (size_t i = 0; i < billboardPoints.size(); i++)
            {
                size_t nextIndex = (i + 1) % billboardPoints.size();
                const BillboardPoint& p0 = billboardPoints[i];
                const BillboardPoint& p1 = billboardPoints[nextIndex];

                bool above0 = (p0.world.Z >= centerMath.Z);
                bool above1 = (p1.world.Z >= centerMath.Z);

                // Only rendering what's above the perspectivic circle plane
                if (above0 && above1)
                {
                    drawList->AddLine(ImVec2(p0.screen.x, p0.screen.y),
                        ImVec2(p1.screen.x, p1.screen.y),
                        colorchoice, g_circlethickness);
                }
                else if (above0 && !above1)
                {
                    float t = (centerMath.Z - p0.world.Z) / (p1.world.Z - p0.world.Z);
                    Vector3 interWorld;
                    interWorld.X = p0.world.X + t * (p1.world.X - p0.world.X);
                    interWorld.Y = p0.world.Y + t * (p1.world.Y - p0.world.Y);
                    interWorld.Z = centerMath.Z;
                    MyVector2 interScreen;
                    if (WorldToScreen(interWorld, view, projection, screenWidth, screenHeight, interScreen))
                    {
                        drawList->AddLine(ImVec2(p0.screen.x, p0.screen.y),
                            ImVec2(interScreen.x, interScreen.y),
                            colorchoice, g_circlethickness);
                    }
                }
                else if (!above0 && above1)
                {
                    float t = (centerMath.Z - p0.world.Z) / (p1.world.Z - p0.world.Z);
                    Vector3 interWorld;
                    interWorld.X = p0.world.X + t * (p1.world.X - p0.world.X);
                    interWorld.Y = p0.world.Y + t * (p1.world.Y - p0.world.Y);
                    interWorld.Z = centerMath.Z;
                    MyVector2 interScreen;
                    if (WorldToScreen(interWorld, view, projection, screenWidth, screenHeight, interScreen))
                    {
                        drawList->AddLine(ImVec2(interScreen.x, interScreen.y),
                            ImVec2(p1.screen.x, p1.screen.y),
                            colorchoice, g_circlethickness);
                    }
                }
            }
        }
    }
}

// Rendering the full vertical circles
static void RenderBillboardCircle(const Vector3& worldCenter, float worldRadius, ImU32 colorchoice, int numSegments = 64)
{
    if (NexusLink && MumbleLink && MumbleIdentity && !MumbleLink->Context.IsMapOpen && NexusLink->IsGameplay)
    {
        if (Settings::IsTimerEnabled)
        {
            Vector3 centerMath = ConvertGW2ToMath(worldCenter);
            Vector3 camPos = ConvertGW2ToMath(MumbleLink->CameraPosition);
            Vector3 camFront = ConvertGW2ToMath(MumbleLink->CameraFront);

            Vector3 toCamera = { camPos.X - centerMath.X, camPos.Y - centerMath.Y, camPos.Z - centerMath.Z };
            float len = std::sqrt(toCamera.X * toCamera.X + toCamera.Y * toCamera.Y + toCamera.Z * toCamera.Z);
            if (len > 0.0001f)
            {
                toCamera.X /= len;
                toCamera.Y /= len;
                toCamera.Z /= len;
            }

            Vector3 camUp = { 0.0f, 0.0f, 1.0f };
            Vector3 target = { camPos.X + camFront.X, camPos.Y + camFront.Y, camPos.Z + camFront.Z };
            Matrix4 view = LookAt(camPos, target, camUp);

            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            float screenWidth = displaySize.x;
            float screenHeight = displaySize.y;
            float fov = 70 / 1.2220f * MumbleIdentity->FOV;
            Matrix4 projection = Perspective(fov, screenWidth / screenHeight, 0.1f, 1000.0f);

            Vector3 right;

            right.X = camFront.Y * camUp.Z - camFront.Z * camUp.Y;
            right.Y = camFront.Z * camUp.X - camFront.X * camUp.Z;
            right.Z = camFront.X * camUp.Y - camFront.Y * camUp.X;

            /*
            right.X = toCamera.Y * camUp.Z - toCamera.Z * camUp.Y;
            right.Y = toCamera.Z * camUp.X - toCamera.X * camUp.Z;
            right.Z = toCamera.X * camUp.Y - toCamera.Y * camUp.X;
            */

            float rLen = std::sqrt(right.X * right.X + right.Y * right.Y + right.Z * right.Z);
            if (rLen > 0.0001f)
            {
                right.X /= rLen; right.Y /= rLen; right.Z /= rLen;
            }

            Vector3 billboardUp;

            billboardUp.X = right.Y * 0 - right.Z * camFront.Y;
            billboardUp.Y = right.Z * camFront.X - right.X * 0;
            billboardUp.Z = right.X * camFront.Y - right.Y * camFront.X;

            /*
            billboardUp.X = right.Y * 0 - right.Z * toCamera.Y;
            billboardUp.Y = right.Z * toCamera.X - right.X * 0;
            billboardUp.Z = right.X * toCamera.Y - right.Y * toCamera.X;
            */

            float upLen = std::sqrt(billboardUp.X * billboardUp.X + billboardUp.Y * billboardUp.Y + billboardUp.Z * billboardUp.Z);
            if (upLen > 0.0001f)
            {
                billboardUp.X /= upLen; billboardUp.Y /= upLen; billboardUp.Z /= upLen;
            }

            std::vector<MyVector2> billboardPoints;
            billboardPoints.reserve(numSegments);
            for (int i = 0; i < numSegments; i++)
            {
                float theta = (2.0f * 3.14159265f * i) / numSegments;

                Vector3 offset;
                offset.X = std::cos(theta) * right.X + std::sin(theta) * billboardUp.X;
                offset.Y = std::cos(theta) * right.Y + std::sin(theta) * billboardUp.Y;
                offset.Z = std::cos(theta) * right.Z + std::sin(theta) * billboardUp.Z;

                Vector3 billboardPoint = {
                    centerMath.X + worldRadius * offset.X,
                    centerMath.Y + worldRadius * offset.Y,
                    centerMath.Z + worldRadius * offset.Z
                };
                MyVector2 sp;
                if (WorldToScreen(billboardPoint, view, projection, screenWidth, screenHeight, sp))
                {
                    billboardPoints.push_back(sp);
                }
            }

            if (billboardPoints.empty())
                return;

            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            for (size_t i = 0; i < billboardPoints.size(); i++)
            {
                size_t nextIndex = (i + 1) % billboardPoints.size();
                const MyVector2& p0 = billboardPoints[i];
                const MyVector2& p1 = billboardPoints[nextIndex];

                if ((p0.x < 10 && p0.y < 10) || (p1.x < 10 && p1.y < 10))
                    continue;
                drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), colorchoice, g_circlethickness);
            }
        }
    }
}

// Main function to handle the math behind the timer values
void UpdateTimer()
{
    if (Settings::IsTimerEnabled)
    {
        std::vector<std::string> filteredSetNames;
        bool useExternalSets = !Coordinates::SetNames.empty();

        if (!frequencyInitialized)
        {
            InitializeHighResTimer();
        }

        Vector3 currentPos = MumbleLink->AvatarPosition;

        const float manualmovementThreshold = Settings::manualstartDiameter / conversionFactor_u_s - 0.381f;
        const float movementThreshold = (Settings::optionPredefined ? startRadius : Settings::startDiameter) / conversionFactor_u_s - 0.381f; // Maximum distance allowed to move away from the start position before the timer triggers, also used to calculate the radii of the circle indicators for visualization of the timer trigger mechanic
        const float finishThreshold = (Settings::optionPredefined ? endRadius : Settings::finishDiameter) / conversionFactor_u_s + 0.381f;
        const float checkpointThreshold = checkpointRadius / conversionFactor_u_s + 0.381f;

        // Initial setup of the base coordinates that act as the starting position for the timer
        if (Settings::optionManual)
        {
            if (!basePosInitialized) {
                basePos = currentPos;
                basePosInitialized = true;
            }
        }

        // Storing the internal / external coordinate set data in variables for use in the rendering update functions
        if (Settings::optionPredefined)
        {
            const Coordinates::CoordinateSet* selectedSet = Coordinates::GetSelectedCoordinateSet();
            if (selectedSet)
            {
                Coordinates::Vector3 start = selectedSet->Start;
                Coordinates::Vector3 end = selectedSet->End;
                startRadius = selectedSet->StartRadius;
                endRadius = selectedSet->EndRadius;
                checkpointcount = selectedSet->Checkpoints.size();

                basePos.X = start.x;
                basePos.Y = start.y;
                basePos.Z = start.z;

                pausePos.X = end.x;
                pausePos.Y = end.y;
                pausePos.Z = end.z;

                if (!selectedSet->Checkpoints.empty() && currentCheckpointIndex < selectedSet->Checkpoints.size())
                {
                    const Coordinates::Checkpoint& checkpoint = selectedSet->Checkpoints[currentCheckpointIndex];

                    checkpointlist = true;

                    checkpointPos.X = checkpoint.Position.x;
                    checkpointPos.Y = checkpoint.Position.y;
                    checkpointPos.Z = checkpoint.Position.z;

                    checkpointRadius = checkpoint.Radius;
                }

                if (!selectedSet->Checkpoints.empty() && (currentCheckpointIndex + 1) <= selectedSet->Checkpoints.size())
                {
                    const Coordinates::Checkpoint& nextcheckpoint = selectedSet->Checkpoints[currentCheckpointIndex + 1];

                    nextcheckpointPos.X = nextcheckpoint.Position.x;
                    nextcheckpointPos.Y = nextcheckpoint.Position.y;
                    nextcheckpointPos.Z = nextcheckpoint.Position.z;

                    nextcheckpointRadius = nextcheckpoint.Radius;
                }
                else if (!selectedSet->Checkpoints.empty() && (currentCheckpointIndex + 1) > selectedSet->Checkpoints.size())
                {
                    checkpointlistfinished = true;
                    nextcheckpointPos.X = -10000.0f;
                    nextcheckpointPos.Y = -10000.0f;
                    nextcheckpointPos.Z = -10000.0f;
                }
            }
            else
            {
                APIDefs->Log(ELogLevel_WARNING, "Simple Speedometer", "No valid coordinate set selected.");
            }
        }

        // This calculates distance between camera and circle center, which wasn't what i needed to do the fading, but it could be useful later on so it lives as a comment for now 
        /*
        float distance = std::sqrt(
            (camPos.X - centerMath.X) * (camPos.X - centerMath.X) +
            (camPos.Y - centerMath.Y) * (camPos.Y - centerMath.Y) +
            (camPos.Z - centerMath.Z) * (camPos.Z - centerMath.Z)
        );
        */

        // Calculating the distance between base coordinates where the timer was set and the concurrent position of the player character
        float basedx = currentPos.X - basePos.X;
        float basedy = currentPos.Y - basePos.Y;
        float basedz = currentPos.Z - basePos.Z;
        float basedistance = sqrtf(basedx * basedx + basedy * basedy + basedz * basedz);

        // Calculating the distance between the coordinates where the timer was paused and the concurrent position of the player character
        float pausedx = currentPos.X - pausePos.X;
        float pausedy = currentPos.Y - pausePos.Y;
        float pausedz = currentPos.Z - pausePos.Z;
        float pausedistance = sqrtf(pausedx * pausedx + pausedy * pausedy + pausedz * pausedz);

        // Calculating the distance between the coordinates where the timer was paused and the concurrent position of the player character
        float checkedx = currentPos.X - checkpointPos.X;
        float checkedy = currentPos.Y - checkpointPos.Y;
        float checkedz = currentPos.Z - checkpointPos.Z;
        float checkpointdistance = sqrtf(checkedx * checkedx + checkedy * checkedy + checkedz * checkedz);

        // Calculating the distance between the coordinates where the timer was paused and the concurrent position of the player character
        float nextcheckedx = currentPos.X - nextcheckpointPos.X;
        float nextcheckedy = currentPos.Y - nextcheckpointPos.Y;
        float nextcheckedz = currentPos.Z - nextcheckpointPos.Z;
        float nextcheckpointdistance = sqrtf(nextcheckedx * nextcheckedx + nextcheckedy * nextcheckedy + nextcheckedz * nextcheckedz);

        // Exporting coordinates for use in a different function
        g_basedistance = basedistance; 
        g_pausedistance = pausedistance;
        g_checkpointdistance = checkpointdistance;

        // Calculating the alpha value (= opacity of the circles) based on distance between player character and circle center. Clamping alpha to zero when max distance is exceeded.
        float startingcircleFadingStart = (Settings::startFadingDistance + (Settings::optionCustom ? Settings::startDiameter : (Settings::optionPredefined ? startRadius : Settings::manualstartDiameter))) / conversionFactor_u_s;
        float startingcircleFadingEnd = (Settings::startFadingDistance + (Settings::optionCustom ? Settings::startDiameter : (Settings::optionPredefined ? startRadius : Settings::manualstartDiameter))) / conversionFactor_u_s + 10.0f;
        float startcirclealpha = 1.0f;
        if (basedistance > startingcircleFadingStart)
        {
            startcirclealpha = 1.0f - ((basedistance - startingcircleFadingStart) / (startingcircleFadingEnd - startingcircleFadingStart));
            if (startcirclealpha < 0.0f) startcirclealpha = 0.0f;
        }

        float selfclosestartFadingStart = (Settings::optionPredefined ? startRadius : Settings::startDiameter) / conversionFactor_u_s + 5.0f;
        float selfclosestartFadingEnd = (Settings::optionPredefined ? startRadius : Settings::startDiameter) / conversionFactor_u_s + 10.0f;
        float selfclosestartalpha = 1.0f;
        if (basedistance > selfclosestartFadingStart)
        {
            selfclosestartalpha = 1.0f - ((basedistance - selfclosestartFadingStart) / (selfclosestartFadingEnd - selfclosestartFadingStart));
            if (selfclosestartalpha < 0.0f) selfclosestartalpha = 0.0f;
        }

        float selfclosemanualstartFadingStart = Settings::manualstartDiameter / conversionFactor_u_s + 5.0f;
        float selfclosemanualstartFadingEnd = Settings::manualstartDiameter / conversionFactor_u_s + 10.0f;
        float selfclosemanualstartalpha = 1.0f;
        if (basedistance > selfclosemanualstartFadingStart)
        {
            selfclosemanualstartalpha = 1.0f - ((basedistance - selfclosemanualstartFadingStart) / (selfclosemanualstartFadingEnd - selfclosemanualstartFadingStart));
            if (selfclosemanualstartalpha < 0.0f) selfclosemanualstartalpha = 0.0f;
        }

        float finishcircleFadingStart = (Settings::finishFadingDistance + (Settings::optionCustom ? Settings::finishDiameter : (Settings::optionPredefined ? endRadius : Settings::manualstartDiameter))) / conversionFactor_u_s;
        float finishcircleFadingEnd = (Settings::finishFadingDistance + (Settings::optionCustom ? Settings::finishDiameter : (Settings::optionPredefined ? endRadius : Settings::manualstartDiameter))) / conversionFactor_u_s + 10.0f;
        float finishcirclealpha = 1.0f;
        if (pausedistance > finishcircleFadingStart) {
            finishcirclealpha = 1.0f - ((pausedistance - finishcircleFadingStart) / (finishcircleFadingEnd - finishcircleFadingStart));
            if (finishcirclealpha < 0.0f) finishcirclealpha = 0.0f;  // Begrenze auf min. 0
        }

        float selfclosefinishFadingStart = (Settings::optionPredefined ? endRadius : Settings::finishDiameter) / conversionFactor_u_s + 5.0f;
        float selfclosefinishFadingEnd = (Settings::optionPredefined ? endRadius : Settings::finishDiameter) / conversionFactor_u_s + 10.0f;
        float selfclosefinishalpha = 1.0f;
        if (pausedistance > selfclosefinishFadingStart) {
            selfclosefinishalpha = 1.0f - ((pausedistance - selfclosefinishFadingStart) / (selfclosefinishFadingEnd - selfclosefinishFadingStart));
            if (selfclosefinishalpha < 0.0f) selfclosefinishalpha = 0.0f;  // Begrenze auf min. 0
        }

        float checkpointcircleFadingStart = (Settings::finishFadingDistance + checkpointRadius) / conversionFactor_u_s;
        float checkpointcircleFadingEnd = (Settings::finishFadingDistance + checkpointRadius) / conversionFactor_u_s + 10.0f;
        float checkpointcirclealpha = 1.0f;
        if (checkpointdistance > checkpointcircleFadingStart) {
            checkpointcirclealpha = 1.0f - ((checkpointdistance - checkpointcircleFadingStart) / (checkpointcircleFadingEnd - checkpointcircleFadingStart));
            if (checkpointcirclealpha < 0.0f) checkpointcirclealpha = 0.0f;  // Begrenze auf min. 0
        }

        float nextcheckpointcircleFadingStart = (Settings::finishFadingDistance + nextcheckpointRadius) / conversionFactor_u_s;
        float nextcheckpointcircleFadingEnd = (Settings::finishFadingDistance + nextcheckpointRadius) / conversionFactor_u_s + 10.0f;
        float nextcheckpointcirclealpha = 0.3f;
        if (nextcheckpointdistance > nextcheckpointcircleFadingStart) {
            nextcheckpointcirclealpha = 0.3f - ((nextcheckpointdistance - nextcheckpointcircleFadingStart) / (nextcheckpointcircleFadingEnd - nextcheckpointcircleFadingStart));
            if (nextcheckpointcirclealpha < 0.0f) nextcheckpointcirclealpha = 0.0f;  // Begrenze auf min. 0
        }

        float selfclosecheckpointFadingStart = checkpointRadius / conversionFactor_u_s + 5.0f;
        float selfclosecheckpointFadingEnd = checkpointRadius / conversionFactor_u_s + 10.0f;
        float selfclosecheckpointalpha = 1.0f;
        if (checkpointdistance > selfclosecheckpointFadingStart) {
            selfclosecheckpointalpha = 1.0f - ((checkpointdistance - selfclosecheckpointFadingStart) / (selfclosecheckpointFadingEnd - selfclosecheckpointFadingStart));
            if (selfclosecheckpointalpha < 0.0f) selfclosecheckpointalpha = 0.0f;  // Begrenze auf min. 0
        }

        // Passing the desired colors of the circles to the rendering functions and calling them according to the user's settings
        if (Settings::optionCustom && timerHotkeyCount == 4)
        {
            g_circlethickness = 9.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
            }

            g_circlethickness = 2.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(currentPos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
            }
        }

        if (Settings::optionCustom && (timerHotkeyCount == 2 || timerHotkeyCount == 3))
        {
            g_circlethickness = 9.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }

            g_circlethickness = 2.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, Settings::startDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
        }

        if (Settings::optionManual)
        {
            g_circlethickness = 9.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }

            g_circlethickness = 2.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }

            if (timerPaused)
            {
                g_circlethickness = 9.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, Settings::manualstartDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
            }

            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosemanualstartalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosemanualstartalpha * 255)));
        }

        if (Settings::optionCustom)
        {
            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosestartalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosestartalpha * 255)));

            if (Settings::optionCustom && timerHotkeyCount == 2)
            {
                g_circlethickness = 9.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(175)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(currentPos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(255, 255, 25, static_cast<int>(175)));
                }
            }
            g_circlethickness = 9.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
            }

            g_circlethickness = 2.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(pausePos, Settings::finishDiameter / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
            }

            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosefinishalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosefinishalpha * 255)));
        }

        if (Settings::optionPredefined)
        {
            g_circlethickness = 9.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, startRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, startRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, startRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(startcirclealpha * 255)));
            }

            g_circlethickness = 2.0f;
            if (Settings::optionFlat)
            {
                RenderGroundCircle(basePos, startRadius / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionArc)
            {
                RenderBillboardCircle_Arc(basePos, startRadius / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }
            if (Settings::optionRing)
            {
                RenderBillboardCircle(basePos, startRadius / conversionFactor_u_s, IM_COL32(255, 255, 255, static_cast<int>(startcirclealpha * 255)));
            }

            if (!checkpointlistfinished)
            {
                g_circlethickness = 9.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(checkpointcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(checkpointcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(checkpointcirclealpha * 255)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(checkpointcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(checkpointcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(checkpointPos, checkpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(checkpointcirclealpha * 255)));
                }

                g_circlethickness = 9.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(nextcheckpointPos, nextcheckpointRadius / conversionFactor_u_s, IM_COL32(100, 255, 255, static_cast<int>(nextcheckpointcirclealpha * 255)));
                }
            }

            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosestartalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosestartalpha * 255)));

            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosecheckpointalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosecheckpointalpha * 255)));

            g_circlethickness = 9.0f;
            if ((currentCheckpointIndex + 1) == checkpointcount)
            {
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 77)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 77)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 77)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 77)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 77)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 77)));
                }
            }

            if ((currentCheckpointIndex + 1) > checkpointcount)
            {
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(0, 0, 0, static_cast<int>(finishcirclealpha * 255)));
                }

                g_circlethickness = 2.0f;
                if (Settings::optionFlat)
                {
                    RenderGroundCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionArc)
                {
                    RenderBillboardCircle_Arc(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
                if (Settings::optionRing)
                {
                    RenderBillboardCircle(pausePos, endRadius / conversionFactor_u_s, IM_COL32(25, 255, 25, static_cast<int>(finishcirclealpha * 255)));
                }
            }

            g_circlethickness = 9.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(0, 0, 0, static_cast<int>(selfclosefinishalpha * 255)));

            g_circlethickness = 2.0f;
            RenderGroundCircle(currentPos, 0.381f, IM_COL32(255, 125, 25, static_cast<int>(selfclosefinishalpha * 255)));
        }

        // Handling the timer triggering and pausing logic based on the user's settings
        if (Settings::optionManual)
        {
            if (timerPaused)
            {
                if (Settings::optionPause)
                {
                    if (pausedistance > manualmovementThreshold)
                    {
                        timerPaused = false;
                        timerRunning = true;
                        QueryPerformanceCounter(&current);
                        LONGLONG offset = static_cast<LONGLONG>(pausedElapsedSeconds * frequency.QuadPart);
                        timerStart.QuadPart = current.QuadPart - offset;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            if (!timerRunning && !timerPaused)
            {
                if (basedistance > manualmovementThreshold)
                {
                    timerRunning = true;
                    QueryPerformanceCounter(&timerStart);
                }
            }

            if (timerRunning || timerPaused)
            {
                if (basedistance < manualmovementThreshold)
                {
                    timerRunning = false;
                    timerPaused = false;
                }
            }
        }

        if (Settings::optionCustom)
        {
            if (timerStartSet && timerFinishSet)
            {
                if (basedistance < movementThreshold)
                {
                    timerStartEntered = true;
                    timerRunning = false;
                    timerPaused = false;
                }
                if (timerStartEntered && !timerRunning && !timerPaused)
                {
                    if (basedistance > movementThreshold)
                    {
                        timerRunning = true;
                        QueryPerformanceCounter(&timerStart);
                    }
                }
                if (timerStartEntered && timerRunning)
                {
                    if (pausedistance < finishThreshold)
                    {
                        double currElapsed = static_cast<double>(current.QuadPart - timerStart.QuadPart) / frequency.QuadPart;
                        if (currElapsed > 0.0)
                        {
                            timerPaused = true;
                            pausedElapsedSeconds = currElapsed;
                            timerRunning = false;
                        }
                    }
                }
            }
        }

        if (Settings::optionPredefined)
        {
            if (basedistance < movementThreshold)
            {
                timerStartEntered = true;
                timerRunning = false;
                timerPaused = false;
                checkpointlistfinished = false;
                currentCheckpointIndex = 0;
            }

            currentMapID = MumbleLink->Context.MapID;
            if (currentMapID != lastMapID && (timerRunning || timerPaused))
            {
                timerStartEntered = false;
                timerRunning = false;
                timerPaused = false;
                checkpointlistfinished = false;
                currentCheckpointIndex = 0;
            }

            if (timerStartEntered && !timerRunning && !timerPaused)
            {
                if (basedistance > movementThreshold)
                {
                    timerRunning = true;
                    QueryPerformanceCounter(&timerStart);
                }
            }
            if (timerStartEntered && timerRunning)
            {
                if (checkpointdistance < checkpointThreshold && checkpointlist)
                {
                    currentCheckpointIndex++;
                }

                if (pausedistance < finishThreshold && checkpointlistfinished)
                {
                    double currElapsed = static_cast<double>(current.QuadPart - timerStart.QuadPart) / frequency.QuadPart;
                    if (currElapsed > 0.0)
                    {
                        timerPaused = true;
                        pausedElapsedSeconds = currElapsed;
                        timerRunning = false;
                    }
                }
            }

        }
    }
}

// Main function responsible for rendering of the timer widget
void RenderTimerWindow()
{
    if (NexusLink && MumbleLink && MumbleIdentity && !MumbleLink->Context.IsMapOpen && NexusLink->IsGameplay)
    {
        if (Settings::IsTimerEnabled)
        {
            QueryPerformanceCounter(&current); // Storing the concurrent value of the high precision timer

            // Calculating the time that passed between the last and current frame in order for the timer to be updated
            double elapsedSeconds = 0.0;

            if (timerRunning && !timerPaused)
            {
                elapsedSeconds = static_cast<double>(current.QuadPart - timerStart.QuadPart) / frequency.QuadPart;

                // Since the only way to stop the timer is standing still and manually resetting it, this will make it force-restart before reaching the maximum reading of 99:59.999, otherwise the minutes counter would get funky and show the first two digits of the then triple digit minute count. Surely nobody keeps it running for over one hour and fourty minutes straight, right? ...right?
                if (elapsedSeconds >= 5999.983)
                {
                    timerRunning = false;
                    timerPaused = false;
                    basePosInitialized = false;
                }
            }
            else if (timerPaused)
            {
                elapsedSeconds = pausedElapsedSeconds; // Storing the current time on pausing the timer
            }

            // Calculating the acquired value from the high precision timer into minutes, seconds and milliseconds
            unsigned long minutes = static_cast<unsigned long>(elapsedSeconds / 60.0);
            unsigned long seconds = static_cast<unsigned long>(fmod(elapsedSeconds, 60.0));
            unsigned long milliseconds = static_cast<unsigned long>((elapsedSeconds - (minutes * 60 + seconds)) * 1000.0);

            // Handling timer position and size through the sliders in the settings
            ImVec2 TimerPos = ImVec2(Settings::TimerPositionH, Settings::TimerPositionV);
            ImVec2 TimerSize = ImVec2(Settings::TimerScale * 5.50f, Settings::TimerScale * 2.30f);

            // previously the timer didn't feature a clickable stopwatch, but it might change again, so the segment that handled transparency on mouseover lives here as comment for now
            /*
            ImVec2 timermousePos = ImGui::GetMousePos();
            bool isMouseOver = (timermousePos.x >= TimerPos.x && timermousePos.x <= TimerPos.x + TimerSize.x && timermousePos.y >= TimerPos.y && timermousePos.y <= TimerPos.y + TimerSize.y);
            float timertargetAlpha = isMouseOver ? 0.2f : 1.0f;
            timerimageAlpha += (timertargetAlpha - timerimageAlpha) * fadeSpeed;
            */

            // Setting up ImGui Window for the timer widget and its background
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::SetNextWindowPos(ImVec2(TimerPos), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(TimerSize));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Timer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);
            if (timerTexture)
            {
                ImGui::Image(timerTexture->Resource, TimerSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, timerimageAlpha));
            }
            else
            {
                timerTexture = APIDefs->Textures.GetOrCreateFromResource("timer_background", timer_background, hSelf);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);

            // Setting up ImGui Window for the timer digits
            if (!numberTexture)
            {
                numberTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_numbers", speedometer_numbers, hSelf);
            }
            if (numberTexture)
            {
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 0.42f, TimerPos.y + Settings::TimerScale * 1.292f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(TimerSize));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("TimerNumbers", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                // Formatting the timer value from the current MM:SS.mmm to a string MMSSmmmm of seven digits
                char timerStr[8]; // 7 Zeichen + null-Terminator
                snprintf(timerStr, sizeof(timerStr), "%02lu%02lu%03lu", minutes, seconds, milliseconds);

                int digitRow = timerRunning ? 0 : (timerPaused ? 4 : ((Settings::optionCustom && (!timerStartSet || !timerFinishSet || !timerStartEntered)) ? 3 : 2)); // Handling the choice of the row from where to pick a digit on the sprite sheet

                // Calculating which digits to pick for the minutes from the sprite-sheet and where they are to be displayed. Two digits, followed by a ":". The first digit is directly placed at the top left of the window, the following digits and the signs inbetween get carefully placed in accordance by means of imgui::sameline instructions
                for (int i = 0; i < 2; i++)
                {
                    int digit = timerStr[i] - '0';
                    float digitWidth = 1.0f / 12.0f;
                    float digitHeight = 1.0f / 5.0f;
                    ImVec2 digitUV0(digit * digitWidth, digitRow * digitHeight);
                    ImVec2 digitUV1((digit + 1) * digitWidth, (digitRow + 1) * digitHeight);

                    ImGui::Image(numberTexture->Resource,
                        ImVec2(Settings::TimerScale * 0.47f, Settings::TimerScale * 0.68f),
                        digitUV0, digitUV1, ImVec4(1, 1, 1, timerimageAlpha));
                    if (i == 0)
                        ImGui::SameLine(Settings::TimerScale * 0.420f, 0.0f); // Second minutes digit placement
                    if (i == 1)
                    {
                        ImGui::SameLine(Settings::TimerScale * 0.825f, 0.0f); // ":" placement and following is its source on the sprite-sheet
                        int colonIndex = 10;
                        float digitWidth = 1.0f / 12.0f;
                        float digitHeight = 1.0f / 5.0f;
                        ImVec2 colonUV0(colonIndex * digitWidth, digitRow * digitHeight);
                        ImVec2 colonUV1((colonIndex + 1) * digitWidth, (digitRow + 1) * digitHeight);
                        ImGui::Image(numberTexture->Resource,
                            ImVec2(Settings::TimerScale * 0.47f, Settings::TimerScale * 0.68f),
                            colonUV0, colonUV1, ImVec4(1, 1, 1, timerimageAlpha));
                    }
                }

                // Calculating which digits to pick for the seconds and where they are to be displayed
                for (int i = 2; i < 4; i++)
                {
                    if (i == 2)
                        ImGui::SameLine(Settings::TimerScale * 1.080f, 0.0f); // First seconds digit placement
                    if (i == 3)
                        ImGui::SameLine(Settings::TimerScale * 1.495f, 0.0f); // Second seconds digit placement
                    int digit = timerStr[i] - '0';
                    float digitWidth = 1.0f / 12.0f;
                    float digitHeight = 1.0f / 5.0f;
                    ImVec2 digitUV0(digit * digitWidth, digitRow * digitHeight);
                    ImVec2 digitUV1((digit + 1) * digitWidth, (digitRow + 1) * digitHeight);
                    ImGui::Image(numberTexture->Resource,
                        ImVec2(Settings::TimerScale * 0.47f, Settings::TimerScale * 0.68f),
                        digitUV0, digitUV1, ImVec4(1, 1, 1, timerimageAlpha));
                    if (i == 3)
                    {
                        ImGui::SameLine(Settings::TimerScale * 1.905f, 0.0f); // "." placement and following is its source on the sprite-sheet
                        int pointIndex = 11;
                        float digitWidth = 1.0f / 12.0f;
                        float digitHeight = 1.0f / 5.0f;
                        ImVec2 pointUV0(pointIndex * digitWidth, digitRow * digitHeight);
                        ImVec2 pointUV1((pointIndex + 1) * digitWidth, (digitRow + 1) * digitHeight);
                        ImGui::Image(numberTexture->Resource,
                            ImVec2(Settings::TimerScale * 0.47f, Settings::TimerScale * 0.68f),
                            pointUV0, pointUV1, ImVec4(1, 1, 1, timerimageAlpha));
                    }
                }

                // Calculating which digits to pick for the milliseconds and where they are to be displayed
                for (int i = 4; i < 7; i++)
                {
                    if (i == 4)
                        ImGui::SameLine(Settings::TimerScale * 2.165f, 0.0f); // First milliseconds digit placement
                    if (i == 5)
                        ImGui::SameLine(Settings::TimerScale * 2.585f, 0.0f); // Second milliseconds digit placement
                    if (i == 6)
                        ImGui::SameLine(Settings::TimerScale * 3.005f, 0.0f); // Third milliseconds digit placement
                    int digit = timerStr[i] - '0';
                    float digitWidth = 1.0f / 12.0f;
                    float digitHeight = 1.0f / 5.0f;
                    ImVec2 digitUV0(digit * digitWidth, digitRow * digitHeight);
                    ImVec2 digitUV1((digit + 1) * digitWidth, (digitRow + 1) * digitHeight);
                    ImGui::Image(numberTexture->Resource,
                        ImVec2(Settings::TimerScale * 0.47f, Settings::TimerScale * 0.68f),
                        digitUV0, digitUV1, ImVec4(1, 1, 1, timerimageAlpha));
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Setting up the mode displays on the Timer
            if (!timerindicatorTexture)
            {
                timerindicatorTexture = APIDefs->Textures.GetOrCreateFromResource("timer_indicators", timer_indicators, hSelf);
            }
            if (timerindicatorTexture)
            {
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 0.0f, TimerPos.y + Settings::TimerScale * 0.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(TimerSize));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Timer Modes", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                int timerindicatorFrame = Settings::optionManual ? 1 : (Settings::optionCustom ? 0 : 2); // Picking a specific frame from the sprite sheet based on settings - count starts from 0 for frame 1
                int timerindicatornumCols = 3; // Number of colums of the sprite sheet
                int timerindicatornumRows = 1; // Number of rows of the sprite sheet
                int timerindicatorframeCol = timerindicatorFrame % timerindicatornumCols; // Calculating the active column
                int timerindicatorframeRow = timerindicatorFrame / timerindicatornumCols; // Calculating the active row
                float timerindicatorframeWidth = 1.0f / static_cast<float>(timerindicatornumCols); // Calculating the width of each frame
                float timerindicatorframeHeight = 1.0f / static_cast<float>(timerindicatornumRows); // Calculating the height of each frame

                ImVec2 timerindicatorUV0(timerindicatorframeCol * timerindicatorframeWidth, timerindicatorframeRow * timerindicatorframeHeight); // Calculating upper left pixel coordinate of the frame
                ImVec2 timerindicatorUV1((timerindicatorframeCol + 1) * timerindicatorframeWidth, (timerindicatorframeRow + 1) * timerindicatorframeHeight); // Calculating lower right pixel coordinate of the frame

                ImVec2 timerindicatorPos = ImVec2(TimerPos.x + Settings::TimerScale * 1.65f, TimerPos.y + Settings::TimerScale * 0.220f); // Positioning relative to the Timer position
                ImVec2 timerindicatorSize = ImVec2(TimerSize.x * 0.34f, TimerSize.y * 0.27f); // Size scaling relative to Timer size

                ImGui::SetCursorScreenPos(timerindicatorPos);
                ImGui::Image(timerindicatorTexture->Resource, timerindicatorSize, timerindicatorUV0, timerindicatorUV1, ImVec4(1, 1, 1, timerimageAlpha)); // Drawing the frame from the sprite sheet

                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Setting up ImGui Window for the clickable stopwatch on the timer widget
            if (!stopwatchTexture)
            {
                stopwatchTexture = APIDefs->Textures.GetOrCreateFromResource("timer_stopwatch", timer_stopwatch, hSelf);
            }
            if (stopwatchTexture)
            {
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 4.15f, TimerPos.y + Settings::TimerScale * 0.78f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(TimerSize.x * 0.18, TimerSize.y * 0.79));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Timer Stopwatch", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

                // Setting up an invisible button that sits in front of the stopwatch and handling its actions according to the user's settings
                ImGui::InvisibleButton("stopwatch_button", ImVec2(TimerSize.x * 0.17, TimerSize.y * 0.54));
                if (ImGui::IsItemClicked())
                {
                    Vector3 currentPos = MumbleLink->AvatarPosition;
                    if (Settings::optionManual)
                    {
                        if (!timerPaused && timerRunning)
                        {
                            double currElapsed = static_cast<double>(current.QuadPart - timerStart.QuadPart) / frequency.QuadPart;
                            if (currElapsed > 0.0)
                            {
                                timerPaused = true;
                                pausedElapsedSeconds = currElapsed;
                                timerRunning = false;
                                pausePos = currentPos;
                            }
                        }

                        else if (timerPaused)
                        {
                            if (pausedElapsedSeconds > 0.0)
                            {
                                timerPaused = false;
                                timerRunning = false;
                                pausedElapsedSeconds = 0.0;
                                basePos = currentPos;
                            }
                        }
                        else
                        {
                            timerPaused = false;
                            timerRunning = false;
                            pausedElapsedSeconds = 0.0;
                            basePos = currentPos;
                        }
                    }
                    if (Settings::optionCustom)
                    {
                        if (timerHotkeyCount == 4)
                        {
                            timerHotkeyCount = 1;
                        }
                        if (timerHotkeyCount == 3)
                        {
                            timerFinishSet = false;
                            timerStartSet = false;
                            timerPaused = false;
                            timerRunning = false;
                            timerStartEntered = false;
                            basePos = { -10000.0f, -10000.0f, -10000.0f };
                            pausePos = { -10000.0f, -10000.0f, -10000.0f };
                            timerHotkeyCount = 4;
                        }
                        if (timerHotkeyCount == 2)
                        {
                            timerFinishSet = true;
                            pausePos = currentPos;
                            timerHotkeyCount = 3;
                        }
                        if (timerHotkeyCount == 1)
                        {
                            timerStartSet = true;
                            basePos = currentPos;
                            timerHotkeyCount = 2;
                        }
                    }
                    if (Settings::optionPredefined)
                    {
                        if (timerHotkeyCount == 4)
                        {
                            timerHotkeyCount = 3;
                        }
                        else
                        {
                            timerHotkeyCount = 4;
                        }
                    }
                }

                // Handling the animation of the stopwatch by calling each frame from the sprite-sheet with a predetermined delay in succession and on repeat. Pausing the timer will cause the current frame to be picked from the set of green ones instead, and the animation is halted. Resetting the timer to its not running state will only draw a grey version of the first frame.
                double timerframeDelay = 0.033333; // Delay given in seconds
                int timertotalFrames = 91; // Number of frames on the sprite-sheet
                if (timerRunning && !timerPaused)
                {
                    if (!timerAnimationTriggered)
                    {
                        timerAnimationTriggered = true;
                        timerNextFrameTime = ImGui::GetTime() + timerframeDelay;
                    }
                    else if (ImGui::GetTime() >= timerNextFrameTime)
                    {
                        timerFrame = (timerFrame + 1) % (timertotalFrames - 46); // Running the animation using only the golden stopwatch frames
                        timerNextFrameTime = ImGui::GetTime() + timerframeDelay;
                    }
                }
                else if (timerPaused)
                {
                    timerAnimationTriggered = false; // Freezing the animation
                }
                else
                {
                    timerAnimationTriggered = false;
                    timerFrame = 90; // Setting the displayed frame to the grey stopwatch
                }

                // Drawing the frames from the sprite-sheet
                int timernumCols = 9; // Nmber of rows on the sheet
                int timernumRows = 11; // Number of columns on the sheet
                int timerframeRow = (timerPaused ? timerFrame + 45 : timerFrame) / timernumCols; // Determining in which row the frame to pick resides, either one of the golden ones or, if paused, a green one
                int timerframeCol = (timerPaused ? timerFrame + 45 : timerFrame) % timernumCols; // Determining in which column the frame to pick resides, either one of the golden ones or, if paused, a green one
                float timerframeWidth = 1.0f / static_cast<float>(timernumCols); // Calculating the width of a single frame based on the number of columns on the sheet
                float timerframeHeight = 1.0f / static_cast<float>(timernumRows); // Calculating the height of a single frame based on the number of rows on the sheet
                ImVec2 timerUV0(timerframeCol * timerframeWidth, timerframeRow * timerframeHeight); // Calculating the top left pixel of the frame to draw 
                ImVec2 timerUV1((timerframeCol + 1) * timerframeWidth, (timerframeRow + 1) * timerframeHeight); // Calculating the bottom right pixel of the frame to draw 
                ImVec2 stopwatchPos = ImVec2(TimerPos.x + Settings::TimerScale * 4.10f, TimerPos.y + Settings::TimerScale * 0.755f); // Location of the stopwatch on screen, relative to the timer position and scale
                ImVec2 stopwatchSize = ImVec2(TimerSize.x * 0.19f, TimerSize.y * 0.54f); // Size of the stopwatch on screen, relative to the timer scale
                ImGui::SetCursorScreenPos(stopwatchPos);
                ImGui::Image(stopwatchTexture->Resource, stopwatchSize, timerUV0, timerUV1, ImVec4(1, 1, 1, timerimageAlpha));

                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Handling the Timer auxiliary window for the Custom mode
            if (Settings::optionCustom && !timerStartEntered)
            {
                if (Settings::optionTimerRight)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 5.30f - 500.0f, Settings::optionTimerTop ? TimerPos.y - 105.0f : TimerPos.y + Settings::TimerScale * 2.30f + 0.0f), ImGuiCond_Always);
                }
                if (Settings::optionTimerLeft)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 0.16f, Settings::optionTimerTop ? TimerPos.y - 105.0f : TimerPos.y + Settings::TimerScale * 2.30f + 0.0f), ImGuiCond_Always);
                }
                ImGui::SetNextWindowSize(ImVec2(500, 100));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f); // Setzt die Randbreite auf 2 Pixel
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
                ImGui::Begin("Custom Timer info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

                ImGui::PushFont((ImFont*)fontptr);

                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "Custom Timer status info:");
                ImGui::SameLine(350.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Map ID:");
                ImGui::SameLine(430.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%d", MumbleLink->Context.MapID);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Starting location set:");
                ImGui::SameLine();
                if (!timerStartSet)
                {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.6f), " NO");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 0.6f), " YES");
                }
                ImGui::SameLine(350.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "X:");
                ImGui::SameLine(383.0f, 0.0f);

                char posXBuffer[128];
                sprintf_s(posXBuffer, "%.4f", g_currentPos.X);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posXBuffer);


                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Finish location set:");
                ImGui::SameLine();
                if (!timerFinishSet)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.6f), " NO");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 0.6f), " YES");
                }
                ImGui::SameLine(350.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Y:");
                ImGui::SameLine(383.0f, 0.0f);

                char posYBuffer[128];
                sprintf_s(posYBuffer, "%.4f", g_currentPos.Y);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posYBuffer);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Current task:");
                ImGui::SameLine();
                if (!timerStartSet && !timerFinishSet)
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "Set your start!");
                }
                else if (timerStartSet && !timerFinishSet)
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "Set your finish!");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "Enter start area!");
                }
                ImGui::SameLine(350.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Z:");
                ImGui::SameLine(383.0f, 0.0f);

                char posZBuffer[128];
                sprintf_s(posZBuffer, "%.4f", g_currentPos.Z);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posZBuffer);
                ImGui::PopFont();

                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);
            }

            // Handling the contents of the dropdown by filtering them based on Map ID
            if (Settings::optionPredefined)
            {
                currentMapID = MumbleLink->Context.MapID;
                if (currentMapID != lastMapID)
                {
                    lastMapID = currentMapID;
                    Coordinates::UpdateFilteredSetNames(currentMapID);

                    if (Settings::selectedPredefSet >= Coordinates::FilteredSetNames.size())
                    {
                        Settings::selectedPredefSet = 0;
                        Settings::Settings[PREDEF_TIMER_SET] = 0;
                        Settings::Save(SettingsPath);
                    }
                }
            }

            // Handling the two Timer auxiliary windows for the Predefined mode
            if (Settings::optionPredefined && timerHotkeyCount == 3)
            {
                if (Settings::optionTimerRight)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 5.30f - 500.0f, Settings::optionTimerTop ? TimerPos.y - 110.0f : TimerPos.y + Settings::TimerScale * 2.30f + 0.0f), ImGuiCond_Always);
                }
                if (Settings::optionTimerLeft)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 0.16f, Settings::optionTimerTop ? TimerPos.y - 110.0f : TimerPos.y + Settings::TimerScale * 2.30f + 0.0f), ImGuiCond_Always);
                }
                ImGui::SetNextWindowSize(ImVec2(500, 107));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f); // Setzt die Randbreite auf 2 Pixel
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 2.0f));
                ImGui::Begin("Set selection", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

                ImGui::PushFont((ImFont*)fontptr);

                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "Predefined Timer settings:");
                ImGui::SameLine(382.0f, 0.0f);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "Set reload:");

                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Coordinate Sets - Status:");
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                if (Coordinates::SetNames.empty() && !wasJsonMissing)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.6f), " INVALID");
                }
                else if (Coordinates::SetNames.empty() && wasJsonMissing)
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 0.6f), " RECREATED");
                }
                else if (!Coordinates::SetNames.empty() && !wasJsonMissing)
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 0.6f), " OK");
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 0.6f), " The dev is a monkey.");
                }

                ImGui::SameLine(378.0f, 0.0f);


                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f, 0.025f, 0.0f, 1.0f));  // Standardfarbe der Box
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.05f, 0.0f, 1.0f)); // Farbe beim Hover
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.05f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));           // Textfarbe
                if (ImGui::Button("Reload Sets"))
                {
                    if (Settings::IsCloudConfigEnabled && Settings::cloudConfigID != "") {
                        wasJsonMissing = false;
                        Coordinates::LoadCloudConfig();
                        Coordinates::UpdateFilteredSetNames(currentMapID);

                    }
                    else if (std::filesystem::exists(APIDefs->Paths.GetAddonDirectory("Simple Speedometer/coordinates.json"))) {
                        wasJsonMissing = false;
                        Coordinates::Load(CoordinatesPath);
                    }
                }
                ImGui::PopStyleColor(4); // Entfernt die 5 gesetzten Style-Änderungen

                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "Choose a coordinate set:");

                if (!Coordinates::FilteredSetNames.empty())
                {

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f, 0.025f, 0.0f, 1.0f));  // Standardfarbe der Box
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.05f, 0.0f, 1.0f)); // Farbe beim Hover
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.05f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.025f, 0.0f, 1.0f));      // Hintergrund des geschlossenen Dropdowns
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.05f, 0.0f, 1.0f)); // Hover-Farbe
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.05f, 0.0f, 1.0f));  // Aktive Farbe
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));           // Textfarbe
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));         // Randfarbe
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Hintergrund des Dropdown-Menüs
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.05f, 0.025f, 0.0f, 1.0f)); // Ausgewählte Option: Orange
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.05f, 0.0f, 1.0f)); // Hover über Option: Hellorange
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.075f, 0.0f, 1.0f));

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 1.0f);

                    if (ImGui::BeginCombo("##Set Auswahl", Coordinates::FilteredSetNames[Settings::selectedPredefSet].c_str()))
                    {
                        for (size_t i = 0; i < Coordinates::FilteredSetNames.size(); i++)
                        {
                            bool isSelected = (Settings::selectedPredefSet == i);
                            if (ImGui::Selectable(Coordinates::FilteredSetNames[i].c_str(), isSelected))
                            {
                                Settings::selectedPredefSet = i;
                                Settings::Settings[PREDEF_TIMER_SET] = Settings::selectedPredefSet;
                                Settings::Save(SettingsPath);
                                timerPaused = false;
                                timerRunning = false;
                                timerStartEntered = false;
                                currentCheckpointIndex = 0;
                                checkpointlistfinished = false;
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopStyleColor(12); // Entfernt die 5 gesetzten Style-Änderungen
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.0f, 0.0f, 1.0f), "No sets for this map ID available.");
                }
                ImGui::PopFont();

                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);

                if (Settings::optionTimerRight)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 5.30f - 150.0f, Settings::optionTimerTop ? TimerPos.y - 222.0f : TimerPos.y + Settings::TimerScale * 2.30f + 121.0f), ImGuiCond_Always);
                }
                if (Settings::optionTimerLeft)
                {
                    ImGui::SetNextWindowPos(ImVec2(TimerPos.x + Settings::TimerScale * 0.16f, Settings::optionTimerTop ? TimerPos.y - 222.0f : TimerPos.y + Settings::TimerScale * 2.30f + 121.0f), ImGuiCond_Always);
                }
                ImGui::SetNextWindowSize(ImVec2(150, 100));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f); // Setzt die Randbreite auf 2 Pixel
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 2.0f));
                ImGui::Begin("Map info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                ImGui::PushFont((ImFont*)fontptr);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Map ID:");
                ImGui::SameLine(80.0f, 0.0f);

                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%d", MumbleLink->Context.MapID);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "X:");
                ImGui::SameLine(33.0f, 0.0f);

                char posXBuffer[128];
                sprintf_s(posXBuffer, "%.4f", g_currentPos.X);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posXBuffer);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Y:");
                ImGui::SameLine(33.0f, 0.0f);

                char posYBuffer[128];
                sprintf_s(posYBuffer, "%.4f", g_currentPos.Y);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posYBuffer);

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Z:");
                ImGui::SameLine(33.0f, 0.0f);

                char posZBuffer[128];
                sprintf_s(posZBuffer, "%.4f", g_currentPos.Z);
                ImGui::TextColored(ImVec4(0.5f, 0.25f, 0.1f, 1.0f), "%s", posZBuffer);
                ImGui::PopFont();

                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);


            }
        }
    }
}

// Main function for the rendering of the Speedometer as well as calling the functions related to the calculations of the speedometer, the timer and the rendering of the latter
void AddonRender()
{
    UpdateSpeed();

    UpdateTimer();
    RenderTimerWindow();

    if (NexusLink && MumbleLink && MumbleIdentity && !MumbleLink->Context.IsMapOpen && NexusLink->IsGameplay)
    {
        if (Settings::IsDialEnabled)
        {
            // Setting size and position of the speedometer
            ImVec2 speedometerPos = ImVec2(Settings::DialPositionH, Settings::DialPositionV);
            ImVec2 dialSize = ImVec2(Settings::DialScale * 5.60f, Settings::DialScale * 5.00f);

            // Handling transparency on mouseover
            ImVec2 mousePos = ImGui::GetMousePos();
            bool isMouseOver = (mousePos.x >= speedometerPos.x && mousePos.x <= speedometerPos.x + dialSize.x && mousePos.y >= speedometerPos.y && mousePos.y <= speedometerPos.y + dialSize.y);
            float targetAlpha = isMouseOver ? 0.2f : 1.0f;
            imageAlpha += (targetAlpha - imageAlpha) * fadeSpeed;

            // Setting up ImGui Window for the Speedometer dial
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::SetNextWindowPos(ImVec2(speedometerPos), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(dialSize));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Speedometer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);
            if (speedometerTexture)
            {
                ImGui::Image(speedometerTexture->Resource, dialSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, imageAlpha));
            }
            else
            {
                speedometerTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_background", speedometer_background, hSelf);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);

            // Speed calculation according to settings and variables for the max amplitude / exceeding speed animations on needle and numbers
            int speed = static_cast<int>(roundf((Settings::option2D ? lastSpeed2D : lastSpeed3D) * (Settings::optionUnits ? conversionFactor_u_s : (Settings::optionFeetPercent ? conversionFactor_foot : conversionFactor_beetle)))); // Calculating the concurrent speed
            float minAngle = -18.0f; // Minimum needle angle
            float maxAngle = 99.0f; // Maximum needle angle
            float needleLimitUnits = Settings::amplitudeUnits; // Needle max amp for units
            float needleLimitFeetPercent = Settings::amplitudeFeetPercent; // Needle max amp for feet
            float needleLimitBeetlePercent = Settings::amplitudeBeetlePercent; // Needle max amp for beetle
            float needleLimit = (Settings::optionUnits ? needleLimitUnits : (Settings::optionFeetPercent ? needleLimitFeetPercent : needleLimitBeetlePercent) + 0.01); // Whether the needle max amp for units, feet or beetle is to be used

            // Setting up ImGui Windows for the Speedometer needle and max amp animations
            if (!needleTexture or !blueneedleTexture)
            {
                needleTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_needle", speedometer_needle, hSelf);
                blueneedleTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_needle_blue", speedometer_needle_blue, hSelf);
            }
            if (needleTexture and blueneedleTexture)
            {

                // Render max amp sparking from needle pivot underneath the needle
                if (!fireTexture)
                {
                    fireTexture = APIDefs->Textures.GetOrCreateFromResource("fire_sparks", fire_sparks, hSelf);
                }
                if (fireTexture)
                {
                    ImGui::SetNextWindowBgAlpha(0.0f);
                    ImGui::SetNextWindowPos(ImVec2(speedometerPos.x + Settings::DialScale * 0.0f, speedometerPos.y + Settings::DialScale * 0.0f), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(dialSize));

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin("fire Sparks", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);
                    double fireframeDelay = 0.008333; // Delay in seconds to draw each sprite frame
                    int firetotalFrames = 32;  // Total sprite frames to be drawn

                    // Setting the rules for when to play the animation
                    if (speed > needleLimit)
                    {
                        if (!fireAnimationTriggered)
                        {
                            fireAnimationTriggered = true;
                            fireAnimationFinished = false;
                            fireFrame = 0;
                            fireNextFrameTime = ImGui::GetTime() + fireframeDelay;
                        }
                        else if (!fireAnimationFinished && ImGui::GetTime() >= fireNextFrameTime)
                        {
                            fireFrame++;
                            if (fireFrame >= firetotalFrames)
                            {
                                fireFrame = 0;
                                fireAnimationFinished = true;
                            }
                            fireNextFrameTime = ImGui::GetTime() + fireframeDelay;
                        }
                    }
                    else
                    {
                        fireAnimationTriggered = false;
                        fireAnimationFinished = false;
                        fireFrame = 0;
                    }

                    // Draw sprite animation if triggered
                    if (fireAnimationTriggered)
                    {
                        int firenumCols = 8; // Number of colums of the sprite sheet
                        int firenumRows = 4; // Number of rows of the sprite sheet
                        int fireframeCol = fireFrame % firenumCols; // Calculating the active column
                        int fireframeRow = fireFrame / firenumCols; // Calculating the active row
                        float fireframeWidth = 1.0f / static_cast<float>(firenumCols); // Calculating the width of each frame
                        float fireframeHeight = 1.0f / static_cast<float>(firenumRows); // Calculating the height of each frame

                        ImVec2 fireUV0(fireframeCol * fireframeWidth, fireframeRow * fireframeHeight); // Calculating upper left pixel coordinate of the frame
                        ImVec2 fireUV1((fireframeCol + 1) * fireframeWidth, (fireframeRow + 1) * fireframeHeight); // Calculating lower right pixel coordinate of the frame

                        ImVec2 firePos = ImVec2(speedometerPos.x + Settings::DialScale * 2.490f, speedometerPos.y + Settings::DialScale * 1.070f); // Positioning relative to the Speedometer position
                        ImVec2 fireSize = ImVec2(dialSize.x * 0.5f, dialSize.y * 0.8f); // Size scaling relative to Speedometer size

                        ImGui::SetCursorScreenPos(firePos);
                        ImGui::Image(fireTexture->Resource, fireSize, fireUV0, fireUV1, ImVec4(1, 1, 1, imageAlpha)); // Drawing the frame from the sprite sheet
                    }
                    ImGui::End();
                    ImGui::PopStyleVar(2);
                }

                // Render speedometer needle
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(speedometerPos), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(dialSize));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Needle", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                ImDrawList* drawList = ImGui::GetWindowDrawList(); // Getting current draw list 
                ImVec2 needlePivot = ImVec2(speedometerPos.x + dialSize.x * 0.565f, speedometerPos.y + dialSize.y * 0.630f); // Setting the pivot point of the needle on the Speedometer

                float clampedSpeed = speed;
                if (speed > needleLimit) clampedSpeed = needleLimit; // Clamping the needle to the currently set max speed at its max angle
                float targetAngle = minAngle + (maxAngle - minAngle) * (clampedSpeed / needleLimit); // Calculating the target angle for the current speed reading

                // Fancy needle twitching animation as if it tries to exceed its turning limits
                if (speed > needleLimit)
                {
                    float amplitude = 4.0f;
                    float frequency = 15.0f;
                    targetAngle = maxAngle + sin(ImGui::GetTime() * frequency * 2.0f * 3.14159265f) * amplitude;
                }

                // Smoothing the needle movements
                static float currentAngle = minAngle;
                const float smoothingFactor = 0.5f;
                currentAngle += (targetAngle - currentAngle) * smoothingFactor;

                float rad = currentAngle * (3.14159265f / 180.0f); // Calculating the target angle for the needle movement
                ImVec2 needleCenter = ImVec2(dialSize.x * 0.52680f, dialSize.y * 0.05817f); // Defining the position of the needle pivot point on the needle texture

                // Calculating the four corners of the needle texture, this affects its shape
                ImVec2 p1 = ImVec2(-needleCenter.x, -needleCenter.y);
                ImVec2 p2 = ImVec2(dialSize.x * 0.57636f - needleCenter.x, -needleCenter.y);
                ImVec2 p3 = ImVec2(dialSize.x * 0.57636f - needleCenter.x, dialSize.y * 0.11633f - needleCenter.y);
                ImVec2 p4 = ImVec2(-needleCenter.x, dialSize.y * 0.11633f - needleCenter.y);

                // Rotating the needle
                auto rotate = [&](ImVec2 p) -> ImVec2 {return ImVec2(needlePivot.x + cos(rad) * p.x - sin(rad) * p.y, needlePivot.y + sin(rad) * p.x + cos(rad) * p.y);};

                // Calculate the positions the positions of the four corners of the needle texture
                ImVec2 r1 = rotate(p1);
                ImVec2 r2 = rotate(p2);
                ImVec2 r3 = rotate(p3);
                ImVec2 r4 = rotate(p4);

                ImU32 col = IM_COL32(255, 255, 255, (int)(imageAlpha * 255)); // Calculating the color (white with imageAlpha)

                if (speed > needleLimit)
                {
                    drawList->PushTextureID(blueneedleTexture->Resource); // Pushing the texture to the drawlist
                }
                else
                {
                    drawList->PushTextureID(needleTexture->Resource); // Pushing the texture to the drawlist
                }

                // Extending the vertex and index buffer with four vertices and six indices
                int vtx_current_size = drawList->VtxBuffer.Size;
                int idx_current_size = drawList->IdxBuffer.Size;
                drawList->PrimReserve(6, 4);

                // Writing the four vertex datapoints (position, UV coordinates, color)
                ImDrawVert* vtx = drawList->VtxBuffer.Data + vtx_current_size;
                vtx[0].pos = r1;  vtx[0].uv = ImVec2(0.0f, 0.0f);  vtx[0].col = col;
                vtx[1].pos = r2;  vtx[1].uv = ImVec2(1.0f, 0.0f);  vtx[1].col = col;
                vtx[2].pos = r3;  vtx[2].uv = ImVec2(1.0f, 1.0f);  vtx[2].col = col;
                vtx[3].pos = r4;  vtx[3].uv = ImVec2(0.0f, 1.0f);  vtx[3].col = col;

                // Writing the six indices for two trianggles (Quad = [0,1,2] und [0,2,3])
                ImDrawIdx* idx = drawList->IdxBuffer.Data + idx_current_size;
                idx[0] = (ImDrawIdx)(vtx_current_size + 0);
                idx[1] = (ImDrawIdx)(vtx_current_size + 1);
                idx[2] = (ImDrawIdx)(vtx_current_size + 2);
                idx[3] = (ImDrawIdx)(vtx_current_size + 0);
                idx[4] = (ImDrawIdx)(vtx_current_size + 2);
                idx[5] = (ImDrawIdx)(vtx_current_size + 3);

                drawList->PopTextureID(); // Releasing the texture from the drawlist
                ImGui::End();
                ImGui::PopStyleVar(2);

                // Render max amp sparking on top of the needle pivot
                if (!pivotTexture)
                {
                    pivotTexture = APIDefs->Textures.GetOrCreateFromResource("pivot_sparks", pivot_sparks, hSelf);
                }
                if (pivotTexture)
                {
                    ImGui::SetNextWindowBgAlpha(0.0f);
                    ImGui::SetNextWindowPos(ImVec2(speedometerPos.x + Settings::DialScale * 0.0f, speedometerPos.y + Settings::DialScale * 0.0f), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(dialSize));

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin("Pivot Sparks", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                    double pivotframeDelay = 0.05; // Delay in seconds to draw each sprite frame
                    int pivottotalFrames = 8;  // Total sprite frames to be drawn

                    // Setting the rules for when to play the animation
                    if (speed > needleLimit)
                    {
                        if (!pivotAnimationTriggered)
                        {
                            pivotAnimationTriggered = true;
                            pivotFrame = 0;
                            pivotNextFrameTime = ImGui::GetTime() + pivotframeDelay;
                        }
                        else if (ImGui::GetTime() >= pivotNextFrameTime)
                        {
                            pivotFrame = (pivotFrame + 1) % pivottotalFrames;
                            pivotNextFrameTime = ImGui::GetTime() + pivotframeDelay;
                        }
                    }
                    else
                    {
                        pivotAnimationTriggered = false;
                        pivotFrame = 0;
                    }

                    // Draw sprite animation if triggered
                    if (pivotAnimationTriggered)
                    {
                        int pivotnumCols = 4; // Number of colums of the sprite sheet
                        int pivotnumRows = 2; // Number of rows of the sprite sheet
                        int pivotframeRow = pivotFrame / pivotnumCols; // Calculating the active column
                        int pivotframeCol = pivotFrame % pivotnumCols; // Calculating the active row
                        float pivotframeWidth = 1.0f / static_cast<float>(pivotnumCols); // Calculating the width of each frame
                        float pivotframeHeight = 1.0f / static_cast<float>(pivotnumRows); // Calculating the height of each frame

                        ImVec2 pivotUV0(pivotframeCol * pivotframeWidth, pivotframeRow * pivotframeHeight); // Calculating upper left pixel coordinate of the frame
                        ImVec2 pivotUV1((pivotframeCol + 1) * pivotframeWidth, (pivotframeRow + 1) * pivotframeHeight); // Calculating lower right pixel coordinate of the frame

                        ImVec2 pivotPos = ImVec2(speedometerPos.x + Settings::DialScale * 2.82f, speedometerPos.y + Settings::DialScale * 2.83f); // Positioning relative to the Speedometer position
                        ImVec2 pivotSize = ImVec2(dialSize.x * 0.13f, dialSize.y * 0.13f); // Size scaling relative to Speedometer size

                        ImGui::SetCursorScreenPos(pivotPos);
                        ImGui::Image(pivotTexture->Resource, pivotSize, pivotUV0, pivotUV1, ImVec4(1, 1, 1, imageAlpha)); // Drawing the frame from the sprite sheet
                    }
                    ImGui::End();
                    ImGui::PopStyleVar(2);
                }
            }

            //Setting up the numerical display on the Speedometer dial
            if (!numberTexture)
            {
                numberTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_numbers", speedometer_numbers, hSelf);
            }
            if (numberTexture)
            {
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(speedometerPos.x + Settings::DialScale * 3.58f, speedometerPos.y + Settings::DialScale * 1.55f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(dialSize));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Numbers", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                // Turning the concurrent speed into a four-piece string with leading zeroes
                char speedStr[5];
                snprintf(speedStr, sizeof(speedStr), "%04d", speed);
                bool leadingZero = true;

                // Looping through the four digits
                for (int i = 0; i < 4; i++)
                {
                    int digit = speedStr[i] - '0'; // Getting the digit
                    int digitRow;

                    // Picking which row on the sprite sheet to pick the number from
                    if (leadingZero && digit == 0)
                    {
                        digitRow = 2;
                    }
                    else
                    {
                        leadingZero = false;
                        digitRow = (speed > needleLimit) ? 1 : 0;
                    }

                    float digitWidth = 1.0f / 12.0f; // Width of each number determined as a divider of the total width of the sprite sheet
                    float digitHeight = 1.0f / 5.0f; // Heigth of each number determined as a divider of the total width of the sprite sheet


                    ImVec2 digitUV0(digit * digitWidth, digitRow * digitHeight); // Calculating upper left pixel coordinate of the frame
                    ImVec2 digitUV1((digit + 1) * digitWidth, (digitRow + 1) * digitHeight); // Calculating lower right pixel coordinate of the frame

                    ImGui::Image(numberTexture->Resource, ImVec2(Settings::DialScale * 0.45f, Settings::DialScale * 0.69f), digitUV0, digitUV1, ImVec4(1, 1, 1, imageAlpha)); // Drawing the frame from the sprite sheet

                    // Calculating offsets to place the last three numbers relative to the first
                    if (i == 0)
                        ImGui::SameLine(Settings::DialScale * 0.396f, 0.0f);
                    if (i == 1)
                        ImGui::SameLine(Settings::DialScale * 0.786f, 0.0f);
                    if (i == 2)
                        ImGui::SameLine(Settings::DialScale * 1.176f, 0.0f);
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Setting up the dimension and unit displays on the Speedometer dial
            if (!indicatorTexture)
            {
                indicatorTexture = APIDefs->Textures.GetOrCreateFromResource("speedometer_indicators", speedometer_indicators, hSelf);
            }
            if (indicatorTexture)
            {
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::SetNextWindowPos(ImVec2(speedometerPos.x + Settings::DialScale * 0.0f, speedometerPos.y + Settings::DialScale * 0.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(dialSize));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Dimensions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                int dimensionFrame = Settings::option2D ? 0 : 1; // Picking a specific frame from the sprite sheet based on settings - count starts from 0 for frame 1
                int dimensionnumCols = 4; // Number of colums of the sprite sheet
                int dimensionnumRows = 2; // Number of rows of the sprite sheet
                int dimensionframeCol = dimensionFrame % dimensionnumCols; // Calculating the active column
                int dimensionframeRow = dimensionFrame / dimensionnumCols; // Calculating the active row
                float dimensionframeWidth = 1.0f / static_cast<float>(dimensionnumCols); // Calculating the width of each frame
                float dimensionframeHeight = 1.0f / static_cast<float>(dimensionnumRows); // Calculating the height of each frame

                ImVec2 dimensionUV0(dimensionframeCol * dimensionframeWidth, dimensionframeRow * dimensionframeHeight); // Calculating upper left pixel coordinate of the frame
                ImVec2 dimensionUV1((dimensionframeCol + 1) * dimensionframeWidth, (dimensionframeRow + 1) * dimensionframeHeight); // Calculating lower right pixel coordinate of the frame

                ImVec2 dimensionPos = ImVec2(speedometerPos.x + Settings::DialScale * 3.63f, speedometerPos.y + Settings::DialScale * 2.81f); // Positioning relative to the Speedometer position
                ImVec2 dimensionSize = ImVec2(dialSize.x * 0.28f, dialSize.y * 0.125f); // Size scaling relative to Speedometer size

                ImGui::SetCursorScreenPos(dimensionPos);
                ImGui::Image(indicatorTexture->Resource, dimensionSize, dimensionUV0, dimensionUV1, ImVec4(1, 1, 1, imageAlpha)); // Drawing the frame from the sprite sheet

                int unitFrame = Settings::optionFeetPercent ? 3 : (Settings::optionBeetlePercent ? 4 : 5); // Picking a specific frame from the sprite sheet based on settings - count starts from 0 for frame 1
                int unitnumCols = 3; // Number of colums of the sprite sheet
                int unitnumRows = 2; // Number of rows of the sprite sheet
                int unitframeRow = unitFrame / unitnumCols; // Calculating the active column
                int unitframeCol = unitFrame % unitnumCols; // Calculating the active row
                float unitframeWidth = 1.0f / static_cast<float>(unitnumCols); // Calculating upper left pixel coordinate of the frame
                float unitframeHeight = 1.0f / static_cast<float>(unitnumRows); // Calculating the height of each frame

                ImVec2 unitUV0(unitframeCol * unitframeWidth, unitframeRow * unitframeHeight); // Calculating upper left pixel coordinate of the frame
                ImVec2 unitUV1((unitframeCol + 1) * unitframeWidth, (unitframeRow + 1) * unitframeHeight); // Calculating the height of each frame

                ImVec2 unitPos = ImVec2(speedometerPos.x + Settings::DialScale * 3.01f, speedometerPos.y + Settings::DialScale * 3.955f); // Positioning relative to the Speedometer position
                ImVec2 unitSize = ImVec2(dialSize.x * 0.29f, dialSize.y * 0.13f); // Size scaling relative to Speedometer size

                ImGui::SetCursorScreenPos(unitPos);
                ImGui::Image(indicatorTexture->Resource, unitSize, unitUV0, unitUV1, ImVec4(1, 1, 1, imageAlpha)); // Drawing the frame from the sprite sheet

                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Setting up the Speedometer-attached data chart
            if (Settings::IsTableEnabled)
            {
                ImGui::SetNextWindowPos(ImVec2(speedometerPos.x + Settings::DialScale * 5.42f - 260.0f, speedometerPos.y - 250.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(260.0f, 248.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f); // Setzt die Randbreite auf 2 Pixel
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
                ImGui::Begin("##Data chart", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs );

                ImGui::PushFont((ImFont*)fontptr);

                ImGui::BeginTable("SpeedTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 175.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableHeadersRow();

                auto AlignRight = [](const char* text) {
                    float posX = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text).x - ImGui::GetScrollX() - 0 * ImGui::GetStyle().ItemSpacing.x;
                    if (posX > ImGui::GetCursorPosX())
                        ImGui::SetCursorPosX(posX);
                    ImGui::Text("%s", text);
                    };

                struct SpeedEntry {
                    const char* label;
                    float value;
                    const char* unit;
                };

                SpeedEntry speedEntries[] = {
                    {"2D speed, u/sec:", lastSpeed2D * conversionFactor_u_s},
                    {"3D speed, u/sec:", lastSpeed3D * conversionFactor_u_s},
                    {"2D speed, Feet%:", lastSpeed2D * conversionFactor_foot},
                    {"3D speed, Feet%:", lastSpeed3D * conversionFactor_foot},
                    {"2D speed, Btl.%:", lastSpeed2D * conversionFactor_beetle},
                    {"3D speed, Btl.%:", lastSpeed3D * conversionFactor_beetle},
                    {"Dist. from Start:", (Settings::optionCustom && !timerStartSet) ? 0 : ((Settings::optionPredefined && Coordinates::FilteredSetNames.empty()) ? 0 : (g_basedistance + 0.381f) * conversionFactor_u_s - (Settings::optionCustom ? Settings::startDiameter : (Settings::optionPredefined ? startRadius : Settings::manualstartDiameter)))},
                    {"Dist. to Checkp.:", (Settings::optionPredefined && !Coordinates::FilteredSetNames.empty() && !checkpointlistfinished) ? ((g_checkpointdistance - 0.381f) * conversionFactor_u_s - checkpointRadius) : 0},
                    {"Dist. to Finish:", Settings::optionManual ? 0 : (Settings::optionCustom && !timerFinishSet) ? 0 : (((Settings::optionPredefined && Coordinates::FilteredSetNames.empty()) ? 0 : (g_pausedistance - 0.381f)) * conversionFactor_u_s - (Settings::optionCustom ? Settings::finishDiameter : (Settings::optionPredefined && Coordinates::FilteredSetNames.empty()) ? 0 : endRadius))}
                    //{"X Pos.(W-E):", g_currentPos.X},
                    //{"Y Pos.(Height):", g_currentPos.Y},
                    //{"Z Pos.(N-S):", g_currentPos.Z},
                    //{"Map ID:", MumbleLink->Context.MapID}
                };

                for (const auto& entry : speedEntries)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::Text("%s", entry.label);
                    ImGui::PopStyleColor();
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.25f, 0.1f, 1.0f));

                    char buffer[16];

                    // Prüfe, ob der Eintrag eine Koordinate ist
                    if (strstr(entry.label, "Pos.") != nullptr)
                    {
                        snprintf(buffer, sizeof(buffer), "%.4f", entry.value);  // 4 Nachkommastellen für X, Y, Z
                    }
                    else
                    {
                        snprintf(buffer, sizeof(buffer), "%4d", static_cast<int>(roundf(entry.value)));  // Ganze Zahl für andere Werte
                    }

                    AlignRight(buffer);
                    ImGui::PopStyleColor();
                }
                ImGui::EndTable();

                ImGui::PopFont();

                ImGui::End();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);
            }
        }

        /*
        if (Settings::IsReadMeEnabled)
        {
            ImGui::SetNextWindowBgAlpha(0.95f);
            ImGui::SetNextWindowPos(ImVec2(Settings::ReadMePositionH, Settings::ReadMePositionV), ImGuiCond_Appearing);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("ReadMe", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImVec2 maxSize = ImGui::GetContentRegionMax();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Welcome to the Simple Speedometer!");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "This shall serve as a short guide on what it is, what it does and how to use it.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "1.: Features");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "This plugin comes with both a Speedometer for velocity displaying and a Timer with a location-based starting trigger.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Both widgets can be turned on and off as well as be moved and resized independantly from each other.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Use the settings tab of this plugin in Nexus to configure it to your liking.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "2.: Speedometer");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "The Speedometer can measure your velocity in 2D or 3D space, each with a choice of one of three units.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Choose u/s for a measurement of velocity using the standard ingame unit for distance, that being, well, Units.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Choose Feet%% for a measurement of velocity relative to standard out of combat walking speed, which is set to 100%%.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Choose Beetle%% for a measurement of velocity relative to the normal maximum speed of the Roller Beetle, which is set to 99%%.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "You may configure the maximum amplitude of the Speedometer needle for each of those units.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "A chart containing all six possible measurement combinations may be opened on top of the Speedometer.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "The speedometer widget is designed to be click-through so as to not impede gameplay, and it will fade on mouse-over.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "A customizable keybind is available to show or hide the Speedometer widget on demand.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "3.: Timer");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "The Timer starts counting minutes, seconds and milliseconds once you leave the starting location displayed by a white circle.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "The orange circle around your feet represents your own location. Once part of the orange circle exceeds the white one, timing starts.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "The Timer will automatically reset to 00:00.000 upon reentering the white circle, so be sure to stay out to prevent accidental resets.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "You may pause the timer by clicking the stopwatch or using another customizable keybind when standing still. Timing will resume on movement.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "A second click or press of the keybind while paused will manually reset the timer to 00:00.000 and set the starting location at your feet.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "A customizable keybind is available to show or hide the Timer widget on demand, but be cautious as this also resets the Timer! ");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "The Timer widget doesn't fade on mouse-over, but aside from the stopwatch, it, too, is click-through and won't intercept.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "4.: Limitations");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Calculations are done each in each frame of GW2. Low framerate results in low amount of calculations.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "This will affect responsiveness of the displays and animations, and going below 25 fps or having bad hiccups may spike the Speedometer.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Timing is accurate and updated on every frame, but ultimately reliant on frametime and Mumble-API update rate.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "The Timer is meant to be an easy way of having a recordable time reference without having to rely on external means, like overlays.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "It is not designed to stop counting upon reaching a specific goal. You'll have to check the time given on that specific frame.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Unfortunately I was not able to find a way to setup scalability properly for all the different elements.");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "If you look too closely, there will be slight sliding of elements when scaling. I tried my best to mitigate that.");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "5.: Thanks and have fun!");
            ImGui::Separator();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "I built this for fun and with big help of ChatGPT. I hope you find enjoyment in using this plugin!");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Have a nice day, and be fast.");
            ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "--Toxxa");

            ImGui::End();
            ImGui::PopStyleVar(2);
        }
        */
    }
}

// Handling the keybind that toggles the Speedometer
void SpeedometerToggleVisibility(const char* aIdentifier, bool aIsRelease)
{
    if ((strcmp(aIdentifier, "Toggle Speedometer visibility") == 0) && !aIsRelease)
    {
        if (Settings::IsDialEnabled)
        {
            Settings::IsDialEnabled = false;
        }
        else
        {
            Settings::IsDialEnabled = true;
        }
        Settings::Settings[IS_SPEEDOMETER_DIAL_VISIBLE] = Settings::IsDialEnabled;
        Settings::Save(SettingsPath);
    }
}

// Handling the keybind that toggles the Timer
void TimerToggleVisibility(const char* aIdentifier, bool aIsRelease)
{
    if ((strcmp(aIdentifier, "Toggle Timer visibility") == 0) && !aIsRelease)
    {
        if (Settings::IsTimerEnabled)
        {
            Settings::IsTimerEnabled = false;
            timerRunning = false;
            timerPaused = false;
            basePosInitialized = false;
        }
        else
        {
            Settings::IsTimerEnabled = true;
        }
        Settings::Settings[IS_SPEEDOMETER_TIMER_VISIBLE] = Settings::IsTimerEnabled;
        Settings::Save(SettingsPath);
    }
}

// Handling the keybind that pauses or resets the Timer
void TimerPauseReset(const char* aIdentifier, bool aIsRelease)
{
    if ((strcmp(aIdentifier, "Pause or reset the Timer / set locations / toggle selection window") == 0) && !aIsRelease)
    {
        Vector3 currentPos = MumbleLink->AvatarPosition;
        QueryPerformanceCounter(&current);

        if (Settings::optionManual)
        {
            if (!timerPaused && timerRunning)
            {
                double currElapsed = static_cast<double>(current.QuadPart - timerStart.QuadPart) / frequency.QuadPart;
                if (currElapsed > 0.0)
                {
                    timerPaused = true;
                    pausedElapsedSeconds = currElapsed;
                    timerRunning = false;
                    pausePos = currentPos;
                }
            }

            else if (timerPaused)
            {
                if (pausedElapsedSeconds > 0.0)
                {
                    timerPaused = false;
                    timerRunning = false;
                    pausedElapsedSeconds = 0.0;
                    basePos = currentPos;
                }
            }
            else
            {
                timerPaused = false;
                timerRunning = false;
                pausedElapsedSeconds = 0.0;
                basePos = currentPos;
            }
        }
        if (Settings::optionCustom)
        {
            if (timerHotkeyCount == 4)
            {
                timerHotkeyCount = 1;
            }
            if (timerHotkeyCount == 3)
            {
                timerFinishSet = false;
                timerStartSet = false;
                timerPaused = false;
                timerRunning = false;
                timerStartEntered = false;
                basePos = { -10000.0f, -10000.0f, -10000.0f };
                pausePos = { -10000.0f, -10000.0f, -10000.0f };
                timerHotkeyCount = 4;
            }
            if (timerHotkeyCount == 2)
            {
                timerFinishSet = true;
                pausePos = currentPos;
                timerHotkeyCount = 3;
            }
            if (timerHotkeyCount == 1)
            {
                timerStartSet = true;
                basePos = currentPos;
                timerHotkeyCount = 2;
            }
        }
        if (Settings::optionPredefined)
        {
            if (timerHotkeyCount == 4)
            {
                timerHotkeyCount = 3;
            }
            else
            {
                timerHotkeyCount = 4;
            }
        }
    }
}

void OpenURL(const std::string& url) {
#ifdef _WIN32
    ShellExecute(0, "open", url.c_str(), 0, 0, SW_SHOWNORMAL);
#elif __APPLE__
    std::string command = "open " + url;
    system(command.c_str());
#elif __linux__
    std::string command = "xdg-open " + url;
    system(command.c_str());
#endif
}


// Realm of the Settings
void AddonOptions()
{
    /*
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Speedometer and Timer ingame ReadMe:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Toggle the ingame ReadMe on or off");
    if (ImGui::Checkbox("Show ReadMe", &Settings::IsReadMeEnabled))
    {
        if (!Settings::IsReadMeEnabled)
        {
            Settings::ReadMePositionH = 450.0f;
            Settings::ReadMePositionV = 250.0f;
        }
        Settings::Settings[IS_READ_ME_VISIBLE] = Settings::IsReadMeEnabled;
        Settings::Settings[READ_ME_POSITION_H] = Settings::DialPositionH;
        Settings::Settings[READ_ME_POSITION_V] = Settings::DialPositionV;
        Settings::Save(SettingsPath);
    }
    */

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Configure Cloud Config:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Cloud Configs can be shared and edited online");
    if (ImGui::Checkbox("Enable Cloud Config", &Settings::IsCloudConfigEnabled))
    {
        if (!Settings::IsCloudConfigEnabled) Settings::IsCloudConfigEnabled = false;
        Settings::Settings[IS_CLOUDCONFIG_ENABLED] = Settings::IsCloudConfigEnabled;
        if (Settings::cloudConfigID == "") {
            Settings::cloudConfigID = Settings::GenerateRandomString(32);
            Settings::Settings[CLOUDCONFIG_ID] = Settings::cloudConfigID;
        }
        Settings::Save(SettingsPath);
    }

    static char cloudConfigID[256] = ""; // Buffer for input text
    strncpy_s(cloudConfigID, Settings::cloudConfigID.c_str(), sizeof(Settings::cloudConfigID));
    cloudConfigID[sizeof(cloudConfigID) - 1] = '\0';
    if (Settings::IsCloudConfigEnabled)
    {
        ImGui::Text("Enter Cloud Config ID:");
        if (ImGui::InputText("##cloudConfigID", cloudConfigID, IM_ARRAYSIZE(cloudConfigID)))
        {
            // Save the entered cloudConfigID to settings
            Settings::cloudConfigID = std::string(cloudConfigID);
            Settings::Settings[CLOUDCONFIG_ID] = Settings::cloudConfigID;
            Settings::Save(SettingsPath);
        }
        if (ImGui::Button("Open Cloud Config Page")) {
            OpenURL("https://speedometer.cloudflare8462.workers.dev/" + Settings::cloudConfigID);
        }
    }

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Speedometer visibility settings:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Toggle visibility on or off");
    if (ImGui::Checkbox("Show Speedometer", &Settings::IsDialEnabled))
    {
        if (!Settings::IsDialEnabled) Settings::IsTableEnabled = false;
        Settings::Settings[IS_SPEEDOMETER_DIAL_VISIBLE] = Settings::IsDialEnabled;
        Settings::Settings[IS_SPEEDOMETER_TABLE_VISIBLE] = Settings::IsTableEnabled;
        Settings::Save(SettingsPath);
    }

    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Show data chart", &Settings::IsTableEnabled))
    {
        if (!Settings::IsDialEnabled) Settings::IsDialEnabled = true;
        Settings::Settings[IS_SPEEDOMETER_DIAL_VISIBLE] = Settings::IsDialEnabled;
        Settings::Settings[IS_SPEEDOMETER_TABLE_VISIBLE] = Settings::IsTableEnabled;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "This chart only exists to provide a quick glance at all possible velocity values and your distance to the start circle border.");
        ImGui::EndTooltip();
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Use the sliders to adjust the position and scale of the Speedometer:");
    if (ImGui::SliderFloat("hPos Speedometer", &Settings::DialPositionH, NexusLink->Width * 0.0f, NexusLink->Width * 1.0f, "%.0f X%"))
    {
        Settings::Settings[SPEEDOMETER_DIAL_POSITION_H] = Settings::DialPositionH;
        Settings::Save(SettingsPath);
    }

    if (ImGui::SliderFloat("vPos Speedometer", &Settings::DialPositionV, NexusLink->Height * 0.0f, NexusLink->Height * 1.0f, "%.0f Y%"))
    {
        Settings::Settings[SPEEDOMETER_DIAL_POSITION_V] = Settings::DialPositionV;
        Settings::Save(SettingsPath);
    }

    if (ImGui::SliderFloat("Scale between 25% and 125%", &Settings::DialScale, 25.0f, 125.0f, "%.0f %%"))
    {
        Settings::Settings[SPEEDOMETER_DIAL_SCALE] = Settings::DialScale;
        Settings::Save(SettingsPath);
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Speedometer measuring settings:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Set the dimension that velocity gets measured in:");
    if (ImGui::Checkbox("2D", &Settings::option2D))
    {
        if (Settings::option2D) Settings::option3D = false;
        if (!Settings::option2D) Settings::option3D = true;
        Settings::Settings[IS_OPTION_2D_ENABLED] = Settings::option2D;
        Settings::Settings[IS_OPTION_3D_ENABLED] = Settings::option3D;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Measure speed in 2D space. Best suitable for JPs and Beetle racing.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("3D", &Settings::option3D)) {
        if (Settings::option3D) Settings::option2D = false;
        if (!Settings::option3D) Settings::option2D = true;
        Settings::Settings[IS_OPTION_2D_ENABLED] = Settings::option2D;
        Settings::Settings[IS_OPTION_3D_ENABLED] = Settings::option3D;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Measure speed in 3D space. Best suitable for Griffon racing.");
        ImGui::EndTooltip();
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Set the unit that velocity gets measured in:");
    if (ImGui::Checkbox("u/s", &Settings::optionUnits)) {
        if (Settings::optionUnits) { Settings::optionFeetPercent = false; Settings::optionBeetlePercent = false; }
        if (!Settings::optionUnits) { Settings::optionUnits = true; }
        Settings::Settings[IS_OPTION_UNITS_ENABLED] = Settings::optionUnits;
        Settings::Settings[IS_OPTION_FEETPERCENT_ENABLED] = Settings::optionFeetPercent;
        Settings::Settings[IS_OPTION_BEETLEPERCENT_ENABLED] = Settings::optionBeetlePercent;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Measure speed in u/s. Units are what all distances are measured as ingame.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Feet%", &Settings::optionFeetPercent)) {
        if (Settings::optionFeetPercent) { Settings::optionUnits = false; Settings::optionBeetlePercent = false; }
        if (!Settings::optionFeetPercent) { Settings::optionUnits = true; }
        Settings::Settings[IS_OPTION_UNITS_ENABLED] = Settings::optionUnits;
        Settings::Settings[IS_OPTION_FEETPERCENT_ENABLED] = Settings::optionFeetPercent;
        Settings::Settings[IS_OPTION_BEETLEPERCENT_ENABLED] = Settings::optionBeetlePercent;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Measure speed in Feet%%. Assumes standard out of combat walk speed to be 100%%.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(400.0f, 0.0f);
    if (ImGui::Checkbox("Beetle%", &Settings::optionBeetlePercent)) {
        if (Settings::optionBeetlePercent) { Settings::optionUnits = false; Settings::optionFeetPercent = false; }
        if (!Settings::optionBeetlePercent) { Settings::optionUnits = true; }
        Settings::Settings[IS_OPTION_UNITS_ENABLED] = Settings::optionUnits;
        Settings::Settings[IS_OPTION_FEETPERCENT_ENABLED] = Settings::optionFeetPercent;
        Settings::Settings[IS_OPTION_BEETLEPERCENT_ENABLED] = Settings::optionBeetlePercent;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Measure speed in Beetle%%. Assumes standard max speed of the Beetle to be 99%%.");
        ImGui::EndTooltip();
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Click and hold, then slide to set the maximum needle amplitude per measuring unit:");
    if (ImGui::DragFloat("Set between 50 and 3000", &Settings::amplitudeUnits, 1.0f, 50.0f, 3000.0f, "%.1f u/s"))
    {
        Settings::Settings[NEEDLE_AMPLITUDE_UNITS] = Settings::amplitudeUnits;
        Settings::Save(SettingsPath);
    }
    if (ImGui::DragFloat("Set between 5 and 1000", &Settings::amplitudeFeetPercent, 1.0f, 5.0f, 1000.0f, "%.1f Feet%%"))
    {
        Settings::Settings[NEEDLE_AMPLITUDE_FEETPERCENT] = Settings::amplitudeFeetPercent;
        Settings::Save(SettingsPath);
    }
    if (ImGui::DragFloat("Set between 1 and 150", &Settings::amplitudeBeetlePercent, 1.0f, 1.0f, 200.0f, "%.1f Beetle%%"))
    {
        Settings::Settings[NEEDLE_AMPLITUDE_BEETLEPERCENT] = Settings::amplitudeBeetlePercent;
        Settings::Save(SettingsPath);
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Timer visibility and appearance settings:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Toggle visibility on or off");
    if (ImGui::Checkbox("Show Timer", &Settings::IsTimerEnabled))
    {
        Settings::Settings[IS_SPEEDOMETER_TIMER_VISIBLE] = Settings::IsTimerEnabled;
        Settings::Save(SettingsPath);
        if (!Settings::IsTimerEnabled)
        {
            timerRunning = false;
            timerPaused = false;
            basePosInitialized = false;
        }
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Use the sliders to adjust the position and scale of the Timer:");
    if (ImGui::SliderFloat("hPos Timer", &Settings::TimerPositionH, NexusLink->Width * 0.0f, NexusLink->Width * 1.0f, "%.0f X%"))
    {
        Settings::Settings[SPEEDOMETER_TIMER_POSITION_H] = Settings::TimerPositionH;
        Settings::Save(SettingsPath);
    }

    if (ImGui::SliderFloat("vPos Timer", &Settings::TimerPositionV, NexusLink->Height * 0.0f, NexusLink->Height * 1.0f, "%.0f Y%"))
    {
        Settings::Settings[SPEEDOMETER_TIMER_POSITION_V] = Settings::TimerPositionV;
        Settings::Save(SettingsPath);
    }

    if (ImGui::SliderFloat("Scale between 25% and 150%", &Settings::TimerScale, 25.0f, 150.0f, "%.0f %%"))
    {
        Settings::Settings[SPEEDOMETER_TIMER_SCALE] = Settings::TimerScale;
        Settings::Save(SettingsPath);
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Click and hold, then slide to set the start and checkpoint / finish circle fading distances:");
    if (ImGui::DragFloat("Start circle fading distance (300 - 20000 units)", &Settings::startFadingDistance, 5.0f, 300.0f, 20000.0f, "%.1f units"))
    {
        Settings::Settings[START_FADING_DISTANCE] = Settings::startFadingDistance;
        Settings::Save(SettingsPath);
    }
    if (ImGui::DragFloat("CHeckpoint / Finish circle fading distance (300 - 20000 units)", &Settings::finishFadingDistance, 5.0f, 300.0f, 20000.0f, "%.1f units"))
    {
        Settings::Settings[FINISH_FADING_DISTANCE] = Settings::finishFadingDistance;
        Settings::Save(SettingsPath);
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Choose the visual style of start, checkpoint and finish circles:");
    if (ImGui::Checkbox("Flat circle", &Settings::optionFlat))
    {
        if (Settings::optionFlat)
        {

        }
        if (!Settings::optionFlat && !Settings::optionArc && !Settings::optionRing)
        {
            Settings::optionFlat = true;
        }

        Settings::Settings[CIRCLE_STYLE_FLAT] = Settings::optionFlat;
        Settings::Settings[CIRCLE_STYLE_ARC] = Settings::optionArc;
        Settings::Settings[CIRCLE_STYLE_RING] = Settings::optionRing;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "The classic flat circular display of start, checkpoints and finish, aligned with the ground.");
        ImGui::EndTooltip();
    }

    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Dome Arc", &Settings::optionArc))
    {
        if (Settings::optionArc)
        {
            Settings::optionRing = false;
        }
        if (!Settings::optionFlat && !Settings::optionArc && !Settings::optionRing)
        {
            Settings::optionFlat = true;
        }
        Settings::Settings[CIRCLE_STYLE_FLAT] = Settings::optionFlat;
        Settings::Settings[CIRCLE_STYLE_ARC] = Settings::optionArc;
        Settings::Settings[CIRCLE_STYLE_RING] = Settings::optionRing;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "A vertically aligned arc that sits perpendicular to the ground of start, checkpoints and finish.");
        ImGui::EndTooltip();
    }

    ImGui::SameLine(400.0f, 0.0f);
    if (ImGui::Checkbox("Vertical Ring", &Settings::optionRing))
    {
        if (Settings::optionRing)
        {
            Settings::optionArc = false;
        }
        if (!Settings::optionFlat && !Settings::optionArc && !Settings::optionRing)
        {
            Settings::optionFlat = true;
        }
        Settings::Settings[CIRCLE_STYLE_FLAT] = Settings::optionFlat;
        Settings::Settings[CIRCLE_STYLE_ARC] = Settings::optionArc;
        Settings::Settings[CIRCLE_STYLE_RING] = Settings::optionRing;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "A vertically aligned ring that sits perpendicular to the ground of start, checkpoints and finish.");
        ImGui::EndTooltip();
    }
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Set the relative position of the Timer auxiliary window");
    if (ImGui::Checkbox("Left-aligned", &Settings::optionTimerLeft)) {
        if (Settings::optionTimerLeft) Settings::optionTimerRight = false;
        if (!Settings::optionTimerLeft) Settings::optionTimerRight = true;
        Settings::Settings[TIMER_AUX_RIGHT_ALIGNED] = Settings::optionTimerRight;
        Settings::Settings[TIMER_AUX_LEFT_ALIGNED] = Settings::optionTimerLeft;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Auxiliary window will align flush with the left side of the Timer. Best if you place your Timer at the left border of your screen.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Right-aligned", &Settings::optionTimerRight))
    {
        if (Settings::optionTimerRight) Settings::optionTimerLeft = false;
        if (!Settings::optionTimerRight) Settings::optionTimerLeft = true;
        Settings::Settings[TIMER_AUX_RIGHT_ALIGNED] = Settings::optionTimerRight;
        Settings::Settings[TIMER_AUX_LEFT_ALIGNED] = Settings::optionTimerLeft;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Auxiliary window will align flush with the right side of the Timer. Best if you place your Timer at the right border of your screen.");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("On top", &Settings::optionTimerTop))
    {
        if (Settings::optionTimerTop) Settings::optionTimerBottom = false;
        if (!Settings::optionTimerTop) Settings::optionTimerBottom = true;
        Settings::Settings[TIMER_AUX_TOP_ALIGNED] = Settings::optionTimerTop;
        Settings::Settings[TIMER_AUX_BOTTOM_ALIGNED] = Settings::optionTimerBottom;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Auxiliary window will appear on top of the Timer. Best if you place your Timer at the bottom border of your screen.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Below", &Settings::optionTimerBottom)) {
        if (Settings::optionTimerBottom) Settings::optionTimerTop = false;
        if (!Settings::optionTimerBottom) Settings::optionTimerTop = true;
        Settings::Settings[TIMER_AUX_TOP_ALIGNED] = Settings::optionTimerTop;
        Settings::Settings[TIMER_AUX_BOTTOM_ALIGNED] = Settings::optionTimerBottom;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Auxiliary window will appear below the Timer. Best if you place your Timer at the top border of your screen.");
        ImGui::EndTooltip();
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Timer mode settings:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Choose the mode of the Timer:");
    if (ImGui::Checkbox("Manual", &Settings::optionManual))
    {
        if (Settings::optionManual)
        {
            Settings::optionPredefined = false; Settings::optionCustom = false;

            timerRunning = false;
            timerPaused = false;
            basePosInitialized = false;
        }
        if (!Settings::optionManual)
        {
            Settings::optionCustom = true;

            basePosInitialized = false;

            timerFinishSet = false;
            timerStartSet = false;
            timerPaused = false;
            timerRunning = false;
            timerStartEntered = false;
            basePos = { -10000.0f, -10000.0f, -10000.0f };
            basePosInitialized = false;
            pausePos = { -10000.0f, -10000.0f, -10000.0f };
            timerHotkeyCount = 4;
        }
        Settings::Settings[IS_OPTION_MANUAL_ENABLED] = Settings::optionManual;
        Settings::Settings[IS_OPTION_PREDEFINED_ENABLED] = Settings::optionPredefined;
        Settings::Settings[IS_OPTION_CUSTOM_ENABLED] = Settings::optionCustom;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Starting location is set upon activation or reset, and the Timer can be paused and reset at will.");
        ImGui::EndTooltip();
    }

    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Custom", &Settings::optionCustom))
    {
        if (Settings::optionCustom)
        {
            Settings::optionManual = false; Settings::optionPredefined = false;

            timerFinishSet = false;
            timerStartSet = false;
            timerPaused = false;
            timerRunning = false;
            timerStartEntered = false;
            basePos = { -10000.0f, -10000.0f, -10000.0f };
            basePosInitialized = false;
            pausePos = { -10000.0f, -10000.0f, -10000.0f };
            timerHotkeyCount = 4;
        }
        if (!Settings::optionCustom)
        {
            Settings::optionManual = true;

            timerRunning = false;
            timerPaused = false;
            basePosInitialized = false;
        }
        Settings::Settings[IS_OPTION_MANUAL_ENABLED] = Settings::optionManual;
        Settings::Settings[IS_OPTION_PREDEFINED_ENABLED] = Settings::optionPredefined;
        Settings::Settings[IS_OPTION_CUSTOM_ENABLED] = Settings::optionCustom;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "You can set your own start and finish locations by using the Pause keybind or clicking the stopwatch.");
        ImGui::EndTooltip();
    }

    ImGui::SameLine(400.0f, 0.0f);
    if (ImGui::Checkbox("Predefined", &Settings::optionPredefined))
    {
        if (Settings::optionPredefined)
        {
            Settings::optionManual = false; Settings::optionCustom = false;

            timerPaused = false;
            timerRunning = false;
            timerStartEntered = false;
            timerCheckpointEntered = false;
            timerHotkeyCount = 4;
            checkpointlistfinished = false;
        }
        if (!Settings::optionPredefined)
        {
            Settings::optionManual = true;

            timerRunning = false;
            timerPaused = false;
            basePosInitialized = false;
        }
        Settings::Settings[IS_OPTION_MANUAL_ENABLED] = Settings::optionManual;
        Settings::Settings[IS_OPTION_PREDEFINED_ENABLED] = Settings::optionPredefined;
        Settings::Settings[IS_OPTION_CUSTOM_ENABLED] = Settings::optionCustom;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Start, checkpoint and finish location sets from coordinates.json may be selected. Timer only resets upon returning to the start.");
        ImGui::EndTooltip();
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Manual Timer settings:");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Choose if pausing the timer is temporary or final");
    if (ImGui::Checkbox("Temporary", &Settings::optionPause))
    {
        if (Settings::optionPause) Settings::optionStop = false;
        if (!Settings::optionPause) Settings::optionStop = true;
        Settings::Settings[IS_OPTION_PAUSE_ENABLED] = Settings::optionPause;
        Settings::Settings[IS_OPTION_STOP_ENABLED] = Settings::optionStop;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "If paused, the Timer will resume on movement. Pausing only works when standing still.");
        ImGui::EndTooltip();
    }
    ImGui::SameLine(200.0f, 0.0f);
    if (ImGui::Checkbox("Final", &Settings::optionStop))
    {
        if (Settings::optionStop) Settings::optionPause = false;
        if (!Settings::optionStop) Settings::optionPause = true;
        Settings::Settings[IS_OPTION_PAUSE_ENABLED] = Settings::optionPause;
        Settings::Settings[IS_OPTION_STOP_ENABLED] = Settings::optionStop;
        Settings::Save(SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "If paused, the Timer will be stopped and cannot resume. Reset required to start again.");
        ImGui::EndTooltip();
    }
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Click and hold, then slide to set the manual start location radius in units:");
    if (ImGui::DragFloat("Manual start and pause circle radii (18 - 900 units)", &Settings::manualstartDiameter, 1.0f, 18.0f, 900.0f, "%.1f units"))
    {
        Settings::Settings[MANUAL_START_DIAMETER_UNITS] = Settings::manualstartDiameter;
        Settings::Save(SettingsPath);
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Custom Timer settings");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Click and hold, then slide to set the start and finish location radii in units:");
    if (ImGui::DragFloat("Custom start circle radius (18 - 900 units)", &Settings::startDiameter, 1.0f, 18.0f, 900.0f, "%.1f units"))
    {
        Settings::Settings[START_DIAMETER_UNITS] = Settings::startDiameter;
        Settings::Save(SettingsPath);
    }
    if (ImGui::DragFloat("Custom finish circle radius (60 - 900 units)", &Settings::finishDiameter, 1.0f, 60.0f, 900.0f, "%.1f units%"))
    {
        Settings::Settings[FINISH_DIAMETER_UNITS] = Settings::finishDiameter;
        Settings::Save(SettingsPath);
    }

    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "Restore defaults!");
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 0.5f), "Need a do-over, then hit this button:");
    if (ImGui::Button("Reset Defaults"))
    {
        basePos = currentPos;

        Settings::IsReadMeEnabled = false;
        Settings::ReadMePositionH = 450.0f;
        Settings::ReadMePositionV = 250.0f;

        Settings::IsDialEnabled = true;
        Settings::IsTableEnabled = false;
        Settings::option2D = true;
        Settings::option3D = false;
        Settings::optionUnits = true;
        Settings::optionFeetPercent = false;
        Settings::optionBeetlePercent = false;

        Settings::amplitudeUnits = 391.0f;
        Settings::amplitudeFeetPercent = 100.0f;
        Settings::amplitudeBeetlePercent = 99.0f;

        Settings::DialPositionH = 320.0f;
        Settings::DialPositionV = 300.0f;
        Settings::DialScale = 60.0f;

        Settings::IsTimerEnabled = true;
        Settings::startFadingDistance = 2500.0f;
        Settings::finishFadingDistance = 2500.0f;
        Settings::optionFlat = true;
        Settings::optionArc = false;
        Settings::optionRing = false;
        Settings::optionPause = true;
        Settings::optionTimerRight = true;
        Settings::optionTimerLeft = false;
        Settings::optionTimerTop = false;
        Settings::optionTimerBottom = true;

        Settings::optionPause = true;
        Settings::optionStop = false;
        Settings::manualstartDiameter = 18.0f;

        Settings::optionManual = true;
        Settings::optionPredefined = false;
        Settings::optionCustom = false;

        Settings::startDiameter = 100.0f;
        Settings::finishDiameter = 200.0f;

        Settings::TimerPositionH = 260.0f;
        Settings::TimerPositionV = 560.0f;
        Settings::TimerScale = 60.0f;

        Settings::Settings[IS_READ_ME_VISIBLE] = Settings::IsReadMeEnabled;
        Settings::Settings[READ_ME_POSITION_H] = Settings::DialPositionH;
        Settings::Settings[READ_ME_POSITION_V] = Settings::DialPositionV;
        Settings::Settings[IS_SPEEDOMETER_DIAL_VISIBLE] = Settings::IsDialEnabled;
        Settings::Settings[IS_SPEEDOMETER_TABLE_VISIBLE] = Settings::IsTableEnabled;
        Settings::Settings[IS_OPTION_2D_ENABLED] = Settings::option2D;
        Settings::Settings[IS_OPTION_3D_ENABLED] = Settings::option3D;
        Settings::Settings[IS_OPTION_UNITS_ENABLED] = Settings::optionUnits;
        Settings::Settings[IS_OPTION_FEETPERCENT_ENABLED] = Settings::optionFeetPercent;
        Settings::Settings[IS_OPTION_BEETLEPERCENT_ENABLED] = Settings::optionBeetlePercent;
        Settings::Settings[NEEDLE_AMPLITUDE_UNITS] = Settings::amplitudeUnits;
        Settings::Settings[NEEDLE_AMPLITUDE_FEETPERCENT] = Settings::amplitudeFeetPercent;
        Settings::Settings[NEEDLE_AMPLITUDE_BEETLEPERCENT] = Settings::amplitudeBeetlePercent;
        Settings::Settings[SPEEDOMETER_DIAL_POSITION_H] = Settings::DialPositionH;
        Settings::Settings[SPEEDOMETER_DIAL_POSITION_V] = Settings::DialPositionV;
        Settings::Settings[SPEEDOMETER_DIAL_SCALE] = Settings::DialScale;
        Settings::Settings[IS_SPEEDOMETER_TIMER_VISIBLE] = Settings::IsTimerEnabled;
        Settings::Settings[START_FADING_DISTANCE] = Settings::startFadingDistance;
        Settings::Settings[FINISH_FADING_DISTANCE] = Settings::finishFadingDistance;
        Settings::Settings[CIRCLE_STYLE_FLAT] = Settings::optionFlat;
        Settings::Settings[CIRCLE_STYLE_ARC] = Settings::optionArc;
        Settings::Settings[CIRCLE_STYLE_RING] = Settings::optionRing;
        Settings::Settings[IS_OPTION_PAUSE_ENABLED] = Settings::optionPause;
        Settings::Settings[IS_OPTION_STOP_ENABLED] = Settings::optionStop;
        Settings::Settings[TIMER_AUX_RIGHT_ALIGNED] = Settings::optionTimerRight;
        Settings::Settings[TIMER_AUX_LEFT_ALIGNED] = Settings::optionTimerLeft;
        Settings::Settings[TIMER_AUX_TOP_ALIGNED] = Settings::optionTimerTop;
        Settings::Settings[TIMER_AUX_BOTTOM_ALIGNED] = Settings::optionTimerBottom;
        Settings::Settings[MANUAL_START_DIAMETER_UNITS] = Settings::manualstartDiameter;
        Settings::Settings[IS_OPTION_MANUAL_ENABLED] = Settings::optionManual;
        Settings::Settings[IS_OPTION_PREDEFINED_ENABLED] = Settings::optionPredefined;
        Settings::Settings[IS_OPTION_CUSTOM_ENABLED] = Settings::optionCustom;
        Settings::Settings[START_DIAMETER_UNITS] = Settings::startDiameter;
        Settings::Settings[FINISH_DIAMETER_UNITS] = Settings::finishDiameter;
        Settings::Settings[SPEEDOMETER_TIMER_POSITION_H] = Settings::TimerPositionH;
        Settings::Settings[SPEEDOMETER_TIMER_POSITION_V] = Settings::TimerPositionV;
        Settings::Settings[SPEEDOMETER_TIMER_SCALE] = Settings::TimerScale;

        Settings::Save(SettingsPath);

        /*
        Coordinates::TestedTheDingDong = true;

        Coordinates::Coordinates[TEST_TEST_DING_DONG] = Coordinates::TestedTheDingDong;

        Coordinates::Save(CoordinatesPath);
        */
    }
    ImGui::Separator();
    ImGui::Separator();
}