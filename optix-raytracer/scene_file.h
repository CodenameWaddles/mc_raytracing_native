#pragma once

#include "core/primitive.h"
#include "core/scene.h"

#include "shape/sphere.h"
#include "shape/trianglemesh.h"

#include "material/conductor.h"
#include "material/dielectric.h"
#include "material/diffuse.h"
#include "material/emitter.h"
#include "material/mmaps.h"

#include "texture/constant.h"
#include "texture/checker.h"
#include "texture/image.h"

/**
 * \note 
 * If a PrimitiveInstance store meshes and custom primitives (i.e. Sphere, Cylinder...), 
 * please sort primitives array to render geometries correctly as like follow code.
 * 
 * auto ps = oprt::PrimitiveInstance(oprt::Transform());
 * ps.set_sbt_index_base(0);
 * ps.add_primitive(sphere, white_lambert);
 * ps.add_primitive(mesh, metal);
 * ps.sort();
 * scene.add_primitive_instance(ps);
 */

oprt::Scene my_scene() {
    oprt::Scene scene;

    // シーンの一般的な設定    
    scene.set_bgcolor(make_float4(0.0f));
    scene.set_width(768);
    scene.set_height(768);
    scene.set_depth(5);
    scene.set_samples_per_launch(1);
    scene.set_num_samples(4096);

    // テクスチャの準備
    auto checker1 = new oprt::CheckerTexture(
        make_float3(0.3f), make_float3(0.9f), 10.0f
    );
    auto earth_image = new oprt::ImageTexture("../../data/image/earth.jpg");

    // マテリアルの準備 
    auto red_diffuse = new oprt::Diffuse(make_float3(0.8f, 0.05f, 0.05f));
    auto green_diffuse = new oprt::Diffuse(make_float3(0.05f, 0.8f, 0.05f));
    auto white_diffuse = new oprt::Diffuse(make_float3(0.8f, 0.8f, 0.8f));
    auto emitter = new oprt::Emitter(make_float3(0.8f, 0.8f, 0.7f), 25.0f);
    auto metal = new oprt::Conductor(make_float3(0.8f, 0.8f, 0.2f), 0.01f);
    auto glass = new oprt::Dielectric(make_float3(0.9f), 1.5f);
    auto floor_checker = new oprt::Diffuse(checker1);
    auto earth_diffuse = new oprt::Diffuse(earth_image);
    auto teapot_diffuse = new oprt::Diffuse(make_float3(1.0f, 0.8f, 0.3f));
    auto mmaps = new oprt::MMAPs(make_float3(1.0f));

    // コーネルボックスの中心位置
    float3 cornel_center = make_float3(278.0f, 274.4f, 279.6f);

    // コーネルボックス用のプリミティブインスタンス
    oprt::PrimitiveInstance cornel_ps = oprt::PrimitiveInstance(oprt::Transform());
    cornel_ps.set_sbt_index_base(0);

    // Floor 
    auto floor_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 559.2f, 0.0f, oprt::Axis::Y);
    cornel_ps.add_primitive(floor_mesh, floor_checker);
    // Ceiling 
    auto ceiling_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 559.2f, 548.8f, oprt::Axis::Y);
    cornel_ps.add_primitive(ceiling_mesh, white_diffuse);
    // Back wall 
    auto back_wall_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 548.8f, 559.2f, oprt::Axis::Z);
    cornel_ps.add_primitive(back_wall_mesh, white_diffuse);
    // Right wall 
    auto right_wall_mesh = oprt::createQuadMesh(0.0f, 548.8f, 0.0f, 559.2f, 0.0f, oprt::Axis::X);
    cornel_ps.add_primitive(right_wall_mesh, red_diffuse);
    // Left wall 
    auto left_wall_mesh = oprt::createQuadMesh(0.0f, 548.8f, 0.0f, 559.2f, 556.0f, oprt::Axis::X);
    cornel_ps.add_primitive(left_wall_mesh, green_diffuse);
    // Ceiling light
    auto ceiling_light_mesh = oprt::createQuadMesh(213.0f, 343.0f, 227.0f, 332.0f, 548.6f, oprt::Axis::Y);
    cornel_ps.add_primitive(ceiling_light_mesh, emitter);
    scene.add_primitive_instance(cornel_ps);

    // Armadillo
    auto armadillo_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(150.0f, -210.0f, -130.0f)) 
                       * sutil::Matrix4x4::scale(make_float3(1.2f));
    auto armadillo_ps = oprt::PrimitiveInstance(armadillo_matrix);
    armadillo_ps.set_sbt_index_base(cornel_ps.sbt_index());
    auto armadillo = new oprt::TriangleMesh("../../model/Armadillo.ply");
    armadillo_ps.add_primitive(armadillo, metal);
    scene.add_primitive_instance(armadillo_ps);

    // Center bunny with lambert material
    auto bunny2_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(0.0f, -270.0f, 100.0f)) 
                       * sutil::Matrix4x4::rotate(M_PIf, make_float3(0.0f, 1.0f, 0.0f))
                       * sutil::Matrix4x4::scale(make_float3(1200.0f));
    auto bunny2_ps = oprt::PrimitiveInstance(bunny2_matrix);
    bunny2_ps.set_sbt_index_base(armadillo_ps.sbt_index());
    auto bunny2 = new oprt::TriangleMesh("../../model/bunny.obj");
    bunny2_ps.add_primitive(bunny2, white_diffuse);
    scene.add_primitive_instance(bunny2_ps);

    // Teapot
    auto teapot_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(-150.0f, -260.0f, -120.0f)) 
                    //    * sutil::Matrix4x4::rotate(M_PIf, make_float3(0.0f, 1.0f, 0.0f))
                       * sutil::Matrix4x4::scale(make_float3(40.0f));
    auto teapot_ps = oprt::PrimitiveInstance(teapot_matrix);
    teapot_ps.set_sbt_index_base(bunny2_ps.sbt_index());
    auto teapot = new oprt::TriangleMesh("../../model/teapot_normal_merged.obj");
    teapot_ps.add_primitive(teapot, teapot_diffuse);
    scene.add_primitive_instance(teapot_ps);

    auto sphere_ps = oprt::PrimitiveInstance(oprt::Transform());
    sphere_ps.set_sbt_index_base(teapot_ps.sbt_index());

    // Sphere 1
    auto sphere = new oprt::Sphere(cornel_center + make_float3(120.0f, 80.0f, 100.0), 90.0f);
    sphere_ps.add_primitive(sphere, earth_diffuse);

    // Sphere 2
    auto glass_sphere = new oprt::Sphere(cornel_center + make_float3(-150.0f, 0.0f, 80.0f), 80.0f);
    sphere_ps.add_primitive(glass_sphere, glass);
    scene.add_primitive_instance(sphere_ps);

    // MMAPs
    auto mmaps_matrix = sutil::Matrix4x4::translate(cornel_center)
                      * sutil::Matrix4x4::rotate(-M_PIf/4, make_float3(1.0f, 0.0f, 0.0f));
    auto mmaps_ps = oprt::PrimitiveInstance(mmaps_matrix);
    mmaps_ps.set_sbt_index_base(sphere_ps.sbt_index());
    auto mmaps_geom = oprt::createQuadMesh(-100, 100, -100, 100, 0, oprt::Axis::Y);
    mmaps_ps.add_primitive(mmaps_geom, mmaps);
    scene.add_primitive_instance(mmaps_ps);

    return scene;
}