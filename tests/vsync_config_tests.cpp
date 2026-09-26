#include "app/config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    const auto path = std::filesystem::temp_directory_path() /
        "NitLinkVSyncConfigTests.ini";
    std::error_code ignored;

    try {
        {
            std::ofstream file(path, std::ios::trunc);
            Require(file.is_open(), "legacy config opens");
            file << "low_latency = false\npresent_pacing = unique\n";
        }
        NitLink::Config legacy;
        Require(legacy.Load(path.string()), "legacy config loads");
        Require(!legacy.vsync, "missing VSync preference stays off");
        Require(!legacy.lowLatency && legacy.presentPacing == NitLink::kPacingUnique,
                "legacy low-latency and pacing preferences remain independent");

        NitLink::Config defaults;
        Require(defaults.Save(path.string()), "default config saves");
        NitLink::Config defaultLoaded;
        Require(defaultLoaded.Load(path.string()), "default config reloads");
        Require(!defaultLoaded.vsync && defaultLoaded.lowLatency,
                "VSync defaults off while Low Latency defaults on");

        for (bool lowLatency : {false, true}) {
            for (int pacing : {NitLink::kPacingRefresh, NitLink::kPacingCaptured,
                               NitLink::kPacingUnique}) {
                for (int cap : {-1, 0, 117}) {
                    NitLink::Config saved;
                    saved.lowLatency = lowLatency;
                    saved.presentPacing = pacing;
                    saved.presentCapHz = cap;
                    saved.hdrEnabled = true;
                    for (bool vsync : {true, false}) {
                        saved.vsync = vsync;
                        Require(saved.Save(path.string()), "VSync config saves");
                        NitLink::Config loaded;
                        Require(loaded.Load(path.string()), "VSync config reloads");
                        Require(loaded.vsync == vsync, "VSync on and off both persist");
                        Require(loaded.lowLatency == lowLatency &&
                                loaded.presentPacing == pacing &&
                                loaded.presentCapHz == cap && loaded.hdrEnabled,
                                "VSync preserves latency, pacing, cap and HDR preferences");

                        loaded.audioMuted = true;
                        loaded.windowWidth = 1280;
                        Require(loaded.Save(path.string()), "unrelated settings save");
                        NitLink::Config reloaded;
                        Require(reloaded.Load(path.string()), "unrelated save reloads");
                        Require(reloaded.vsync == vsync &&
                                reloaded.lowLatency == lowLatency &&
                                reloaded.presentPacing == pacing &&
                                reloaded.presentCapHz == cap && reloaded.hdrEnabled,
                                "unrelated save preserves presentation preferences");
                        Require(reloaded.audioMuted && reloaded.windowWidth == 1280,
                                "unrelated settings also persist");
                    }
                }
            }
        }
        std::cout << "VSync config migration and persistence tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        std::filesystem::remove(path, ignored);
        return 1;
    }
    std::filesystem::remove(path, ignored);
    return 0;
}
