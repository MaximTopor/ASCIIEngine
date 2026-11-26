#pragma once

#include "renderer/FontAtlas.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Lighting.hpp"
#include "renderer/Camera.hpp"
#include <cstddef>
#include <optional>
#include <vector>
#include <map>
#include <tuple>
#include <cstdint>
#include <array>
#include <iosfwd>
#include <type_traits>

enum class AnimationType
{
    None = 0,
    Wind = 1,
    Water = 2,
    Grass = 3
};

struct GlyphAnimation
{
    AnimationType type = AnimationType::None;

    float phase = 0.0f;
    float amplitude = 0.0f;
};

struct GlyphTransform
{
    float rotation = 0.0f;

    float opacity = 1.0f;

    float pivotX = 0.5f;
    float pivotY = 1.0f;
};

struct GlyphInstance
{
    // Position

    float x;
    float y;

    // Size

    float width;
    float height;

    // Texture coordinates

    float u0;
    float v0;
    float u1;
    float v1;

    // Color

    float r;
    float g;
    float b;

    // Animation

    float animationType;
    float phase;
    float amplitude;

    // Transform

    float rotation;
    float opacity;
    float pivotX;
    float pivotY;
    float lightOffsetY;
    float clipDepth;
    float solid;
    float anchorOffsetX;
    float anchorOffsetY;

    float worldZ;
    float worldSpace;
    std::uint32_t normalAndMode;
    std::uint32_t materialParameters;
};

static_assert(std::is_standard_layout_v<GlyphInstance>);
static_assert(sizeof(GlyphInstance) == 108);
static_assert(offsetof(GlyphInstance, normalAndMode) == 100);
static_assert(offsetof(GlyphInstance, materialParameters) == 104);

enum class GlyphAnchor
{
    Baseline,
    Center
};

class GlyphRenderer
{
private:

    const FontAtlas& atlas;

    Shader shader;
    Mesh quad;

    GLuint instanceVBO = 0;
    std::size_t bufferCapacity = 0;

    GLuint lightmapTexture = 0;

    std::vector<GlyphInstance> instances;
    std::vector<GlyphInstance> orderedInstances;
    std::vector<float> depths;

    struct GrassFootprint { float x = 0, y = 0, time = -1000; };
    std::array<GrassFootprint, 6> grassFootprints{};
    std::size_t nextGrassFootprint = 0;

    struct WaterFootprint
    {
        float x = 0.0f;
        float y = 0.0f;
        float time = -1000.0f;
    };

    std::array<WaterFootprint, 6> waterFootprints{};

    std::size_t nextWaterFootprint = 0;

    bool playerWasInWater = false;



    void initializeInstanceBuffer();
    void appendDepthSortedInstances();

    // ========================================
    // STATIC GPU GEOMETRY CACHE
    // ========================================

    using GroundChunkKey = std::tuple<int, int, int>;

    struct StaticGroundChunk
    {
        GLuint buffer = 0;

        GLsizei count = 0;

        std::uint64_t lastUsed = 0;

        bool vegetation = false;
    };

    static constexpr std::size_t MAX_GPU_CHUNKS = 512;
    static constexpr std::size_t MAX_GPU_CACHE_BYTES = 96u * 1024u * 1024u;
    std::size_t cachedMemoryBytes = 0;
    std::size_t cacheHits = 0;
    std::size_t cacheMisses = 0;
    std::array<std::size_t, 3> treeLevels{};

    std::map<
        GroundChunkKey,
        StaticGroundChunk
    > groundChunks;

    std::vector<StaticGroundChunk*> activeGroundChunks;

    // Temporary geometry used only when creating a chunk.

    std::vector<GlyphInstance> capturedInstances;

    GroundChunkKey captureKey{};

    bool capturingGround = false;

    std::uint64_t cacheFrame = 0;

    // Per-frame statistics.

    std::size_t cachedGroundCount = 0;
    std::size_t cachedUploadBytes = 0;

    void removeOldGroundChunks();

public:
    std::size_t getInstanceCount() const;
    std::size_t getGroundInstanceCount() const { return instances.size(); }

    explicit GlyphRenderer(
        const FontAtlas& atlas
    );

    ~GlyphRenderer();

    GlyphRenderer(
        const GlyphRenderer&
    ) = delete;

    bool useRockCache(
        std::size_t rockId,
        int detailLevel
    );

    bool useTreeCache(
        std::size_t treeId,
        int detailLevel
    );
    void recordTreeLevel(int level) { ++treeLevels[level]; }
    void printCacheStats(std::ostream& out) const;

    void setCamera(const Camera& camera);

    GlyphRenderer& operator=(
        const GlyphRenderer&
    ) = delete;

    bool useVegetationChunk(
        int chunkX,
        int chunkY,
        int detailStep
    );

    void setScreenSize(
        int width,
        int height
    );

    void setTime(float time);
    void setGrassEnvironment(float x, float y, float height, float time,
                             float windX, float windY, float windStrength);

    void setWaterEnvironment(
        float x,
        float y,
        float time,
        bool inWater
    );

    void setLightmap(GLuint texture);

    void begin();

    void draw(
        char32_t character,
        float x,
        float y,
        float scale = 1.0f,
        Color color = {},
        GlyphAnimation animation = {},
        GlyphTransform transform = {},
        GlyphAnchor anchor = GlyphAnchor::Baseline,
        float lightOffsetY = 0.0f,
        std::optional<float> depth = {},
        bool solid = false,
        std::optional<float> worldZ = {},
        GlyphMaterial material = {}
    );

    void drawWorld(const Camera& camera, char32_t character, float x, float y, float z,
                   float scale, Color color, GlyphAnimation animation = {},
                   GlyphTransform transform = {}, GlyphAnchor anchor = GlyphAnchor::Center,
                   bool groundLayer = false, bool solid = false, GlyphMaterial material = {});

    void setSurfaceLighting(const SurfaceLighting& lighting, std::span<const PointLight> lights, int debugMode);

    void flush();

    // Returns true if the chunk already exists on GPU.

    bool useGroundChunk(
        int chunkX,
        int chunkY,
        int detailLevel
    );

    void finishGroundChunk();

    void clearGroundCache();

    std::size_t getCachedGroundCount() const
    {
        return cachedGroundCount;
    }

    std::size_t getCachedUploadBytes() const
    {
        return cachedUploadBytes;
    }

    std::size_t getCachedDrawCalls() const
    {
        return activeGroundChunks.size();
    }

    std::size_t getCachedChunkCount() const
    {
        return groundChunks.size();
    }

    std::size_t getCachedMemoryBytes() const;

};
