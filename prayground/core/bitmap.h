﻿#pragma once

#include <prayground/math/vec.h>

#ifndef __CUDACC__
#include <filesystem>
#include <prayground/gl/shader.h>
#include <prayground/app/window.h>
#include <map>
#include <variant>
#endif

namespace prayground {

    /// @todo Casting bitmap to OptixImage2D

    namespace impl {
        template <typename PixelT> struct PixelDecl {};
        template <> struct PixelDecl<uint8_t> { using V1 = uint8_t; using V2 = Vec2u; using V3 = Vec3u; using V4 = Vec4u; };
        template <> struct PixelDecl<float> { using V1 = float; using V2 = Vec2f; using V3 = Vec3f; using V4 = Vec4f; };
        template <> struct PixelDecl<int8_t> { using V1 = int8_t; using V2 = Vec2c; using V3 = Vec3c; using V4 = Vec4c; };
        template <> struct PixelDecl<double> { using V1 = double; using V2 = Vec2d; using V3 = Vec3d; using V4 = Vec4d; };
        template <> struct PixelDecl<int32_t> { using V1 = int32_t; using V2 = Vec2i; using V3 = Vec3i; using V4 = Vec4i; };
    } // namespace impl

    enum class PixelFormat : int 
    {
        NONE        = 0, 
        GRAY        = 1, 
        GRAY_ALPHA  = 2, 
        RGB         = 3, 
        RGBA        = 4
    };

    template <typename PixelT>
    class Bitmap_ {
    private:
#ifndef __CUDACC__
        using V1 = typename impl::PixelDecl<PixelT>::V1;
        using V2 = typename impl::PixelDecl<PixelT>::V2;
        using V3 = typename impl::PixelDecl<PixelT>::V3;
        using V4 = typename impl::PixelDecl<PixelT>::V4;
        using PixelVariant = std::variant<typename impl::PixelDecl<PixelT>::V1, typename impl::PixelDecl<PixelT>::V2, typename impl::PixelDecl<PixelT>::V3, typename impl::PixelDecl<PixelT>::V4>;

    public:
        using Type = PixelT;

        Bitmap_();
        Bitmap_(PixelFormat format, int width, int height, PixelT* data = nullptr);
        explicit Bitmap_(const std::filesystem::path& filename);
        explicit Bitmap_(const std::filesystem::path& filename, PixelFormat format);
        /// @todo: Check if "Disallow the copy-constructor"
        // Bitmap_(const Bitmap_& bmp) = delete;

        void allocate(PixelFormat format, int width, int height, PixelT* data = nullptr);
        void setData(PixelT* data, int offset_x, int offset_y, int width, int height);
        void setData(PixelT* data, const int2& offset, const int2& res);

        void load(const std::filesystem::path& filename);
        void load(const std::filesystem::path& filename, PixelFormat format);
        void write(const std::filesystem::path& filename, int quality=100) const;

        void draw() const;
        void draw(int32_t x, int32_t y) const;
        void draw(int32_t x, int32_t y, int32_t width, int32_t height) const;
    
        void allocateDevicePtr();
        void copyToDevice();
        void copyFromDevice();

        // Return pixel value in vector type. 
        // Please use std::get<VecT> to unpack pixel vector
        // 
        // Example: 
        // Bitmap<uint8_t> bmp(PixelFormat::RGB, 512, 512);
        // Vec3u pixel = std::get<Vec3u>(bmp.at(10, 10));
        PixelVariant at(int32_t x, int32_t y);

        PixelT* data() const { return m_data.get(); }
        PixelT* devicePtr() const { return d_data; }

        OptixImage2D toOptixImage2D() const;

        int width() const { return m_width; }
        int height() const { return m_height; }
        int channels() const { return m_channels; }
    private:
        void prepareGL();

        std::unique_ptr<PixelT[]> m_data;  // Data on CPU
        PixelT* d_data { nullptr };        //      on GPU

        PixelFormat m_format { PixelFormat::NONE };
        int m_width { 0 };
        int m_height { 0 };
        int m_channels { 0 };

        // Member variables to draw Bitmap on OpenGL context
        GLuint m_gltex;
        GLuint m_vbo, m_vao, m_ebo; // vertex buffer object, vertex array object, element buffer object
        gl::Shader m_shader;
#endif
    };

    using Bitmap = Bitmap_<uint8_t>;
    using FloatBitmap = Bitmap_<float>;

} // namespace prayground
