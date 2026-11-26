#pragma once

#include "world/Tile.hpp"
#include "world/Rock.hpp"
#include "world/WorldScale.hpp"

#include <vector>
#include <utility>

enum class TreeSpecies { Oak, Birch, Pine };

constexpr float treeBaseRadiusRatio(TreeSpecies species)
{
    switch (species)
    {
        case TreeSpecies::Oak: return 0.056f;
        case TreeSpecies::Birch: return 0.024f;
        case TreeSpecies::Pine: return 0.033f;
    }
    return 0.056f;
}

struct Tree
{
    float x;
    float y;
    float height;
    std::uint32_t seed;
    TreeSpecies species = TreeSpecies::Oak;
};

class World
{
private:
    int width;
    int height;
    int seed;
    WorldScale scale;

    std::vector<Tile> tiles;
    std::vector<Rock> rocks;
    std::vector<bool> rockFootprint;
    std::vector<Tree> trees;
    std::vector<bool> treeTrunks;

    void generate();
    void generateRocks();
    void generateTrees();
    void generateBenchmark();
    void placeTree(float x, float y, float treeHeight, std::uint32_t treeSeed, TreeSpecies species);
    bool placeRock(float x, float y, std::uint32_t objectSeed, float diameter, bool solid = true);
    std::pair<int, int> findSpawnCell() const;

public:
    World(
        int width,
        int height,
        int seed = 42,
        WorldScale scale = {},
        bool benchmark = false
    );

    bool isInside(int x, int y) const;
    bool isWalkable(int x, int y) const;
    bool isWalkableAt(
        float worldX,
        float worldY,
        float radius
    ) const;

    const Tile& getTile(int x, int y) const;
    bool hasRock(int x, int y) const;
    float getLightOpacity(int x, int y) const;
    const WorldScale& getScale() const { return scale; }
    const std::vector<Rock>& getRocks() const { return rocks; }
    const std::vector<Tree>& getTrees() const { return trees; }

    std::pair<int, int> findSpawn() const;

    int getWidth() const;
    int getHeight() const;
};
