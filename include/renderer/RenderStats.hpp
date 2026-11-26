#pragma once

#include <glad/gl.h>

#include <chrono>
#include <cstddef>
#include <string>

class RenderStats
{
private:

    using Clock = std::chrono::steady_clock;

    Clock::time_point cpuStart;

    GLuint gpuQuery = 0;

    bool gpuQueryPending = false;
    bool gpuQueryActive = false;
    bool gpuTimeAvailable = false;

    double cpuRenderTime = 0.0;
    double gpuRenderTime = 0.0;

    std::size_t glyphCount = 0;
    std::size_t triangleCount = 0;
    std::size_t drawCalls = 0;
    std::size_t visibleTiles = 0;
    std::size_t uploadedBytes = 0;

public:

    RenderStats();
    ~RenderStats();

    RenderStats(const RenderStats&) = delete;

    RenderStats& operator=(
        const RenderStats&
    ) = delete;

    void beginFrame();
    void endFrame();

    void recordDrawCall(
        std::size_t triangles
    );

    void recordGlyphs(
        std::size_t count,
        std::size_t bytesPerInstance
    );

    void setVisibleTiles(
        std::size_t count
    );

    std::string getTitleSuffix() const;

    void print(
        int fps,
        double frameTime
    ) const;
};