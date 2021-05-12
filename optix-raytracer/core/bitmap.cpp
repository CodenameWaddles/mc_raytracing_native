#include "bitmap.h"
#include "color.h"
#include "cudabuffer.h"
#include "file_util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../ext/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../ext/stb/stb_image_write.h"

namespace oprt {

// --------------------------------------------------------------------
template <class T>
void Bitmap<T>::allocate(int width, int height)
{
    m_width = width; m_height = height;
    Assert(!this->m_data, "Image data in the host side is already allocated.");
    m_data = new T[m_width*m_height];
}
template void Bitmap<uchar4>::allocate(int, int);
template void Bitmap<float4>::allocate(int, int);
template void Bitmap<uchar3>::allocate(int, int);
template void Bitmap<float3>::allocate(int, int);

// --------------------------------------------------------------------
/**
 * @todo T = float4 でも jpg ファイルを読み込みたい場合に対応する...?
 */
template <class T>
void Bitmap<T>::load(const std::string& filename) {
    // 画像ファイルが存在するかのチェック
    Assert(std::filesystem::exists(filename.c_str()), "The image file '" + filename + "' is not found.");

    using loadable_type = std::conditional_t< 
        (std::is_same_v<T, uchar4> || std::is_same_v<T, float4>), 
        uchar4, 
        uchar3
    >;

    std::string file_extension = get_extension(filename);
    if (file_extension == ".png" || file_extension == ".PNG")

    if (file_extension == ".png" || file_extension == ".PNG")
        Assert(sizeof(loadable_type) == 4, "The type of bitmap must have 4 channels (RGBA) when loading PNG file.");
    else if (file_extension == ".jpg" || file_extension == ".JPG")
        Assert(sizeof(loadable_type) == 3, "The type of bitmap must have 3 channels (RGB) when loading JPG file.");

    loadable_type* data = reinterpret_cast<loadable_type*>(
        stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, sizeof(loadable_type)));
    
    // PNG
    if (file_extension == ".png" || file_extension == ".PNG")
    {
        static_assert(std::is_same_v<T, float4> || std::is_same_v<T, uchar4>, "The type of bitmap should be float4 or uchar4 when load .png file.");

        uchar4* d = reinterpret_cast<uchar4*>(stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha));
        data = new T[m_width*m_height];
        Assert(d, "Failed to load image file'" + filename + "'");

        if constexpr (std::is_same_v<T, float4>)
        {
            float4* fd = new float4[this->m_width*this->m_height];
            for (int i=0; i<this->m_width; i++)
            {
                for (int j=0; j<this->m_height; j++)
                {
                    int idx = i * this->m_height + j;
                    fd[idx] = color2float(d[idx]);
                }
            }
            memcpy(this->m_data, fd, this->m_width*this->m_height*sizeof(T));
            delete[] fd;
        }
        else
        {
            memcpy(this->m_data, d, this->m_width*this->m_height*sizeof(T));
        }
        stbi_image_free(d);
    }
    // JPG
    else if (file_extension == ".jpg" || file_extension == ".JPG")
    {
        static_assert(std::is_same_v<T, float3> || std::is_same_v<T, uchar3>, "The type of bitmap should be float3 or uchar3 when load .jpg file.");

        // stbライブラリを使って画像読み込み
        uchar3* d = reinterpret_cast<uchar3*>(stbi_load(filename.c_str(), &this->m_width, &this->m_height, &this->m_channels, STBI_rgb));
        // データ読み込みができているかチェック
        Assert(d, "Failed to load image file'" + filename + "'");

        this->m_data = new T[this->m_width*this->m_height];
        // stbライブラリはchar型で読み込むので、float4の場合はデータの中身を変換する
        if constexpr (std::is_same_v<T, float3>)
        {
            float3* fd = new float3[this->m_width*this->m_height];
            for (int i=0; i<this->m_width; i++)
            {
                for (int j=0; j<this->m_height; j++)
                {
                    int idx = i * this->m_height + j;
                    fd[idx] = color2float(d[idx]);
                }
            }
            memcpy(this->m_data, fd, this->m_width*this->m_height*sizeof(T));
            delete[] fd;
        }
        else
        {
            memcpy(this->m_data, d, this->m_width*this->m_height*sizeof(T));
        }
        stbi_image_free(d);
    }
}
template void Bitmap<uchar4>::load(const std::string&);
template void Bitmap<float4>::load(const std::string&);
template void Bitmap<uchar3>::load(const std::string&);
template void Bitmap<float3>::load(const std::string&);

// --------------------------------------------------------------------
template <class T>
void Bitmap<T>::write(const std::string& filename, bool gamma_enabled, int quality) const
{
    // Tの方によって出力時の型をuchar4 or uchar3 で切り替える。
    using writable_type = std::conditional_t< 
        (std::is_same_v<T, uchar4> || std::is_same_v<T, float4>), 
        uchar4, 
        uchar3
    >;

    std::string file_extension = get_extension(filename);

    writable_type* data = new writable_type[m_width*m_height];
    // Tのデータ型が浮動小数点の場合はchar型に変換する。
    if constexpr (std::is_same_v<T, float4> || std::is_same_v<T, float3>)
    {
        for (int i=0; i<m_width; i++)
        {
            for (int j=0; j<m_height; j++)
            {
                int idx = i * m_height + j;
                data[idx] = make_color(m_data[idx], gamma_enabled);
            }
        }
    }
    else 
    {
        memcpy(data, m_data, m_width*m_height*sizeof(T));
    }
    
    if (file_extension == ".png" || file_extension == ".PNG")
    {
        stbi_write_png(filename, m_width, m_height, m_channels, data, 0);
    }
    else if (file_extension == ".jpg" || file_extension == ".JPG")
    {
        stbi_write_jpg(filename, m_width, m_height, m_channels, data, quality);
    }

    delete[] data;
}
template void Bitmap<float4>::write(const std::string&, bool, int) const;
template void Bitmap<uchar4>::write(const std::string&, bool, int) const;
template void Bitmap<float3>::write(const std::string&, bool, int) const;
template void Bitmap<uchar3>::write(const std::string&, bool, int) const;

// --------------------------------------------------------------------
template <class T>
void Bitmap<T>::copy_to_device() {
    // CPU側のデータが準備されているかチェック
    Assert(this->m_data, "Image data in the host side has been not allocated yet.");

    // GPU上に画像データを準備
    CUDABuffer<T> d_buffer;
    d_buffer.alloc_copy(this->m_data, this->m_width*this->m_height*sizeof(T));
    d_data = d_buffer.data();
}
template void Bitmap<uchar4>::copy_to_device();
template void Bitmap<uchar3>::copy_to_device();
template void Bitmap<float4>::copy_to_device();
template void Bitmap<float3>::copy_to_device();

}