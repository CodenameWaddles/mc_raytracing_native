#pragma once 

#include <optix.h>
#include <prayground/math/vec_math.h>
#include <prayground/optix/sbt.h>

namespace prayground {

  using ConstantTexture = ConstantTexture_<Vec3f>;

  enum class RayType : uint32_t
  {
    RADIANCE = 0,
    SHADOW = 1,
    N_RAY = 2
  };

  struct Light
  {
    Vec3f pos;
  };

  struct LaunchParams
  {
    unsigned int width, height;
    Light light;
    uint32_t max_depth;

    Vec4u* result_buffer;

    OptixTraversableHandle handle;
  };

  struct RaygenData
  {
    Camera::Data camera;
  };

  struct HitgroupData
  {
    void* shape_data;
    SurfaceInfo surface_info;
  };

  struct MissData
  {
    void* env_data;
  };

  struct EmptyData
  {

  };

  //extern "C" LaunchParams params;

} // namespace prayground