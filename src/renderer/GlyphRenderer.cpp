#include "renderer/CpuProfiler.hpp"
#include "renderer/GlyphRenderer.hpp"

#include <cstddef>
#include <vector>
#include <algorithm>
#include <array>
#include <string>
#include <cmath>
#include <stdexcept>

namespace
{
    const std::vector<Vertex> vertices = {
        {-0.5f,  0.5f, 0.0f, 0.0f, 1.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f}
    };

    const std::vector<GLuint> indices = {
        0, 3, 2,
        0, 2, 1
    };

    std::uint32_t quantize(float value, float maximum)
    {
        return static_cast<std::uint32_t>(std::lround(std::clamp(value,0.0f,1.0f)*maximum));
    }
}

GlyphRenderer::GlyphRenderer(const FontAtlas& atlas)
    : atlas(atlas),
      shader(
          "shaders/glyph.vert",
          "shaders/glyph.frag"
      ),
      quad(vertices, indices)
{
    instances.reserve(10000);

    shader.setInt("glyphTexture", 0);
    shader.setInt("uLightmap", 1);

    initializeInstanceBuffer();
}

GlyphRenderer::~GlyphRenderer()
{
    clearGroundCache();

    glDeleteBuffers(
        1,
        &instanceVBO
    );
}

void GlyphRenderer::initializeInstanceBuffer()
{
    GLint maxAttributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttributes);
    if (maxAttributes <= 10)
    {
        throw std::runtime_error("Surface lighting requires 11 vertex attributes");
    }
    quad.bind();

    glGenBuffers(1, &instanceVBO);

    constexpr GLuint bindingIndex = 2;

    glBindVertexBuffer(
        bindingIndex,
        instanceVBO,
        0,
        sizeof(GlyphInstance)
    );

    const auto attribute =
        [](GLuint index, GLint size, GLuint offset)
        {
            glEnableVertexAttribArray(index);

            glVertexAttribFormat(
                index,
                size,
                GL_FLOAT,
                GL_FALSE,
                offset
            );

            glVertexAttribBinding(
                index,
                2
            );
        };

    // Position

    attribute(
        2,
        2,
        offsetof(GlyphInstance, x)
    );

    // Size

    attribute(
        3,
        2,
        offsetof(GlyphInstance, width)
    );

    // UV coordinates

    attribute(
        4,
        4,
        offsetof(GlyphInstance, u0)
    );

    // Color

    attribute(
        5,
        3,
        offsetof(GlyphInstance, r)
    );

    // Animation

    attribute(
        6,
        3,
        offsetof(GlyphInstance, animationType)
    );

    // Transform

    attribute(
        7,
        4,
        offsetof(GlyphInstance, rotation)
    );

    // Lighting and depth

    attribute(
        8,
        3,
        offsetof(GlyphInstance, lightOffsetY)
    );

    // World-space anchor

    attribute(
        9,
        4,
        offsetof(GlyphInstance, anchorOffsetX)
    );
    glEnableVertexAttribArray(10);
    glVertexAttribIFormat(10, 2, GL_UNSIGNED_INT, offsetof(GlyphInstance, normalAndMode));
    glVertexAttribBinding(10, bindingIndex);

    glVertexBindingDivisor(
        bindingIndex,
        1
    );

    quad.unbind();
}

void GlyphRenderer::setScreenSize(
    int width,
    int height
)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    shader.setVec2(
        "uScreenSize",
        static_cast<float>(width),
        static_cast<float>(height)
    );
}

void GlyphRenderer::begin()
{
    instances.clear();

    orderedInstances.clear();

    depths.clear();

    activeGroundChunks.clear();

    capturedInstances.clear();

    capturingGround = false;

    cachedGroundCount = 0;

    cachedUploadBytes = 0;
    cacheHits = cacheMisses = 0;
    treeLevels.fill(0);

    ++cacheFrame;
}

void GlyphRenderer::drawWorld(
    const Camera& camera,
    char32_t character,
    float x,
    float y,
    float z,
    float scale,
    Color color,
    GlyphAnimation animation,
    GlyphTransform transform,
    GlyphAnchor anchor,
    bool groundLayer,
    bool solid,
    GlyphMaterial material
)
{
    // ========================================
    // STATIC GEOMETRY CAPTURE
    // ========================================

    if (capturingGround)
    {
        const bool vegetation =
            std::get<2>(captureKey) < 0;

        draw(
            character,

            x,
            y,

            scale,

            color,

            animation,
            transform,
            anchor,

            0.0f,

            std::nullopt,

            solid || vegetation,

            z,
            material
        );

        return;
    }

    // ========================================
    // DYNAMIC GEOMETRY
    // ========================================

    const auto screen =
        camera.project(x, y, z);

    const float screenScale =
        scale * camera.getZoom();

    if (!camera.isVisible(
            screen,
            128.0f * screenScale
        ))
    {
        return;
    }

    const auto depth =
        groundLayer
            ? std::nullopt
            : std::optional{
        camera.depth(x, y, z)
    };

    draw(
        character,

        x,
        y,

        scale,

        color,

        animation,
        transform,
        anchor,

        0.0f,

        depth,

        solid,

        z,
        material
    );
}

void GlyphRenderer::draw(
    char32_t character,
    float x,
    float y,
    float scale,
    Color color,
    GlyphAnimation animation,
    GlyphTransform transform,
    GlyphAnchor anchor,
    float lightOffsetY,
    std::optional<float> depth,
    bool solid,
    std::optional<float> worldZ,
    GlyphMaterial material
)
{
    const AtlasGlyph& glyph =
        atlas.getGlyph(character);

    if (glyph.width == 0 ||
        glyph.height == 0)
    {
        return;
    }

    if (transform.opacity <= 0.0f)
    {
        return;
    }

    // Grass scale encodes desired world height / 36, independent of font metrics.
    if (animation.type == AnimationType::Grass)
        scale *= 36.0f / static_cast<float>(glyph.height);

    // Glyph anchor offset.

    const float offsetX =
        (
            anchor == GlyphAnchor::Center
                ? -0.5f * glyph.width
                : glyph.bearingX
        ) * scale;

    const float offsetY =
        -(
            anchor == GlyphAnchor::Center
                ? 0.5f * glyph.height
                : glyph.bearingY
        ) * scale;

    const bool worldSpace =
        worldZ.has_value();

    // Create GPU instance.

    GlyphInstance instance
    {
        // Position

        worldSpace ? x : x + offsetX,
        worldSpace ? y : y + offsetY,

        // Size

        glyph.width * scale,
        glyph.height * scale,

        // UV coordinates

        glyph.u0,
        glyph.v0,
        glyph.u1,
        glyph.v1,

        // Color

        color.r,
        color.g,
        color.b,

        // Animation

        static_cast<float>(animation.type),

        animation.phase,
        animation.amplitude,

        // Transform

        transform.rotation,

        std::clamp(
            transform.opacity,
            0.0f,
            1.0f
        ),

        transform.pivotX,
        transform.pivotY,

        // Lighting and depth

        lightOffsetY,
        0.0f,
        solid ? 1.0f : 0.0f,

        // World-space projection

        worldSpace ? offsetX : 0.0f,
        worldSpace ? offsetY : 0.0f,

        worldZ.value_or(0.0f),

        worldSpace ? 1.0f : 0.0f,
        quantize(material.nx*0.5f+0.5f,1023)
            | (quantize(material.ny*0.5f+0.5f,1023) << 10)
            | (quantize(material.nz*0.5f+0.5f,1023) << 20)
            | (static_cast<std::uint32_t>(worldSpace ? material.mode : LightingMode::Legacy) << 30),
        quantize(material.roughness,255)
            | (quantize(material.reflectivity,255) << 8)
            | (quantize(material.ambientOcclusion,255) << 16)
    };

    // Preserve the existing depth sorting.

    if (capturingGround)
    {
        capturedInstances.push_back(
            instance
        );

        return;
    }

    if (depth)
    {
        depths.push_back(*depth);

        orderedInstances.push_back(
            instance
        );
    }
    else
    {
        instances.push_back(
            instance
        );
    }
}

void GlyphRenderer::appendDepthSortedInstances()
{
    if (depths.empty())
    {
        return;
    }
    // Stable back-to-front buckets retain submission order within a small depth interval.
    constexpr std::size_t bucketCount = 4096;
    std::array<std::size_t, bucketCount> offsets{};
    const auto [minimum, maximum] = std::minmax_element(depths.begin(), depths.end());
    const float range = *maximum - *minimum;
    const float scale = range > 0.0f ? static_cast<float>(bucketCount - 1) / range : 0.0f;
    const auto bucket = [near = *minimum, scale](float depth)
    {
        return std::min(bucketCount - 1, static_cast<std::size_t>((depth - near) * scale));
    };
    for (float depth : depths)
    {
        ++offsets[bucket(depth)];
    }
    std::size_t offset = instances.size();
    for (auto& count : offsets)
    {
        const auto next = offset + count;
        count = offset;
        offset = next;
    }
    instances.resize(offset);
    for (std::size_t i = 0; i < depths.size(); ++i)
    {
        auto& instance = orderedInstances[i];
        instance.clipDepth = range > 0.0f ? 0.98f - 1.96f * (depths[i] - *minimum) / range : 0.0f;
        instances[offsets[bucket(depths[i])]++] = instance;
    }
    orderedInstances.clear();
    depths.clear();
}

void GlyphRenderer::flush()
{
    const auto groundCount =
        static_cast<GLsizei>(
            instances.size()
        );

    // ========================================
    // CPU SORTING OF DYNAMIC OBJECTS
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::DepthSort
        );

        appendDepthSortedInstances();
    }

    if (
        instances.empty()
        && activeGroundChunks.empty()
    )
    {
        removeOldGroundChunks();

        return;
    }

    shader.use();

    glEnable(GL_BLEND);

    glBlendEquation(GL_FUNC_ADD);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    atlas.getTexture().bind(0);

    glBindTextureUnit(
        1,
        lightmapTexture
    );

    quad.bind();

    glDisable(GL_DEPTH_TEST);

    glDepthMask(GL_TRUE);

    // ========================================
    // UPLOAD DYNAMIC GEOMETRY
    // ========================================

    if (!instances.empty())
    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::BufferUpload
        );

        glBindBuffer(
            GL_ARRAY_BUFFER,
            instanceVBO
        );

        if (
            bufferCapacity
            < instances.size()
        )
        {
            bufferCapacity =
                std::max(
                    instances.size(),
                    bufferCapacity * 2
                );
        }

        glBufferData(
            GL_ARRAY_BUFFER,

            bufferCapacity
                * sizeof(GlyphInstance),

            nullptr,

            GL_STREAM_DRAW
        );

        glBufferSubData(
            GL_ARRAY_BUFFER,

            0,

            instances.size()
                * sizeof(GlyphInstance),

            instances.data()
        );
    }

    // ========================================
    // RENDERING
    // ========================================

    {
        CpuProfiler::Scope timer(
            CpuProfiler::Stage::DrawSubmission
        );

        const auto drawCached =
            [this](
                bool vegetation,
                int coveragePass
            )
        {
            shader.setInt(
                "uCoveragePass",
                coveragePass
            );

            for (
                const auto* chunk
                : activeGroundChunks
            )
            {
                if (
                    chunk->vegetation
                    != vegetation
                )
                {
                    continue;
                }

                glBindVertexBuffer(
                    2,

                    chunk->buffer,

                    0,

                    sizeof(GlyphInstance)
                );

                glDrawElementsInstanced(
                    GL_TRIANGLES,

                    quad.getIndexCount(),

                    GL_UNSIGNED_INT,

                    nullptr,

                    chunk->count
                );
            }
        };

        const auto drawRange =
            [this](
                int pass,
                GLsizei count,
                GLuint first
            )
        {
            if (count <= 0)
            {
                return;
            }

            shader.setInt(
                "uCoveragePass",
                pass
            );

            glDrawElementsInstancedBaseInstance(
                GL_TRIANGLES,

                quad.getIndexCount(),

                GL_UNSIGNED_INT,

                nullptr,

                count,

                first
            );
        };

        const auto surfaceCount =
            static_cast<GLsizei>(
                instances.size()
            ) - groundCount;

        // ====================================
        // PASS 1: STATIC GROUND
        // ====================================

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);

        drawCached(
            false,
            0
        );

        // ====================================
        // PASS 2: DYNAMIC GROUND AND SHADOWS
        // ====================================

        glBindVertexBuffer(
            2,

            instanceVBO,

            0,

            sizeof(GlyphInstance)
        );

        if (groundCount > 0)
        {
            drawRange(
                0,

                groundCount,

                0
            );
        }

        // ====================================
        // ENABLE SHARED DEPTH BUFFER
        // ====================================

        glEnable(GL_DEPTH_TEST);

        glDepthFunc(GL_LEQUAL);

        glDepthMask(GL_TRUE);

        glDisable(GL_BLEND);

        // ====================================
        // PASS 3: STATIC VEGETATION
        //
        // Alpha cutout.
        // GPU world-space depth.
        // No CPU sorting.
        // ====================================

        drawCached(
            true,
            3
        );

        // Restore dynamic instance buffer.

        glBindVertexBuffer(
            2,

            instanceVBO,

            0,

            sizeof(GlyphInstance)
        );

        // ====================================
        // PASS 4: DYNAMIC OPAQUE SURFACES
        // ====================================

        if (surfaceCount > 0)
        {
            drawRange(
                1,

                surfaceCount,

                groundCount
            );
        }

        // ====================================
        // PASS 5: TRANSPARENT SURFACES
        // ====================================

        glEnable(GL_BLEND);

        glDepthMask(GL_FALSE);

        if (surfaceCount > 0)
        {
            drawRange(
                2,

                surfaceCount,

                groundCount
            );
        }

        // ====================================
        // RESTORE OPENGL STATE
        // ====================================

        glDepthMask(GL_TRUE);

        glDisable(GL_DEPTH_TEST);
    }

    quad.unbind();

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glDisable(GL_BLEND);

    instances.clear();

    activeGroundChunks.clear();

    removeOldGroundChunks();
}

void GlyphRenderer::setTime(float time)
{
    shader.setFloat("uTime", time);
}
void GlyphRenderer::setGrassEnvironment(float x, float y, float height, float time,
                                         float windX, float windY, float windStrength)
{
    const float radius = height * 0.19f;
    const float directionLength = std::hypot(windX, windY);
    if (directionLength > 0.0001f) { windX /= directionLength; windY /= directionLength; }
    else { windX = 0.0f; windY = 0.0f; }
    shader.setVec3("uGrassWind", windX, windY, std::clamp(windStrength, 0.0f, 1.0f));
    shader.setVec3("uGrassPlayer", x, y, radius);
    const auto& last = grassFootprints[(nextGrassFootprint + grassFootprints.size() - 1) % grassFootprints.size()];
    if (last.time < 0.0f || std::hypot(x-last.x,y-last.y) > height*0.065f || time < last.time)
    {
        grassFootprints[nextGrassFootprint] = {x,y,time};
        nextGrassFootprint = (nextGrassFootprint+1)%grassFootprints.size();
    }
    for (std::size_t i = 0; i < grassFootprints.size(); ++i)
    {
        const auto& p = grassFootprints[i];
        const float age = time-p.time;
        const float weight = age >= 0.0f ? std::clamp(1.0f-age/1.1f,0.0f,1.0f) : 0.0f;
        shader.setVec4("uGrassTrail["+std::to_string(i)+"]",p.x,p.y,radius,weight*weight);
    }
}

void GlyphRenderer::setWaterEnvironment(
    float x,
    float y,
    float time,
    bool inWater
)
{
    constexpr float RIPPLE_DURATION = 1.35f;

    constexpr float STEP_DISTANCE = 7.0f;

    constexpr float STEP_INTERVAL = 0.12f;

    const auto& last =
        waterFootprints[
            (
                nextWaterFootprint
                + waterFootprints.size()
                - 1
            )
            % waterFootprints.size()
        ];

    const float distance =
        std::hypot(
            x - last.x,
            y - last.y
        );

    const float elapsed =
        time - last.time;

    const bool enteredWater =
        inWater && !playerWasInWater;

    const bool madeStep =
        distance >= STEP_DISTANCE
        && elapsed >= STEP_INTERVAL;

    if (
        inWater
        && (enteredWater || madeStep)
    )
    {
        waterFootprints[nextWaterFootprint] = {
            x,
            y,
            time
        };

        nextWaterFootprint =
            (nextWaterFootprint + 1)
            % waterFootprints.size();
    }

    playerWasInWater = inWater;

    for (
        std::size_t i = 0;
        i < waterFootprints.size();
        ++i
    )
    {
        const auto& point =
            waterFootprints[i];

        const float age =
            time - point.time;

        const float strength =
            age >= 0.0f
            && age < RIPPLE_DURATION
                ? 1.0f - age / RIPPLE_DURATION
                : 0.0f;

        shader.setVec4(
            "uWaterTrail["
                + std::to_string(i)
                + "]",

            point.x,
            point.y,

            std::max(0.0f, age),

            strength
        );
    }
}

void GlyphRenderer::setLightmap(GLuint texture)
{
    lightmapTexture = texture;
}

void GlyphRenderer::setSurfaceLighting(const SurfaceLighting& lighting,
                                        std::span<const PointLight> lights, int debugMode)
{
    const auto setColor = [this](const char* name, Color color)
    {
        shader.setVec3(name,color.r,color.g,color.b);
    };
    setColor("uSurfaceSky",lighting.sky);
    setColor("uSurfaceGround",lighting.ground);
    shader.setVec3("uSunDirection",lighting.sun.x,lighting.sun.y,lighting.sun.z);
    shader.setVec3("uMoonDirection",lighting.moon.x,lighting.moon.y,lighting.moon.z);
    setColor("uSunRadiance",lighting.sun.radiance);
    setColor("uMoonRadiance",lighting.moon.radiance);
    const auto count = std::min<std::size_t>(4,lights.size());
    shader.setInt("uSurfacePointCount",static_cast<int>(count));
    shader.setInt("uLightingDebug",debugMode);
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto& light = lights[i];
        const auto suffix = "["+std::to_string(i)+"]";
        shader.setVec4("uSurfacePoint"+suffix,light.x,light.y,light.z,light.radius);
        shader.setVec3("uSurfaceRadiance"+suffix,light.color.r*light.intensity,
                       light.color.g*light.intensity,light.color.b*light.intensity);
    }
}
std::size_t GlyphRenderer::getInstanceCount() const
{
    return instances.size() + orderedInstances.size();
}

void GlyphRenderer::setCamera(
    const Camera& camera
)
{
    shader.setVec2(
        "uCameraPosition",
        camera.getX(),
        camera.getY()
    );

    shader.setVec4(
        "uCameraBasis",
        camera.getCosAzimuth(),
        camera.getSinAzimuth(),
        camera.getRise(),
        camera.getLift()
    );

    shader.setFloat(
        "uCameraZoom",
        camera.getZoom()
    );
}

bool GlyphRenderer::useGroundChunk(
    int chunkX,
    int chunkY,
    int detailLevel
)
{
    const GroundChunkKey key
    {
        chunkX,
        chunkY,
        detailLevel
    };

    auto iterator = groundChunks.find(key);

    if (iterator != groundChunks.end())
    {
        ++cacheHits;
        StaticGroundChunk& chunk =
            iterator->second;

        chunk.lastUsed = cacheFrame;

        if (chunk.count > 0)
        {
            activeGroundChunks.push_back(
                &chunk
            );

            cachedGroundCount +=
                chunk.count;
        }

        return true;
    }

    // Cache miss.
    ++cacheMisses;
    // Start capturing newly generated geometry.

    captureKey = key;

    capturedInstances.clear();

    capturingGround = true;

    return false;
}

void GlyphRenderer::finishGroundChunk()
{
    if (!capturingGround)
    {
        return;
    }

    capturingGround = false;

    auto [iterator, inserted] =
        groundChunks.try_emplace(
            captureKey
        );

    StaticGroundChunk& chunk =
        iterator->second;

    chunk.lastUsed = cacheFrame;

    chunk.vegetation =
        std::get<2>(captureKey) < 0;

    chunk.count =
        static_cast<GLsizei>(
            capturedInstances.size()
        );

    if (chunk.count == 0)
    {
        capturedInstances.clear();

        return;
    }

    const std::size_t bytes =
        capturedInstances.size()
        * sizeof(GlyphInstance);

    glCreateBuffers(
        1,
        &chunk.buffer
    );

    glNamedBufferStorage(
        chunk.buffer,

        static_cast<GLsizeiptr>(
            bytes
        ),

        capturedInstances.data(),

        0
    );

    cachedUploadBytes += bytes;
    cachedMemoryBytes += bytes;

    cachedGroundCount += chunk.count;

    activeGroundChunks.push_back(
        &chunk
    );

    capturedInstances.clear();
}

void GlyphRenderer::removeOldGroundChunks()
{
    while (
        groundChunks.size()
        > MAX_GPU_CHUNKS || cachedMemoryBytes > MAX_GPU_CACHE_BYTES
    )
    {
        auto oldest =
            groundChunks.end();

        for (
            auto iterator = groundChunks.begin();
            iterator != groundChunks.end();
            ++iterator
        )
        {
            const auto& chunk =
                iterator->second;

            // Never remove a chunk used
            // in the current frame.

            if (chunk.lastUsed == cacheFrame)
            {
                continue;
            }

            if (
                oldest == groundChunks.end()
                ||
                chunk.lastUsed
                    < oldest->second.lastUsed
            )
            {
                oldest = iterator;
            }
        }

        if (oldest == groundChunks.end())
        {
            break;
        }

        if (oldest->second.buffer != 0)
        {
            glDeleteBuffers(
                1,
                &oldest->second.buffer
            );
        }

        cachedMemoryBytes -= static_cast<std::size_t>(oldest->second.count) * sizeof(GlyphInstance);
        groundChunks.erase(oldest);
    }
}

void GlyphRenderer::clearGroundCache()
{
    activeGroundChunks.clear();

    for (auto& [key, chunk] : groundChunks)
    {
        if (chunk.buffer != 0)
        {
            glDeleteBuffers(
                1,
                &chunk.buffer
            );
        }
    }

    groundChunks.clear();
    cachedMemoryBytes = 0;

    capturedInstances.clear();

    capturingGround = false;

    cachedGroundCount = 0;

    cachedUploadBytes = 0;
}

std::size_t GlyphRenderer::getCachedMemoryBytes() const
{
    return cachedMemoryBytes;
}

void GlyphRenderer::printCacheStats(std::ostream& out) const
{
    out << "GPU cache entries: " << groundChunks.size()
        << ", memory: " << cachedMemoryBytes / (1024.0 * 1024.0) << " MiB"
        << ", hits/misses: " << cacheHits << '/' << cacheMisses
        << ", tree LOD H/M/L: " << treeLevels[0] << '/' << treeLevels[1] << '/' << treeLevels[2] << '\n';
}

bool GlyphRenderer::useVegetationChunk(
    int chunkX,
    int chunkY,
    int detailStep
)
{
    // Ground uses non-negative LOD keys.
    // Vegetation uses negative LOD keys.

    return useGroundChunk(
        chunkX,
        chunkY,
        -std::max(1, detailStep)
    );
}

bool GlyphRenderer::useRockCache(
    std::size_t rockId,
    int detailLevel
)
{
    return useGroundChunk(
        static_cast<int>(rockId),
        0,
        -100 - detailLevel
    );
}

bool GlyphRenderer::useTreeCache(
    std::size_t treeId,
    int detailLevel
)
{
    return useGroundChunk(
        static_cast<int>(treeId),
        0,
        -200 - detailLevel
    );
}
