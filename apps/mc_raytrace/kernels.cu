#include <prayground/prayground.h>
#include "params.h"

extern "C" __constant__ LaunchParams params;

struct SurfaceInteraction {
    Vec3f p;
    Vec3f n;
    Vec3f incoming;
    Vec3f albedo;
    double alpha;
    Vec2f uv;
    SurfaceInfo surface_info;
};

static INLINE DEVICE SurfaceInteraction* getSurfaceInteraction()
{
    const uint32_t u0 = getPayload<0>();
    const uint32_t u1 = getPayload<1>();
    return reinterpret_cast<SurfaceInteraction*>(unpackPointer(u0, u1));
}

INLINE DEVICE void trace(
    OptixTraversableHandle handle, 
    const Vec3f& ro, const Vec3f& rd, 
    float tmin, float tmax, SurfaceInteraction* si)
{
    uint32_t u0, u1;
    packPointer(si, u0, u1);
    optixTrace(
        handle, ro, rd, 
        tmin, tmax, 0.0f, 
        OptixVisibilityMask(2), 
        OPTIX_RAY_FLAG_NONE, 
       (uint32_t)RayType::RADIANCE, (uint32_t)RayType::N_RAY, (uint32_t)RayType::RADIANCE,
        u0, u1);
}

static INLINE DEVICE bool shadowTrace(
  OptixTraversableHandle handle, const Vec3f& ro, const Vec3f& rd,
  float tmin, float tmax, int visibilityMask)
{
  return false;
  uint32_t hit = 0u;
  optixTrace(handle, ro.toCUVec(), rd.toCUVec(),
    tmin, tmax, 0.0f,
    OptixVisibilityMask(visibilityMask), OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT,
    (uint32_t)RayType::SHADOW, (uint32_t)RayType::N_RAY, (uint32_t)RayType::SHADOW,
    hit);
  return static_cast<bool>(hit);
}

// Raygen -------------------------------------------------------------------------------
extern "C" __device__ void __raygen__pinhole()
{
    const RaygenData* raygen = reinterpret_cast<RaygenData*>(optixGetSbtDataPointer());

    const Vec3ui idx(optixGetLaunchIndex());

    Vec3f color(0.0f);
    Vec3f throughput(1.0f);

    SurfaceInteraction si;
    si.albedo = Vec3f(0.f);
    si.surface_info.type = SurfaceType::None;

    const Vec2f res(params.width, params.height);
    const Vec2f d = 2.0f * (Vec2f(idx.x(), idx.y()) / res) - 1.0f;
    Vec3f ro, rd;
    getCameraRay(raygen->camera, d.x(), d.y(), ro, rd);

    int depth = 0;
    for (;;) {
        break;
      if (depth >= params.max_depth)
        break;

      si.surface_info.type = SurfaceType::None;

      trace(params.handle, ro, rd, 0.00001f, 1e16f, &si); // using miss or closest_hit functions

      si.incoming = rd;

      if (si.surface_info.type == SurfaceType::Diffuse) // Diffuse mesh
      {
        Vec3f radiance = optixContinuationCall<Vec3f, SurfaceInteraction*, void*, const Vec3f&>(
          si.surface_info.callable_id.bsdf, &si, si.surface_info.data, Vec3f{}); // calling __continuation_callable__diffuse_shading

        color += radiance * throughput;
        depth = params.max_depth; // stop !
      }
      else if ((si.surface_info.type == SurfaceType::Reflection) || (si.surface_info.type == SurfaceType::Refraction))
      {
        Vec3f outputDir = optixDirectCall<Vec3f, SurfaceInteraction*, void*>(
          si.surface_info.callable_id.sample, &si, si.surface_info.data);
        // calling __direct_callable__reflection_direction or __direct_callable__refraction_direction

        Vec3f Coef = optixContinuationCall<Vec3f, SurfaceInteraction*, void*, const Vec3f&>(
          si.surface_info.callable_id.bsdf, &si, si.surface_info.data, Vec3f{});
        // calling __continuation_callable__reflection_shading or __direct_callable__refraction_direction

        throughput *= Coef;

        ro = si.p;
        rd = outputDir;
        depth++;
      }
    }

    const uint32_t image_index = idx.y() * params.width + idx.x();
    
    Vec3u result = make_color(color);
    params.result_buffer[image_index] = Vec4u(result, 255);
}

// Miss -------------------------------------------------------------------------------
extern "C" __device__ void __miss__envmap()
{
    MissData* data = reinterpret_cast<MissData*>(optixGetSbtDataPointer());
    const auto* env = reinterpret_cast<EnvironmentEmitter::Data*>(data->env_data);
    auto* si = getSurfaceInteraction();

    Ray ray = getWorldRay();

    const float a = dot(ray.d, ray.d);
    const float half_b = dot(ray.o, ray.d);
    const float c = dot(ray.o, ray.o) - 1e8f*1e8f;
    const float discriminant = half_b * half_b - a*c;

    float sqrtd = sqrtf(discriminant);
    float t = (-half_b + sqrtd) / a;

    Vec3f p = normalize(ray.at(t));

    float phi = atan2(p.z(), p.x());
    float theta = asin(p.y());
    float u = 1.0f - (phi + math::pi) / (2.0f * math::pi);
    float v = 1.0f - (theta + math::pi / 2.0f) * math::inv_pi;
    
    si->uv = Vec2f(u, v);
    si->n = Vec3f(0.0f);
    si->p = p;
    Vec3f color = optixDirectCall<Vec3f, SurfaceInteraction*, void*>(
        env->texture.prg_id, si, env->texture.data);
    si->albedo = color;
}

// Hitgroups -------------------------------------------------------------------------------

extern "C" __device__ void __closesthit__mesh()
{
    HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const auto* mesh_data = reinterpret_cast<TriangleMesh::Data*>(data->shape_data);

    SurfaceInteraction* si = getSurfaceInteraction();

    Ray ray = getWorldRay();

    const int prim_id = optixGetPrimitiveIndex();
    const Face face = mesh_data->faces[prim_id];
    const float u = optixGetTriangleBarycentrics().x;
    const float v = optixGetTriangleBarycentrics().y;

    const Vec2f texcoord0 = mesh_data->texcoords[face.texcoord_id.x()];
    const Vec2f texcoord1 = mesh_data->texcoords[face.texcoord_id.y()];
    const Vec2f texcoord2 = mesh_data->texcoords[face.texcoord_id.z()];
    const Vec2f texcoords = (1 - u - v) * texcoord0 + u * texcoord1 + v * texcoord2;

    Vec3f n0 = normalize(mesh_data->normals[face.normal_id.x()]);
    Vec3f n1 = normalize(mesh_data->normals[face.normal_id.y()]);
    Vec3f n2 = normalize(mesh_data->normals[face.normal_id.z()]);

    // Linear interpolation of normal by barycentric coordinates.
    Vec3f local_n = (1.0f - u - v) * n0 + u * n1 + v * n2;
    Vec3f world_n = optixTransformNormalFromObjectToWorldSpace(local_n);
    world_n = normalize(world_n);

    si->p = ray.at(ray.tmax);
    si->n = faceforward(world_n, -ray.d, world_n);
    si->uv = texcoords;
    si->surface_info = data->surface_info;
}

extern "C" __device__ void __closesthit__shadow_mesh() // for shadow rays
{
  // Hit to surface
  setPayload<0>(1);
}

// -------------------------------------------------------------------------------------------------
// Blocks

extern "C" __device__ void __intersection__block()
{
    
}

extern "C" __device__ void __closesthit__block()
{

}

// Diffuse
extern "C" __device__  Vec3f __continuation_callable__diffuse_shading(SurfaceInteraction * si, void* mat_data, const Vec3f & wi)
{
  const auto* diffuse = (Diffuse::Data*)mat_data;
  Vec3f albedo = optixDirectCall<Vec3f, SurfaceInteraction*, void*>(
    diffuse->texture.prg_id, si, diffuse->texture.data);
  si->albedo = albedo;

  Vec3f to_light = params.light.pos - si->p;
  const float t_shadow = length(to_light) - 1e-3f;
  const Vec3f light_dir = normalize(to_light);
  // Trace shadow ray
  const bool hit_object = shadowTrace(
    params.handle, si->p, light_dir, 1e-3f, t_shadow, 2);

  Vec3f radiance;
  if (hit_object)
    radiance = 0.2f * si->albedo; //0.2f is the ambient term
  else
    radiance = 0.8f * fmaxf(0.0f, dot(light_dir, si->n)) * si->albedo + 0.2f * si->albedo;

  return radiance;
}

// Reflection
extern "C" __device__  Vec3f __direct_callable__reflection_direction(SurfaceInteraction * si, void* mat_data)
{
  const auto* conductor = (Conductor::Data*)mat_data;
  si->n = faceforward(si->n, -si->incoming, si->n); // Two sided
  return reflect(si->incoming, si->n);
}

extern "C" __device__  Vec3f __continuation_callable__reflection_shading(SurfaceInteraction * si, void* mat_data, const Vec3f & wi)
{
  const auto* conductor = (Conductor::Data*)mat_data;

  Vec3f albedo = optixDirectCall<Vec3f, SurfaceInteraction*, void*>(
    conductor->texture.prg_id, si, conductor->texture.data);

  si->albedo = albedo;
  return albedo;
}

// Refraction
extern "C" __device__  Vec3f __direct_callable__refraction_direction(SurfaceInteraction * si, void* mat_data)
{
  const auto* dielectric = (Dielectric::Data*)mat_data;

  float ni = 1.000292f; // air
  float nt = dielectric->ior;  // ior specified 
  float cosine = dot(si->incoming, si->n);
  bool into = cosine < 0;
  Vec3f outward_normal = into ? si->n : -si->n;

  if (!into) swap(ni, nt);

  cosine = fabs(cosine);
  float sine = sqrtf(1.0f - cosine * cosine);
  bool cannot_refract = ni * sine > nt;

  //float reflect_prob = fresnel(cosine, ni, nt);

  Vec3f outgoingDirection;
  if (cannot_refract)
    outgoingDirection = reflect(si->incoming, outward_normal);
  else
    outgoingDirection = refract(si->incoming, outward_normal, cosine, ni, nt);

  return outgoingDirection;
}

extern "C" __device__  Vec3f __continuation_callable__refraction_shading(SurfaceInteraction * si, void* mat_data, const Vec3f & wi)
{
  const auto* dielectric = (Dielectric::Data*)mat_data;
  Vec3f albedo = optixDirectCall<Vec3f, SurfaceInteraction*, void*>(
    dielectric->texture.prg_id, si, dielectric->texture.data);
  si->albedo = albedo; 
  // not taking into account Fresnel coefficient 
  //float reflect_prob = fresnel(cosine, ni, nt);
  return albedo;
}


static __forceinline__ __device__ Vec2f getUV(const Vec3f& p) {
    float phi = atan2(p.z(), p.x());
    float theta = asin(p.y());
    float u = 1.0f - (phi + math::pi) / (2.0f * math::pi);
    float v = 1.0f - (theta + math::pi / 2.0f) * math::inv_pi;
    return Vec2f(u, v);
}

extern "C" __device__ Vec3f __direct_callable__bitmap(SurfaceInteraction* si, void* tex_data) {
    const auto* image = reinterpret_cast<BitmapTexture::Data*>(tex_data);
    float4 c = tex2D<float4>(image->texture, si->uv.x(), si->uv.y());
    return Vec3f(c);
}

extern "C" __device__ Vec3f __direct_callable__constant(SurfaceInteraction* si, void* tex_data) {
    const auto* constant = reinterpret_cast<ConstantTexture::Data*>(tex_data);
    return constant->color;
}