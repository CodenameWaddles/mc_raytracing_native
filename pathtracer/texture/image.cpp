#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb/stb_image.h"

namespace pt {
#ifndef __CUDACC__

ImageTexture::ImageTexture(const std::string& filename)
{
    uint8_t* d = stbi_load( filename.c_str(), &width, &height, &channels, STBI_rgb_alpha );
    Assert(d, "Failed to load image file'"+filename+"'");
    data = new uchar4[width*height];
    format = UNSIGNED_BYTE4;
    memcpy(data, d, width*height*STBI_rgb_alpha);

    stbi_image_free(d);

    _init_texture_desc();
}

void ImageTexture::prepare_data() {
    // Alloc CUDA array in device memory.
    int32_t pitch = width * STBI_rgb_alpha * sizeof( unsigned char );
    cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<uchar4>();
    CUDA_CHECK( cudaMallocArray( &d_array, &channel_desc, width, height ) );
    CUDA_CHECK( cudaMemcpy2DToArray( d_array, 0, 0, (unsigned char*)data, pitch, pitch, height, cudaMemcpyHostToDevice ) );

    // Create texture object.
    cudaResourceDesc res_desc = {};
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = d_array;

    cudaTextureObject_t texture = 0;
    CUDA_CHECK( cudaCreateTextureObject( &texture, &res_desc, &tex_desc, nullptr ) );

    ImageTextureData img_texture_data = { texture };

    CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>(&d_data), sizeof(ImageTextureData) ) );
    CUDA_CHECK( cudaMemcpy(
        reinterpret_cast<void*>(d_data), 
        &img_texture_data, sizeof(ImageTextureData), 
        cudaMemcpyHostToDevice
    ));
}

void ImageTexture::_init_texture_desc() {
    tex_desc.addressMode[0] = cudaAddressModeClamp;
    tex_desc.addressMode[1] = cudaAddressModeClamp;
    tex_desc.filterMode = cudaFilterModeLinear;
    tex_desc.readMode = cudaReadModeNormalizedFloat;
    tex_desc.normalizedCoords = 1;
    tex_desc.maxAnisotropy = 1;
    tex_desc.maxMipmapLevelClamp = 99;
    tex_desc.minMipmapLevelClamp = 0;
    tex_desc.mipmapFilterMode = cudaFilterModePoint;
    tex_desc.borderColor[0] = 1.0f;
    tex_desc.sRGB = 1;
}

#endif
}