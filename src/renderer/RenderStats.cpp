#include "renderer/RenderStats.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

RenderStats::RenderStats()
{
    glGenQueries(1, &gpuQuery);
}

RenderStats::~RenderStats()
{
    glDeleteQueries(1, &gpuQuery);
}

void RenderStats::beginFrame()
{
    glyphCount = 0;
    triangleCount = 0;
    drawCalls = 0;
    visibleTiles = 0;
    uploadedBytes = 0;

    cpuStart = Clock::now();

    // Read the previous GPU measurement
    // without blocking the CPU.

    if (gpuQueryPending)
    {
        GLint available = GL_FALSE;

        glGetQueryObjectiv(
            gpuQuery,
            GL_QUERY_RESULT_AVAILABLE,
            &available
        );

        if (available == GL_TRUE)
        {
            GLuint64 nanoseconds = 0;

            glGetQueryObjectui64v(
                gpuQuery,
                GL_QUERY_RESULT,
                &nanoseconds
            );

            gpuRenderTime =
                static_cast<double>(nanoseconds)
                / 1000000.0;

            gpuQueryPending = false;
            gpuTimeAvailable = true;
        }
    }

    // Start a new GPU measurement only
    // if the previous one has completed.

    if (!gpuQueryPending)
    {
        glBeginQuery(
            GL_TIME_ELAPSED,
            gpuQuery
        );

        gpuQueryActive = true;
    }
}

void RenderStats::endFrame()
{
    if (gpuQueryActive)
    {
        glEndQuery(GL_TIME_ELAPSED);

        gpuQueryActive = false;
        gpuQueryPending = true;
    }

    const auto cpuEnd = Clock::now();

    cpuRenderTime =
        std::chrono::duration<double, std::milli>(
            cpuEnd - cpuStart
        ).count();
}

void RenderStats::recordDrawCall(
    std::size_t triangles
)
{
    ++drawCalls;

    triangleCount += triangles;
}

void RenderStats::recordGlyphs(
    std::size_t count,
    std::size_t bytesPerInstance
)
{
    glyphCount += count;

    uploadedBytes +=
        count * bytesPerInstance;

    if (count > 0)
    {
        recordDrawCall(count * 2);
    }
}

void RenderStats::setVisibleTiles(
    std::size_t count
)
{
    visibleTiles = count;
}

std::string RenderStats::getTitleSuffix() const
{
    std::ostringstream stream;

    stream << std::fixed
           << std::setprecision(2);

    stream
        << " | CPU: "
        << cpuRenderTime
        << " ms"
        << " | GPU: ";

    if (gpuTimeAvailable)
    {
        stream << gpuRenderTime << " ms";
    }
    else
    {
        stream << "N/A";
    }

    stream
        << " | Glyphs: "
        << glyphCount
        << " | Draws: "
        << drawCalls;

    return stream.str();
}

void RenderStats::print(
    int fps,
    double frameTime
) const
{
    const double uploadedMiB =
        static_cast<double>(uploadedBytes)
        / (1024.0 * 1024.0);

    std::ostringstream stream;

    stream << std::fixed
           << std::setprecision(2);

    stream
        << "\n"
        << "========== Render Stats ==========\n"
        << "FPS:               " << fps << "\n"
        << "Frame Time:        " << frameTime
        << " ms\n"
        << "CPU Render Time:   " << cpuRenderTime
        << " ms\n"
        << "GPU Render Time:   ";

    if (gpuTimeAvailable)
    {
        stream << gpuRenderTime << " ms\n";
    }
    else
    {
        stream << "N/A\n";
    }

    stream
        << "Glyph Instances:   " << glyphCount << "\n"
        << "Triangles:         " << triangleCount << "\n"
        << "Draw Calls:        " << drawCalls << "\n"
        << "Visible Tiles:     " << visibleTiles << "\n"
        << "Instance Upload:   " << uploadedMiB
        << " MiB/frame\n"
        << "==================================\n";

    std::cout << stream.str();
}