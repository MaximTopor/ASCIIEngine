#include "renderer/SurfaceSampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace SurfaceSampler
{
    namespace
    {
        constexpr float pi = std::numbers::pi_v<float>;
        constexpr float tau = 2.0f*pi;

        float random(std::uint32_t& state)
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float>(state >> 8) / 16777216.0f;
        }

        int divisions(float length, float spacing)
        {
            return std::max(1, static_cast<int>(std::ceil(length / spacing)));
        }

        Vec perpendicular(Vec axis)
        {
            return unit(cross(axis, std::abs(axis.z) < 0.8f ? Vec{0,0,1} : Vec{0,1,0}));
        }

        Vec spline(Vec a, Vec b, Vec c, Vec d, float t)
        {
            return b + ((c-a)*t + (a*2-b*5+c*4-d)*(t*t)
                + (b*3-a-c*3+d)*(t*t*t))*0.5f;
        }

        void rings(Vec center, Vec radii, Vec axis, float minAngle, float maxAngle,
                   Settings settings, const Sink& emit, const RadiusProfile& profile = {})
        {
            if (settings.spacing <= 0 || radii.x <= 0 || radii.y <= 0 || radii.z <= 0) return;
            const Vec x = perpendicular(axis);
            const Vec y = cross(axis,x);
            auto state = settings.seed;
            const int rows = divisions((maxAngle-minAngle) *
                (radii.z + (radii.x+radii.y)*0.5f)*0.5f, settings.spacing);
            const float jitter = std::clamp(settings.jitter,0.0f,0.8f);
            for (int row = 0; row < rows; ++row)
            {
                const float middle = std::lerp(minAngle,maxAngle,(row+0.5f)/rows);
                const int count = divisions(pi*(radii.x+radii.y)*std::sin(middle),settings.spacing);
                for (int i = 0; i < count; ++i)
                {
                    const float u = (i+0.5f+0.27f*(row%2)+(random(state)-0.5f)*jitter)/count;
                    const float v = (row+0.5f+(random(state)-0.5f)*jitter)/rows;
                    const float phi = std::lerp(minAngle,maxAngle,v);
                    const float s = std::sin(phi), c = std::cos(phi);
                    const float ca = std::cos(tau*u), sa = std::sin(tau*u);
                    const Vec r = x*(ca*radii.x) + y*(sa*radii.y);
                    const Vec offset = r*s+axis*(c*radii.z);
                    Vec n = unit((x*(ca/radii.x)+y*(sa/radii.y))*s + axis*(c/radii.z));
                    Vec tangent = x*(-sa*radii.x)+y*(ca*radii.y);
                    float deformation = 1.0f;
                    if (profile)
                    {
                        constexpr float eps = 0.001f;
                        const float latitude = phi/pi;
                        deformation = profile(u,latitude);
                        const float du = (profile(u+eps,latitude)-profile(u-eps,latitude))/(2*eps*tau);
                        const float dv = (profile(u,latitude+eps)-profile(u,latitude-eps))/(2*eps*pi);
                        const Vec along = (r*c-axis*(s*radii.z))*deformation+offset*dv;
                        tangent = tangent*(s*deformation)+offset*du;
                        n = unit(cross(along,tangent));
                    }
                    emit({center+offset*deformation,n,unit(tangent),settings.spacing,u,phi/pi,random(state)});
                }
            }
        }
    }

    float dot(Vec a, Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
    Vec cross(Vec a, Vec b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
    float length(Vec v) { return std::sqrt(dot(v,v)); }
    Vec unit(Vec v) { const float l = length(v); return l > 0.00001f ? v*(1.0f/l) : Vec{}; }

    void tube(std::span<const TubePoint> points, Settings settings, const Sink& emit,
              const RadiusProfile& profile)
    {
        if (points.size() < 2 || settings.spacing <= 0.0f) return;
        struct Station { Vec center; float radius, distance; };
        std::vector<Station> path{{points.front().position,std::max(0.0f,points.front().radius),0.0f}};
        for (std::size_t i = 0; i+1 < points.size(); ++i)
        {
            const auto& b = points[i];
            const auto& c = points[i+1];
            const Vec a = i > 0 ? points[i-1].position : b.position*2-c.position;
            const Vec d = i+2 < points.size() ? points[i+2].position : c.position*2-b.position;
            for (int step = 1; step <= 12; ++step)
            {
                const float t = step/12.0f;
                const Vec p = spline(a,b.position,c.position,d,t);
                const float distance = length(p-path.back().center);
                if (distance > 0.00001f)
                    path.push_back({p,std::max(0.0f,std::lerp(b.radius,c.radius,t)),path.back().distance+distance});
            }
        }
        const float total = path.back().distance;
        if (total <= 0.00001f) return;
        const int rows = divisions(total,settings.spacing);
        auto state = settings.seed;
        Vec frame{};
        std::size_t segment = 1;
        const float jitter = std::clamp(settings.jitter,0.0f,0.8f);
        for (int row = 0; row < rows; ++row)
        {
            const float v = (row+0.5f+(random(state)-0.5f)*jitter)/rows;
            const float distance = v*total;
            while (path[segment].distance < distance) ++segment;
            const auto& a = path[segment-1];
            const auto& b = path[segment];
            const float t = (distance-a.distance)/(b.distance-a.distance);
            const Vec center = a.center+(b.center-a.center)*t;
            const Vec axis = unit(b.center-a.center);
            // Parallel transport by projection avoids a frame flip between adjacent rings.
            frame = unit(frame-axis*dot(frame,axis));
            if (length(frame) < 0.001f) frame = perpendicular(axis);
            const Vec side = cross(axis,frame);
            const float radius = std::lerp(a.radius,b.radius,t);
            const float slope = (b.radius-a.radius)/(b.distance-a.distance);
            if (radius <= 0.001f) continue;
            const int count = std::max(3,divisions(tau*radius,settings.spacing));
            for (int i = 0; i < count; ++i)
            {
                const float u = (i+0.34f*(row%2)+(random(state)-0.5f)*jitter)/count;
                const float ca = std::cos(tau*u), sa = std::sin(tau*u);
                const Vec radial = frame*ca+side*sa;
                const Vec around = side*ca-frame*sa;
                float multiplier = 1.0f, du = 0.0f, dv = 0.0f;
                if (profile)
                {
                    constexpr float eps = 0.001f;
                    multiplier = std::max(0.05f,profile(u,v));
                    du = (profile(u+eps,v)-profile(u-eps,v))/(2*eps*tau);
                    dv = (profile(u,v+eps)-profile(u,v-eps))/(2*eps*total);
                }
                const Vec axial = axis+radial*(slope*multiplier+radius*dv);
                const Vec tangent = unit(around*multiplier+radial*du);
                const float inkSpacing = std::clamp(2.2f*radius,settings.spacing*0.32f,settings.spacing);
                emit({center+radial*(radius*multiplier),unit(cross(tangent,axial)),unit(axial),
                      inkSpacing,u,v,random(state)});
            }
        }
    }

    void frustum(Vec a, Vec b, float startRadius, float endRadius, Settings settings, const Sink& emit)
    {
        const std::array points{TubePoint{a,startRadius},TubePoint{b,endRadius}};
        tube(points,settings,emit);
    }

    void cylinder(Vec a, Vec b, float radius, Settings settings, const Sink& emit)
    {
        frustum(a,b,radius,radius,settings,emit);
    }

    void ellipsoid(Vec center, Vec radii, Settings settings, const Sink& emit,
                   const RadiusProfile& profile)
    {
        rings(center,radii,{0,0,1},0,pi,settings,emit,profile);
    }

    void sphere(Vec center, float radius, Settings settings, const Sink& emit)
    {
        ellipsoid(center,{radius,radius,radius},settings,emit);
    }

    void capsule(Vec a, Vec b, float radius, Settings settings, const Sink& emit)
    {
        if (length(b-a) <= 0.00001f) { sphere(a,radius,settings,emit); return; }
        const Vec axis = unit(b-a);
        cylinder(a,b,radius,settings,emit);
        rings(a,{radius,radius,radius},axis,pi*0.5f,pi,settings,emit);
        rings(b,{radius,radius,radius},axis,0,pi*0.5f,settings,emit);
    }

    void parametric(const Mapping& mapping, float uLength, float vLength,
                    Settings settings, const Sink& emit)
    {
        if (!mapping || settings.spacing <= 0 || uLength <= 0 || vLength <= 0) return;
        const int columns = divisions(uLength,settings.spacing);
        const int rows = divisions(vLength,settings.spacing);
        auto state = settings.seed;
        constexpr float eps = 0.001f;
        const float jitter = std::clamp(settings.jitter,0.0f,0.8f);
        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                const float u = (column+0.5f+0.08f*(row%2)+(random(state)-0.5f)*jitter)/columns;
                const float v = (row+0.5f+(random(state)-0.5f)*jitter)/rows;
                const Vec tangent = unit(mapping(std::min(1.0f,u+eps),v)-mapping(std::max(0.0f,u-eps),v));
                const Vec bitangent = mapping(u,std::min(1.0f,v+eps))-mapping(u,std::max(0.0f,v-eps));
                const Vec normal = unit(cross(tangent,bitangent));
                if (length(normal) > 0.0f)
                    emit({mapping(u,v),normal,tangent,settings.spacing,u,v,random(state)});
            }
        }
    }

    void plane(Vec origin, Vec uEdge, Vec vEdge, Settings settings, const Sink& emit)
    {
        parametric([=](float u,float v) { return origin+uEdge*u+vEdge*v; },
                   length(uEdge),length(vEdge),settings,emit);
    }
}
