#include "conductor.h"

namespace prayground {

// ------------------------------------------------------------------
Conductor::Conductor(const std::shared_ptr<Texture>& texture, bool twosided)
: m_texture(texture), m_twosided(twosided)
{
    
}

Conductor::~Conductor()
{

}

// ------------------------------------------------------------------
SurfaceType Conductor::surfaceType() const
{
    return SurfaceType::Reflection;
}

// ------------------------------------------------------------------
void Conductor::copyToDevice()
{
    if (!m_texture->devicePtr())
        m_texture->copyToDevice();

    Data data = {
        .texture = m_texture->getData(),
        .twosided = m_twosided
    };

    if (!d_data)
        CUDA_CHECK(cudaMalloc(&d_data, sizeof(Data)));
    CUDA_CHECK(cudaMemcpy(
        d_data,
        &data, sizeof(Data), 
        cudaMemcpyHostToDevice
    ));
}

void Conductor::free()
{
    m_texture->free();
    Material::free();
}

// ------------------------------------------------------------------
void Conductor::setTexture(const std::shared_ptr<Texture>& texture)
{
    m_texture = texture;
}

std::shared_ptr<Texture> Conductor::texture() const 
{
    return m_texture;
}

} // ::prayground