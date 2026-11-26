#include "renderer/SurfaceSampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace SurfaceSampler;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    bool near(float a, float b, float tolerance = 0.002f)
    {
        return std::abs(a-b) < tolerance;
    }

    template<class Generate>
    std::vector<Sample> collect(const Generate& generate)
    {
        std::vector<Sample> samples;
        generate([&](const Sample& sample)
        {
            require(std::isfinite(sample.position.x) && std::isfinite(sample.position.y)
                && std::isfinite(sample.position.z), "non-finite sample position");
            require(near(length(sample.normal),1.0f), "non-unit normal");
            require(near(length(sample.tangent),1.0f), "non-unit tangent");
            require(near(dot(sample.normal,sample.tangent),0.0f), "non-orthogonal tangent");
            require(sample.spacing > 0, "invalid spacing");
            samples.push_back(sample);
        });
        require(!samples.empty(),"surface emitted no samples");
        return samples;
    }
}

int main()
{
    try
    {
        const Settings settings{2.0f,0.25f,420};
        const auto cylinderSamples = [&](Settings s)
        {
            return collect([&](const Sink& sink) { cylinder({0,0,0},{0,0,24},6,s,sink); });
        };
        const auto a = cylinderSamples(settings);
        const auto b = cylinderSamples(settings);
        require(a.size() == b.size(), "same seed changed the sample count");
        for (std::size_t i=0;i<a.size();++i)
        {
            const auto& sample = a[i];
            require(length(sample.position-b[i].position) == 0.0f, "same seed changed surface positions");
            require(sample.variation == b[i].variation, "same seed changed variations");
            require(near(std::hypot(sample.position.x,sample.position.y),6.0f), "cylinder radius mismatch");
            require(near(sample.normal.z,0.0f), "cylinder normal is tilted");
        }
        require(length(a.front().position-cylinderSamples({2.0f,0.25f,421}).front().position)>0.001f,
            "seed did not vary sample positions");
        require(cylinderSamples({1.0f,0.25f,420}).size()>a.size()*3,"finer spacing did not increase density");

        const auto tapered = collect([&](const Sink& s) { frustum({}, {0,0,28},8,0,settings,s); });
        for (const auto& p:tapered)
        {
            require(near(std::hypot(p.position.x,p.position.y),8*(1-p.position.z/28)), "incorrect taper");
            require(p.normal.z > 0.2f,"taper did not affect the normal");
        }

        const std::array path{TubePoint{{0,0,0},5},TubePoint{{0,4,12},4},
                              TubePoint{{4,6,22},2},TubePoint{{10,6,27},0.2f}};
        collect([&](const Sink& s)
        {
            tube(path, settings, s, [](float u,float v){return 1+0.14f*std::cos(u*18.84956f+v);});
        });
        for (const auto& p:collect([&](const Sink& s) { sphere({},6,settings,s); }))
            require(near(length(p.position),6) && dot(unit(p.position),p.normal)>0.999f,"incorrect sphere");
        for (const auto& p:collect([&](const Sink& s) { ellipsoid({},{9,6,4},settings,s); }))
        {
            const Vec normalized{p.position.x/9,p.position.y/6,p.position.z/4};
            require(near(length(normalized),1),"incorrect ellipsoid");
        }
        const auto deformed = collect([&](const Sink& s) {
            ellipsoid({},{9,6,4},settings,s,[](float u,float v) {
                return 1.0f + 0.15f*std::sin(6.2831853f*u)*std::sin(3.1415927f*v);
            });
        });
        bool deformedNormal = false;
        for (const auto& p : deformed)
        {
            const Vec originalNormal = unit({p.position.x/81,p.position.y/36,p.position.z/16});
            deformedNormal |= dot(originalNormal,p.normal) < 0.999f;
            require(dot(p.position,p.normal)>0.0f,"deformed ellipsoid normal points inward");
        }
        require(deformedNormal,"radial deformation did not change normals");
        const auto rounded = collect([&](const Sink& s) { capsule({0,0,0},{0,0,15},5,settings,s); });
        bool lower=false, upper=false;
        for (const auto& p:rounded)
        {
            lower |= p.position.z < 0;
            upper |= p.position.z > 15;
            require(near(length(p.position-Vec{0,0,std::clamp(p.position.z,0.0f,15.0f)}),5),
                "incorrect capsule radius");
        }
        require(lower && upper,"capsule missing rounded ends");
        for (const auto& p:collect([&](const Sink& s) { plane({0,0,2},{18,0,0},{0,6,0},settings,s); }))
            require(near(p.position.z,2) && near(p.normal.z,1),"incorrect plane or normal");
        for (const auto& p:collect([&](const Sink& s) {
            parametric([](float u,float v){return Vec{u*8,v*10,u*u*3};},10,10,settings,s);
        }))
            require(near(p.position.z,3*p.u*p.u) && p.normal.z>0.0f,"incorrect parametric surface");

        std::cout << "Seven primitives plus parameter mapping passed checks for density, normals and determinism.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
