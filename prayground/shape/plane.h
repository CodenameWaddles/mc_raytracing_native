#pragma once

#ifndef __CUDACC__
#include <prayground/core/shape.h>
#include <prayground/core/cudabuffer.h>
#endif

namespace prayground {

struct PlaneData 
{
    float2 min;
    float2 max;
};

#ifndef __CUDACC__
class Plane final : public Shape {
public:
    explicit Plane(const float2& min, const float2& max) : m_min(min), m_max(max) {}

    OptixBuildInputType buildInputType() const override { return OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES; }

    void copyToDevice() override
    {
        PlaneData data = {
            .min = m_min, 
            .max = m_max
        };

        if (!d_data)
            CUDA_CHECK( cudaMalloc( &d_data, sizeof(PlaneData) ) );
        CUDA_CHECK( cudaMemcpy(
            d_data, 
            &data, sizeof(PlaneData), 
            cudaMemcpyHostToDevice
        ));
    }

    void buildInput( OptixBuildInput& bi ) override
    {
        CUDABuffer<uint32_t> d_sbt_indices;
        uint32_t* sbt_indices = new uint32_t[1];
        sbt_indices[0] = m_sbt_index;
        d_sbt_indices.copyToDevice(sbt_indices, sizeof(uint32_t));

        // Prepare bounding box information on the device.
        OptixAabb aabb = static_cast<OptixAabb>(bound());

        if (d_aabb_buffer) free();

        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_aabb_buffer), sizeof(OptixAabb)));
        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(d_aabb_buffer),
            &aabb,
            sizeof(OptixAabb),
            cudaMemcpyHostToDevice));

        unsigned int* input_flags = new unsigned int[1];
        input_flags[0] = OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;

        bi.type = OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES;
        bi.customPrimitiveArray.aabbBuffers = &d_aabb_buffer;
        bi.customPrimitiveArray.numPrimitives = 1;
        bi.customPrimitiveArray.flags = input_flags;
        bi.customPrimitiveArray.numSbtRecords = 1;
        bi.customPrimitiveArray.sbtIndexOffsetBuffer = d_sbt_indices.devicePtr();
        bi.customPrimitiveArray.sbtIndexOffsetSizeInBytes = sizeof(uint32_t);
        bi.customPrimitiveArray.sbtIndexOffsetStrideInBytes = sizeof(uint32_t);
    }

    AABB bound() const override
    {
        AABB box{make_float3(m_min.x, -0.01f, m_min.y), make_float3(m_max.x, 0.01f, m_max.y)};
        return box;
    }
private:
    float2 m_min, m_max;
};
#endif // __CUDACC__

} // ::prayground