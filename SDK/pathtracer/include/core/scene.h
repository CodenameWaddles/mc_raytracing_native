#pragma once

#include <core/primitive.h>
#include <sutil/Camera.h>
#include <core/pathtracer.h>

namespace pt {

class Scene {
public:
    Scene() {}

    void create_hitgroup_programs(const OptixDeviceContext& ctx, const OptixModule& module);
    /** \brief build geomerty acceleration structure. */
    void build_gas(const OptixDeviceContext& ctx, AccelData& accel_data);
    /** 
     * \brief Create SBT with HitGroupData. 
     * \note SBTs for raygen and miss program is not created at this.
     */
    void create_hitgroup_sbt(const OptixModule& module, OptixShaderBindingTable& sbt);

    void add_primitive(const Primitive& p) { m_primitives.push_back(p); }
    std::vector<Primitive> get_primitives() const { return m_primitives; }

    void set_width(const unsigned int w) { m_width = w; }
    unsigned int get_width() const { return m_width; }

    void set_height(const unsigned int h) { m_height = h; }
    unsigned int get_height() const { return m_height; }

    void set_bgcolor(const float4& bg) { m_bgcolor = bg; }
    float4 get_bgcolor() const { return m_bgcolor; }
private:
    std::vector<Primitive> m_primitives;    // Primitives to describe the scene.
    unsigned int m_width, m_height;         // Dimensions of output result.
    float4 m_bgcolor;                       // Background color
    unsigned int m_depth;                   // Maximum depth of ray tracing.
    unsigned int m_samples_per_launch;      // Specify the number of samples per call of optixLaunch.
};

}