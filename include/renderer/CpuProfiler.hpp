#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <ostream>
#include <iomanip>

namespace CpuProfiler
{
    enum class Stage : std::size_t
    {
        LightMap, Ground, Vegetation, Rocks, Trees, Knight,
        DepthSort, BufferUpload, DrawSubmission,
        Events, Input, WorldUpdate, PlayerUpdate, CameraUpdate, Render, Swap, Count
    };
    inline constexpr auto count = static_cast<std::size_t>(Stage::Count);
    inline constexpr std::array<const char*, count> names = {
        "Light map + background", "Ground + terrain", "Vegetation",
        "Rocks + shadows", "Trees + shadows", "Knight", "Depth sorting",
        "Buffer upload", "Glyph draw submission",
        "Event polling", "Input", "World update", "Player update",
        "Camera update", "Render (inclusive)", "SwapBuffers"
    };
    inline thread_local std::array<double, count> frame{};
    inline thread_local std::array<double, count> sums{};
    inline thread_local std::size_t frames = 0;
    inline thread_local double total = 0.0;
    inline void beginFrame() { frame.fill(0.0); }
    class Scope
    {
        using Clock = std::chrono::steady_clock;
        Stage stage;
        Clock::time_point start = Clock::now();
    public:
        explicit Scope(Stage value) : stage(value) {}
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        ~Scope() {
            frame[static_cast<std::size_t>(stage)] +=
                std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        }
    };
    inline void endFrame(double cpuMs) {
        for (std::size_t i = 0; i < count; ++i) sums[i] += frame[i];
        total += cpuMs;
        ++frames;
    }
    inline void printAndReset(std::ostream& out) {
        if (!frames) return;
        out << "CPU profiler (average of " << frames << " frames; render includes its substages):\n";
        for (std::size_t i = 0; i < count; ++i) {
            out << "  " << std::left << std::setw(25) << names[i]
                << std::right << sums[i] / frames << " ms\n";
        }
        out << "  Total frame average:     " << total / frames << " ms\n";
        sums.fill(0.0);
        total = 0.0;
        frames = 0;
    }
}
