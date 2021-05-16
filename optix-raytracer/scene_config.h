#pragma once

#include "core/primitive.h"
#include "core/scene.h"

#include "shape/sphere.h"
#include "shape/trianglemesh.h"

#include "material/conductor.h"
#include "material/dielectric.h"
#include "material/diffuse.h"
#include "material/disney.h"
#include "material/emitter.h"

#include "texture/constant.h"
#include "texture/checker.h"
#include "texture/image.h"

#include <filesystem>

/**
 * \note 
 * If a PrimitiveInstance store meshes and custom primitives (i.e. Sphere, Cylinder...), 
 * please sort primitives array to render geometries correctly as like follow code.
 * 
 * auto ps = oprt::PrimitiveInstance(oprt::Transform());
 * ps.set_sbt_index_base(0);
 * ps.addPrimitive(sphere, white_lambert);
 * ps.addPrimitive(mesh, metal);
 * ps.sort();
 * scene.addPrimitive_instance(ps);
 */

oprt::Scene my_scene() {
    oprt::Scene scene;

    // シーンの一般的な設定    
    scene.setEnvironment(make_float4(0.0f));
    scene.setWidth(1024);
    scene.setHeight(1024);
    scene.setDepth(5);
    scene.setSamplesPerLaunch(1);
    scene.setNumSamples(1000);

    // カメラの設定
    sutil::Camera camera;
    camera.setEye(make_float3(278.0f, 273.0f, -900.0f));
    camera.setLookat(make_float3(278.0f, 273.0f, 330.0f));
    camera.setUp(make_float3(0.0f, 1.0f, 0.0f));
    camera.setFovY(35.0f);
    scene.setCamera(camera);

    // テクスチャの準備
    auto checker1 = new oprt::CheckerTexture(
        make_float3(0.3f), make_float3(0.9f), 10.0f
    );
    auto earth_image = new oprt::ImageTexture("image/earth.jpg");
    auto skyblue_constant = new oprt::ConstantTexture(make_float3(0.8f, 0.05f, 0.05f));

    // マテリアルの準備 
    auto red_diffuse = new oprt::Diffuse(make_float3(0.8f, 0.05f, 0.05f));
    auto green_diffuse = new oprt::Diffuse(make_float3(0.05f, 0.8f, 0.05f));
    auto white_diffuse = new oprt::Diffuse(make_float3(0.8f, 0.8f, 0.8f));
    auto emitter = new oprt::Emitter(make_float3(0.8f, 0.8f, 0.7f), 15.0f);
    auto glass = new oprt::Dielectric(make_float3(0.9f), 1.5f);
    auto floor_checker = new oprt::Diffuse(checker1);
    auto earth_diffuse = new oprt::Diffuse(earth_image);
    auto disney = new oprt::Disney(skyblue_constant);
    disney->setMetallic(0.8f);
    disney->setRoughness(0.4f);
    auto teapot_diffuse = new oprt::Diffuse(make_float3(1.0f, 0.8f, 0.3f));

    // コーネルボックスの中心位置
    float3 cornel_center = make_float3(278.0f, 274.4f, 279.6f);

    // コーネルボックス用のプリミティブインスタンス
    oprt::PrimitiveInstance cornel_ps = oprt::PrimitiveInstance(oprt::Transform());
    cornel_ps.setSbtIndexBase(0);

    // Floor 
    auto floor_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 559.2f, 0.0f, oprt::Axis::Y);
    cornel_ps.addPrimitive(floor_mesh, floor_checker);
    // Ceiling 
    auto ceiling_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 559.2f, 548.8f, oprt::Axis::Y);
    cornel_ps.addPrimitive(ceiling_mesh, white_diffuse);
    // Back wall 
    auto back_wall_mesh = oprt::createQuadMesh(0.0f, 556.0f, 0.0f, 548.8f, 559.2f, oprt::Axis::Z);
    cornel_ps.addPrimitive(back_wall_mesh, white_diffuse);
    // Right wall 
    auto right_wall_mesh = oprt::createQuadMesh(0.0f, 548.8f, 0.0f, 559.2f, 0.0f, oprt::Axis::X);
    cornel_ps.addPrimitive(right_wall_mesh, red_diffuse);
    // Left wall 
    auto left_wall_mesh = oprt::createQuadMesh(0.0f, 548.8f, 0.0f, 559.2f, 556.0f, oprt::Axis::X);
    cornel_ps.addPrimitive(left_wall_mesh, green_diffuse);
    // Ceiling light
    auto ceiling_light_mesh = oprt::createQuadMesh(213.0f, 343.0f, 227.0f, 332.0f, 548.6f, oprt::Axis::Y);
    cornel_ps.addPrimitive(ceiling_light_mesh, emitter);
    scene.addPrimitiveInstance(cornel_ps);

    // Armadillo
    auto armadillo_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(150.0f, -210.0f, -130.0f)) 
                          * sutil::Matrix4x4::scale(make_float3(1.2f));
    auto armadillo_ps = oprt::PrimitiveInstance(armadillo_matrix);
    armadillo_ps.setSbtIndexBase(cornel_ps.sbtIndex());
    auto armadillo = new oprt::TriangleMesh("model/Armadillo.ply");
    auto metal = new oprt::Conductor(make_float3(0.8f, 0.8f, 0.2f), 0.01f);
    armadillo_ps.addPrimitive(armadillo, metal);
    scene.addPrimitiveInstance(armadillo_ps);

    // Center bunny with lambert material
    auto bunny2_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(0.0f, -270.0f, 100.0f)) 
                       * sutil::Matrix4x4::rotate(M_PIf, make_float3(0.0f, 1.0f, 0.0f))
                       * sutil::Matrix4x4::scale(make_float3(1200.0f));
    auto bunny2_ps = oprt::PrimitiveInstance(bunny2_matrix);
    bunny2_ps.setSbtIndexBase(armadillo_ps.sbtIndex());
    auto bunny2 = new oprt::TriangleMesh("model/bunny.obj");
    bunny2_ps.addPrimitive(bunny2, disney);
    scene.addPrimitiveInstance(bunny2_ps);

    // Teapot
    auto teapot_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(-150.0f, -260.0f, -120.0f)) 
                       * sutil::Matrix4x4::scale(make_float3(40.0f));
    auto teapot_ps = oprt::PrimitiveInstance(teapot_matrix);
    teapot_ps.setSbtIndexBase(bunny2_ps.sbtIndex());
    auto teapot = new oprt::TriangleMesh("model/teapot_normal_merged.obj");
    teapot_ps.addPrimitive(teapot, teapot_diffuse);
    scene.addPrimitiveInstance(teapot_ps);

    // Sphere 1
    auto earth_sphere_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(120.0f, 80.0f, 100.0f))
                             * sutil::Matrix4x4::rotate(M_PIf, make_float3(0.0f, 1.0f, 0.0f));
    auto earth_sphere_ps = oprt::PrimitiveInstance(earth_sphere_matrix);
    earth_sphere_ps.setSbtIndexBase(teapot_ps.sbtIndex());
    auto earth_sphere = new oprt::Sphere(make_float3(0.0f), 90.0f);
    earth_sphere_ps.addPrimitive(earth_sphere, earth_diffuse);
    scene.addPrimitiveInstance(earth_sphere_ps);

    // Sphere 2
    auto glass_sphere_matrix = sutil::Matrix4x4::translate(cornel_center + make_float3(-150.0f, 0.0f, 80.0f))
                             * sutil::Matrix4x4::rotate(M_PIf, make_float3(1.0f, 0.0f, 0.0f));
    auto glass_sphere_ps = oprt::PrimitiveInstance(glass_sphere_matrix);
    glass_sphere_ps.setSbtIndexBase(earth_sphere_ps.sbtIndex());
    auto glass_sphere = new oprt::Sphere(make_float3(0.0f), 80.0f);
    glass_sphere_ps.addPrimitive(glass_sphere, glass);
    scene.addPrimitiveInstance(glass_sphere_ps);

    return scene;
}