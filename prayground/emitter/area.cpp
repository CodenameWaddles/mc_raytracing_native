#include "area.h"

namespace prayground {

// ---------------------------------------------------------------------------
AreaEmitter::AreaEmitter(const std::shared_ptr<Texture>& texture, float intensity, bool twosided)
: m_texture(texture), m_intensity(intensity), m_twosided(twosided)
{

}

// ---------------------------------------------------------------------------
SurfaceType AreaEmitter::surfaceType() const 
{
    return SurfaceType::AreaEmitter;
}
    
// ---------------------------------------------------------------------------
void AreaEmitter::copyToDevice() 
{
    if (!m_texture->devicePtr())
        m_texture->copyToDevice();

    AreaEmitterData data = {
        .tex_data = m_texture->devicePtr(),
        .intensity = m_intensity,
        .twosided = m_twosided,
        .tex_program_id = m_texture->programId()
    };

    CUDA_CHECK(cudaMalloc(&d_data, sizeof(AreaEmitterData)));
    CUDA_CHECK(cudaMemcpy(
        d_data,
        &data, sizeof(AreaEmitterData),
        cudaMemcpyHostToDevice
    ));
}

std::shared_ptr<Texture> AreaEmitter::texture() const
{
    return m_texture;
}

void AreaEmitter::free()
{
    
}

}