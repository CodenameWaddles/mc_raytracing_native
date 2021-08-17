#pragma once 

#include <cuda/random.h>
#include "../core/material.h"
#include "../core/bsdf.h"
#include "../texture/constant.h"

namespace oprt {

struct DielectricData {
    void* texdata;
    float ior;
    unsigned int tex_func_id;
};

#ifndef __CUDACC__

class Dielectric final : public Material {
public:
    Dielectric(const float3& a, float ior)
    : m_texture(new ConstantTexture(a)), m_ior(ior) { }

    Dielectric(const std::shared_ptr<Texture>& texture, float ior)
    : m_texture(texture), m_ior(ior) {}

    ~Dielectric() { }

    void prepareData() override {
        if (!m_texture->devicePtr())
            m_texture->prepareData();

        DielectricData data = {
            m_texture->devicePtr(), 
            m_ior, 
            m_texture->programId()
        };

        CUDA_CHECK(cudaMalloc(&d_data, sizeof(DielectricData)));
        CUDA_CHECK(cudaMemcpy(
            d_data,
            &data, sizeof(DielectricData),
            cudaMemcpyHostToDevice
        ));
    }

    void freeData() override
    {
        m_texture->freeData();
    }

    MaterialType type() const override { return MaterialType::Dielectric; }

private:
    // float3 m_albedo;
    std::shared_ptr<Texture> m_texture;
    float m_ior;
};

#else 

#endif

}