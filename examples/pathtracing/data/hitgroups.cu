#include "util.cuh"
#include <prayground/shape/plane.h>
#include <prayground/shape/trianglemesh.h>
#include <prayground/shape/sphere.h>
#include <prayground/shape/cylinder.h>
#include <prayground/core/ray.h>
#include <prayground/core/onb.h>
#include <prayground/core/bsdf.h>

using namespace prayground;

extern "C" __device__ void __closesthit__shadow()
{
    optixSetPayload_0(1);
}

// Plane -------------------------------------------------------------------------------
extern "C" __device__ void __intersection__plane()
{
    const HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const PlaneData* plane_data = reinterpret_cast<PlaneData*>(data->shape_data);

    const float2 min = plane_data->min;
    const float2 max = plane_data->max;

    Ray ray = getLocalRay();

    const float t = -ray.o.y / ray.d.y;

    const float x = ray.o.x + t * ray.d.x;
    const float z = ray.o.z + t * ray.d.z;

    float2 uv = make_float2(x / (max.x - min.x), z / (max.y - min.y));

    if (min.x < x && x < max.x && min.y < z && z < max.y && ray.tmin < t && t < ray.tmax)
        optixReportIntersection(t, 0, float2_as_ints(uv));
}

extern "C" __device__ void __closesthit__plane()
{
    HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());

    Ray ray = getWorldRay();

    float3 local_n = make_float3(0, 1, 0);
    float3 world_n = optixTransformNormalFromObjectToWorldSpace(local_n);
    world_n = normalize(world_n);
    float2 uv = make_float2(
        int_as_float( optixGetAttribute_0() ), 
        int_as_float( optixGetAttribute_1() )
    );

    SurfaceInteraction* si = getSurfaceInteraction();

    si->p = ray.at(ray.tmax);
    si->t = ray.tmax;
    si->n = world_n;
    si->wi = ray.d;
    si->uv = uv;
    si->surface_info = data->surface_info;
}

extern "C" __device__ float __direct_callable__pdf_plane(AreaEmitterInfo area_info, SurfaceInteraction* si, const float3& to_light)
{
    /*const PlaneData* plane_data = reinterpret_cast<PlaneData*>(area_info.shape_data);
    const float area = (plane_data->max.x - plane_data->min.x) * (plane_data->max.y - plane_data->min.y);
    const float distance_squared = math::sqr(length(to_light));
    const float cosine = dot(si->n, to_light);
    if (cosine < math::eps)
        return 0.0f;
    return distance_squared / cosine * area;*/
    return 0.0f;
}

extern "C" __device__ float3 __direct_callable__rnd_sample_plane(AreaEmitterInfo area_info, SurfaceInteraction* si)
{
    /*const PlaneData* plane_data = reinterpret_cast<PlaneData*>(area_info.shape_data);
    const float3 local_ro = area_info.worldToObj.pointMul(si->p);
    unsigned int seed = si->seed;
    const float3 rnd_p = make_float3(rnd(seed, plane_data->min.x, plane_data->max.x), 0.0f, rnd(seed, plane_data->min.y, plane_data->max.y));
    float3 to_light = area_info.objToWorld.vectorMul(rnd_p - local_ro);
    si->wo = normalize(to_light);
    si->seed = seed;
    return to_light;*/
    return make_float3(0.0f);
}

// Sphere -------------------------------------------------------------------------------
extern "C" __device__ void __intersection__sphere() {
    const HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const SphereData* sphere_data = reinterpret_cast<SphereData*>(data->shape_data);

    const float3 center = sphere_data->center;
    const float radius = sphere_data->radius;

    Ray ray = getLocalRay();

    const float3 oc = ray.o - center;
    const float a = dot(ray.d, ray.d);
    const float half_b = dot(oc, ray.d);
    const float c = dot(oc, oc) - radius * radius;
    const float discriminant = half_b * half_b - a * c;

    if (discriminant > 0.0f) {
        float sqrtd = sqrtf(discriminant);
        float t1 = (-half_b - sqrtd) / a;
        bool check_second = true;
        if (t1 > ray.tmin && t1 < ray.tmax) {
            float3 normal = normalize((ray.at(t1) - center) / radius);
            check_second = false;
            optixReportIntersection(t1, 0, float3_as_ints(normal));
        }

        if (check_second) {
            float t2 = (-half_b + sqrtd) / a;
            if (t2 > ray.tmin && t2 < ray.tmax) {
                float3 normal = normalize((ray.at(t2) - center) / radius);
                optixReportIntersection(t2, 0, float3_as_ints(normal));
            }
        }
    }
}

static __forceinline__ __device__ float2 getSphereUV(const float3& p) {
    float phi = atan2(p.z, p.x);
    float theta = asin(p.y);
    float u = 1.0f - (phi + math::pi) / (2.0f * math::pi);
    float v = 1.0f - (theta + math::pi / 2.0f) / math::pi;
    return make_float2(u, v);
}

extern "C" __device__ void __closesthit__sphere() {
    const HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const SphereData* sphere_data = reinterpret_cast<SphereData*>(data->shape_data);

    Ray ray = getWorldRay();

    float3 local_n = make_float3(
        int_as_float(optixGetAttribute_0()),
        int_as_float(optixGetAttribute_1()),
        int_as_float(optixGetAttribute_2())
    );
    float3 world_n = optixTransformNormalFromObjectToWorldSpace(local_n);
    world_n = normalize(world_n);

    SurfaceInteraction* si = getSurfaceInteraction();
    si->p = ray.at(ray.tmax);
    si->t = ray.tmax;
    si->n = world_n;
    si->wi = ray.d;
    si->uv = getSphereUV(local_n);
    si->surface_info = data->surface_info;
}

extern "C" __device__ float __direct_callable__pdf_sphere(AreaEmitterInfo area_info, SurfaceInteraction* si, const float3& /* to_light */)
{
    /*const SphereData* sphere_data = reinterpret_cast<SphereData*>(area_info.shape_data);
    const float3 center = sphere_data->center;
    const float radius = sphere_data->radius;
    const float3 local_ro = area_info.worldToObj.pointMul(si->p);
    const float cos_theta_max = sqrtf(1.0f - radius * radius / math::sqr(length(center - local_ro)));
    const float solid_angle = 2.0f * math::pi * (1.0f - cos_theta_max);
    return 1.0f / solid_angle;*/
    return 0.0f;
}

extern "C" __device__ float3 __direct_callable__rnd_sample_sphere(AreaEmitterInfo area_info, SurfaceInteraction* si)
{
    /*const SphereData* sphere_data = reinterpret_cast<SphereData*>(area_info.shape_data);
    const float3 center = sphere_data->center;
    const float3 local_ro = area_info.worldToObj.pointMul(si->p);
    const float3 oc = center - local_ro;
    float distance_squared = math::sqr(length(oc));
    Onb onb(oc);
    unsigned int seed = si->seed;
    float3 wo = randomSampleToSphere(seed, sphere_data->radius, distance_squared);
    onb.inverseTransform(wo);
    si->seed = seed;
    si->wo = normalize(wo);
    return wo;*/
    return make_float3(0.0f);
}

// Cylinder -------------------------------------------------------------------------------
static INLINE DEVICE float2 getCylinderUV(
    const float3& p, const float radius, const float height, const bool hit_disk
)
{
    if (hit_disk)
    {
        const float r = sqrtf(p.x*p.x + p.z*p.z) / radius;
        const float theta = atan2(p.z, p.x);
        float u = 1.0f - (theta + math::pi/2.0f) / math::pi;
        return make_float2(u, r);
    } 
    else
    {
        const float theta = atan2(p.z, p.x);
        const float v = (p.y + height / 2.0f) / height;
        float u = 1.0f - (theta + math::pi/2.0f) / math::pi;
        return make_float2(u, v);
    }
}

extern "C" __device__ void __intersection__cylinder()
{
    const HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const CylinderData* cylinder = reinterpret_cast<CylinderData*>(data->shape_data);

    const float radius = cylinder->radius;
    const float height = cylinder->height;

    Ray ray = getLocalRay();
    
    const float a = dot(ray.d, ray.d) - ray.d.y * ray.d.y;
    const float half_b = (ray.o.x * ray.d.x + ray.o.z * ray.d.z);
    const float c = dot(ray.o, ray.o) - ray.o.y * ray.o.y - radius*radius;
    const float discriminant = half_b*half_b - a*c;

    if (discriminant > 0.0f)
    {
        const float sqrtd = sqrtf(discriminant);
        const float side_t1 = (-half_b - sqrtd) / a;
        const float side_t2 = (-half_b + sqrtd) / a;

        const float side_tmin = fmin( side_t1, side_t2 );
        const float side_tmax = fmax( side_t1, side_t2 );

        if ( side_tmin > ray.tmax || side_tmax < ray.tmin )
            return;

        const float upper = height / 2.0f;
        const float lower = -height / 2.0f;
        const float y_tmin = fmin( (lower - ray.o.y) / ray.d.y, (upper - ray.o.y) / ray.d.y );
        const float y_tmax = fmax( (lower - ray.o.y) / ray.d.y, (upper - ray.o.y) / ray.d.y );

        float t1 = fmax(y_tmin, side_tmin);
        float t2 = fmin(y_tmax, side_tmax);
        if (t1 > t2 || (t2 < ray.tmin) || (t1 > ray.tmax))
            return;
        
        bool check_second = true;
        if (ray.tmin < t1 && t1 < ray.tmax)
        {
            float3 P = ray.at(t1);
            bool hit_disk = y_tmin > side_tmin;
            float3 normal = hit_disk 
                          ? normalize(P - make_float3(P.x, 0.0f, P.z))   // Hit at disk
                          : normalize(P - make_float3(0.0f, P.y, 0.0f)); // Hit at side
            float2 uv = getCylinderUV(P, radius, height, hit_disk);
            optixReportIntersection(t1, 0, float3_as_ints(normal), float2_as_ints(uv));
            check_second = false;
        }
        
        if (check_second)
        {
            if (ray.tmin < t2 && t2 < ray.tmax)
            {
                float3 P = ray.at(t2);
                bool hit_disk = y_tmax < side_tmax;
                float3 normal = hit_disk
                            ? normalize(P - make_float3(P.x, 0.0f, P.z))   // Hit at disk
                            : normalize(P - make_float3(0.0f, P.y, 0.0f)); // Hit at side
                float2 uv = getCylinderUV(P, radius, height, hit_disk);
                optixReportIntersection(t2, 0, float3_as_ints(normal), float2_as_ints(uv));
            }
        }
    }
}

extern "C" __device__ void __closesthit__cylinder()
{
    const HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const CylinderData* cylinder = reinterpret_cast<CylinderData*>(data->shape_data);

    Ray ray = getWorldRay();

    float3 local_n = make_float3(
        int_as_float( optixGetAttribute_0() ),
        int_as_float( optixGetAttribute_1() ), 
        int_as_float( optixGetAttribute_2() )
    );

    float2 uv = make_float2(
        int_as_float( optixGetAttribute_3() ),
        int_as_float( optixGetAttribute_4() )
    );

    float3 world_n = optixTransformNormalFromObjectToWorldSpace(local_n);

    SurfaceInteraction* si = getSurfaceInteraction();
    si->p = ray.at(ray.tmax);
    si->t = ray.tmax;
    si->n = normalize(world_n);
    si->wi = ray.d;
    si->uv = uv;
    si->surface_info = data->surface_info;
}

// Triangle mesh -------------------------------------------------------------------------------
extern "C" __device__ void __closesthit__mesh()
{
    HitgroupData* data = reinterpret_cast<HitgroupData*>(optixGetSbtDataPointer());
    const MeshData* mesh_data = reinterpret_cast<MeshData*>(data->shape_data);

    Ray ray = getWorldRay();
    
    const int prim_id = optixGetPrimitiveIndex();
    const Face face = mesh_data->faces[prim_id];
    const float u = optixGetTriangleBarycentrics().x;
    const float v = optixGetTriangleBarycentrics().y;

    const float2 texcoord0 = mesh_data->texcoords[face.texcoord_id.x];
    const float2 texcoord1 = mesh_data->texcoords[face.texcoord_id.y];
    const float2 texcoord2 = mesh_data->texcoords[face.texcoord_id.z];
    const float2 texcoords = (1-u-v)*texcoord0 + u*texcoord1 + v*texcoord2;

    float3 n0 = normalize(mesh_data->normals[face.normal_id.x]);
	float3 n1 = normalize(mesh_data->normals[face.normal_id.y]);
	float3 n2 = normalize(mesh_data->normals[face.normal_id.z]);

    // Linear interpolation of normal by barycentric coordinates.
    float3 local_n = (1.0f-u-v)*n0 + u*n1 + v*n2;
    float3 world_n = optixTransformNormalFromObjectToWorldSpace(local_n);
    world_n = normalize(world_n);

    SurfaceInteraction* si = getSurfaceInteraction();
    si->p = ray.at(ray.tmax);
    si->t = ray.tmax;
    si->n = world_n;
    si->wi = ray.d;
    si->uv = texcoords;
    si->surface_info = data->surface_info;
}