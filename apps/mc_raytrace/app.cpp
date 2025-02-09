#include "app.h"
#include <fstream>

void App::initResultBufferOnDevice()
{
    result_bitmap.allocateDevicePtr();

    params.result_buffer = reinterpret_cast<Vec4u*>(result_bitmap.deviceData());

    CUDA_SYNC_CHECK();
}

void App::handleCameraUpdate()
{
    if (!camera_update)
        return;
    camera_update = false;

    RaygenRecord* rg_record = reinterpret_cast<RaygenRecord*>(sbt.deviceRaygenRecordPtr());
    RaygenData rg_data;
    rg_data.camera = camera.getData();

    CUDA_CHECK(cudaMemcpy(
        reinterpret_cast<void*>(&rg_record->data),
        &rg_data, sizeof(RaygenData),
        cudaMemcpyHostToDevice
    ));

    initResultBufferOnDevice();
}

void App::initData(std::vector<Object> objects)
{
    
}

void App::updateData()
{

}

// ------------------------------------------------------------------
void App::setup()
{
    // setup data 
    std::vector<Object> objects;

    // TODO setup data from shared buffers
    initData(objects);

    // OptixDeviceContext
    stream = 0;
    CUDA_CHECK(cudaFree(0));
    OPTIX_CHECK(optixInit());
    context.setDeviceId(0);
    context.create();

    ias = InstanceAccel{ InstanceAccel::Type::Instances };
    ias.allowUpdate();

    pipeline.setLaunchVariableName("params");
    pipeline.setDirectCallableDepth(5);
    pipeline.setContinuationCallableDepth(5);
    pipeline.setNumPayloads(5);
    pipeline.setNumAttributes(5);

    // OptiX Module CUDA
    Module module = pipeline.createModuleFromCudaFile(context, "kernels.cu");

    // Bitmap
    result_bitmap.allocate(PixelFormat::RGBA, pgGetWidth(), pgGetHeight());

    // LaunchParams
    params.width = result_bitmap.width();
    params.height = result_bitmap.height();
    params.max_depth = 5;
    camera.setAspect(static_cast<float>(params.width) / params.height);

    initResultBufferOnDevice();

    // Raygen
    ProgramGroup raygen_prg = pipeline.createRaygenProgram(context, module, "__raygen__pinhole");
    RaygenRecord raygen_record;
    raygen_prg.recordPackHeader(&raygen_record);
    raygen_record.data.camera = camera.getData();
    sbt.setRaygenRecord(raygen_record);

    // Shader Binding Table Callable Lambda
    auto setupCallable = [&](const std::string& dc, const std::string& cc)
        -> uint32_t
    {
        EmptyRecord callable_record = {};
        auto [prg, id] = pipeline.createCallablesProgram(context, module, dc, cc);
        prg.recordPackHeader(&callable_record);
        sbt.addCallablesRecord(callable_record);
        return id;
    };

    // Callable
    uint32_t constant_prg_id = setupCallable("__direct_callable__constant", "");
    uint32_t bitmap_prg_id = setupCallable("__direct_callable__bitmap", "");

    // shading methods
    // Diffuse
    uint32_t diffuse_shading_prg_id = setupCallable("", "__continuation_callable__diffuse_shading");
    // Reflection
    uint32_t reflection_prg_id = setupCallable("__direct_callable__reflection_direction", "__continuation_callable__reflection_shading");
    // Refraction
    uint32_t refraction_prg_id = setupCallable("__direct_callable__refraction_direction", "__continuation_callable__refraction_shading");

    SurfaceCallableID diffuse_id{ diffuse_shading_prg_id, diffuse_shading_prg_id, 0 };
    SurfaceCallableID reflection_id{ reflection_prg_id, reflection_prg_id, 0 };
    SurfaceCallableID refraction_id{ refraction_prg_id, refraction_prg_id, 0 };

    // cst color background
    auto env_color = make_shared<ConstantTexture>(Vec3f(0.5f), constant_prg_id);
    env_color->copyToDevice();
    auto env = EnvironmentEmitter{ env_color };
    env.copyToDevice();

    // Miss
    ProgramGroup miss_prg = pipeline.createMissProgram(context, module, "__miss__envmap");
    MissRecord miss_record;
    miss_prg.recordPackHeader(&miss_record);
    miss_record.data.env_data = env.devicePtr();
    sbt.setMissRecord({ miss_record });

    struct Primitive
    {
        shared_ptr<ShapeInstance> instance;
        shared_ptr<Material> material;
    };

    uint32_t sbt_offset = 0;
    uint32_t sbt_idx = 0;
    uint32_t instance_id = 0;

    // ShapeInstance Shape ShapeInstance Lambda
    auto setupPrimitive = [&](ProgramGroup hitgroup_prg, ProgramGroup& shadow_prg, const Primitive& primitive)
    {
        auto shape = primitive.instance->shapes()[0];
        shape->setSbtIndex(sbt_idx);
        shape->copyToDevice();
        primitive.material->copyToDevice();

        HitgroupRecord record;
        hitgroup_prg.recordPackHeader(&record);
        HitgroupData record_data =
        {
            .shape_data = shape->devicePtr(),
            .surface_info =
              {
                  .data = primitive.material->devicePtr(),
                  .callable_id = primitive.material->surfaceCallableID(),
                  .type = primitive.material->surfaceType()
              },
        };
        record.data = record_data;

        HitgroupRecord shadow_record;
        shadow_prg.recordPackHeader(&shadow_record);
        shadow_record.data = record_data;

        sbt.addHitgroupRecord({ record, shadow_record });
        sbt_idx += SBT::NRay;

        primitive.instance->allowCompaction();
        primitive.instance->allowUpdate();
        primitive.instance->allowRandomVertexAccess();
        primitive.instance->setSBTOffset(sbt_offset);
        primitive.instance->setId(instance_id);
        primitive.instance->buildAccel(context, stream);

        ias.addInstance(*primitive.instance);

        instance_id++;
        sbt_offset += SBT::NRay;
    };

    auto mesh_prg = pipeline.createHitgroupProgram(context, module, "__closesthit__mesh");
    auto mesh_shadow_prg = pipeline.createHitgroupProgram(context, module, "__closesthit__shadow_mesh");
    auto block_prg = pipeline.createHitgroupProgram(context, module, "__closesthit__block", "__intersection__block");
    auto block_shadow_prg = pipeline.createHitgroupProgram(context, module, "__closesthit__block", "__intersection__block");

    cudaTextureDesc tex_desc = {};
    tex_desc.addressMode[0] = cudaAddressModeWrap;
    tex_desc.addressMode[1] = cudaAddressModeWrap;
    tex_desc.filterMode = cudaFilterModeLinear;
    tex_desc.normalizedCoords = 1;
    tex_desc.sRGB = 1;

    auto white = make_shared<ConstantTexture>(Vec3f(1.0f), constant_prg_id);
    Vec3f black(0.f, 0.f, 0.f);

    for (size_t i = 0; i < objects.size(); ++i)
    {
        if (objects[i].objectType == ObjectType::eMesh)
            // Mesh
        {
            Primitive primitive;

            // Geometry
            shared_ptr<TriangleMesh> triMesh(new TriangleMesh());
            vector<Attributes> material_attributes;

            triMesh->loadWithMtl(objects[i].objectFileName, material_attributes); // need .mtl material for all meshes !!
            _mesh_pos.push_back(objects[i].position);
            _mesh_scale.push_back(objects[i].scale);
            primitive.instance = std::make_shared<ShapeInstance>(
                ShapeType::Mesh,
                triMesh,
                Matrix4f::translate(objects[i].position) * Matrix4f::scale(objects[i].scale) //TODO add rotations as well
                );
            _mesh.push_back(primitive.instance);

            // Material      
            if (material_attributes.empty())
            {
                // No mtl material provided
                // use default white diffuse material
                primitive.material = make_shared<Diffuse>(diffuse_id, white);
            }
            else
            {
                // Materials are either diffuse or reflecting or refracting (not a mix of these !)
                int n;
                // diffuse
                const Vec3f* diffuseColor = material_attributes[0].findVec3f("diffuse", &n);
                if (!(*diffuseColor == black))
                {
                    // get texture map if any
                    shared_ptr<Texture> diffuse_texture;
                    std::string diffuse_texname;
                    diffuse_texname = material_attributes[0].findOneString("diffuse_texture", "");
                    if (!diffuse_texname.empty())
                        diffuse_texture = make_shared<BitmapTexture>(diffuse_texname, tex_desc, bitmap_prg_id);
                    else
                        diffuse_texture = make_shared<ConstantTexture>(*diffuseColor, constant_prg_id);
                    primitive.material = make_shared<Diffuse>(diffuse_id, diffuse_texture);
                }
                else
                {
                    //specular
                    const Vec3f* specularColor = material_attributes[0].findVec3f("specular", &n);
                    if (!(*specularColor == black))
                    {
                        shared_ptr<Texture> specular_texture = make_shared<ConstantTexture>(*specularColor, constant_prg_id);
                        primitive.material = make_shared<Conductor>(reflection_id, specular_texture);
                    }
                    else
                    {
                        // refractive
                        const Vec3f* transmittanceColor = material_attributes[0].findVec3f("transmittance", &n);
                        if (!(*transmittanceColor == black))
                        {
                            shared_ptr<Texture> transmittance_texture = make_shared<ConstantTexture>(*transmittanceColor, constant_prg_id);
                            // get ior also
                            const float* ior = material_attributes[0].findFloat("ior", &n);
                            if (ior != nullptr)
                            {
                                primitive.material = make_shared<Dielectric>(refraction_id, transmittance_texture, *ior);
                            }
                            else
                            {
                                primitive.material = make_shared<Dielectric>(refraction_id, transmittance_texture, 1.0f);
                            }
                        }
                        else
                        {
                            // dafault material
                            primitive.material = make_shared<Diffuse>(diffuse_id, white);
                        }
                    }
                }
            }

            setupPrimitive(mesh_prg, mesh_shadow_prg, primitive);
        }

        // eBlock type, temporary ?
        else if (objects[i].objectType == ObjectType::eBlock)
        {
            Primitive primitive;

            // Geometry
            shared_ptr<Box> block(new Box());
            vector<Attributes> material_attributes;

            _mesh_pos.push_back(objects[i].position);
            _mesh_scale.push_back(objects[i].scale);
            primitive.instance = std::make_shared<ShapeInstance>(
                ShapeType::Mesh,
                block,
                Matrix4f::translate(objects[i].position) * Matrix4f::scale(objects[i].scale) //TODO add rotations as well
                );
            _mesh.push_back(primitive.instance);
    
            // dafault material
            primitive.material = make_shared<Diffuse>(diffuse_id, white);

            setupPrimitive(block_prg, block_shadow_prg, primitive);
        }
    }

    // Shader Binding Table 
    sbt.createOnDevice();
    // TraversableHandle
    ias.build(context, stream);
    params.handle = ias.handle();
    pipeline.create(context);
    // cudaStream
    CUDA_CHECK(cudaStreamCreate(&stream));
    d_params.allocate(sizeof(LaunchParams));

    // GUI setting
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    const char* glsl_version = "#version 150";
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(pgGetCurrentWindow()->windowPtr(), true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

// ------------------------------------------------------------------
void App::update()
{
    updateData();

    handleCameraUpdate();

    float time = pgGetElapsedTimef();

    for (size_t i = 1; i < _mesh.size(); ++i) // Hypothesis mesh 0 = fixed background / can be replaced by gaussian splats
    {
        //_mesh[i]->setTransform(Matrix4f::translate(_mesh_pos[i] + 2.0f * Vec3f(cosf(time), 0.f, sinf(time))) * Matrix4f::rotate(time, { 0.f, 1.0f, 0.0f }) * Matrix4f::scale(_mesh_scale[i]));
    }

    ias.update(context, stream);

    d_params.copyToDeviceAsync(&params, sizeof(LaunchParams), stream);

    OPTIX_CHECK(optixLaunch(
        static_cast<OptixPipeline>(pipeline),
        stream,
        d_params.devicePtr(),
        sizeof(LaunchParams),
        &sbt.sbt(),
        params.width,
        params.height,
        1
    ));

    CUDA_CHECK(cudaStreamSynchronize(stream));
    CUDA_SYNC_CHECK();

    result_bitmap.copyFromDevice();
}

// ------------------------------------------------------------------
void App::draw()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Minecraft Raytracing GUI");

    ImGui::Text("Camera info:");
    ImGui::Text("Camera Origin: %f %f %f", camera.origin().x(), camera.origin().y(), camera.origin().z());
    ImGui::Text("Camera Lookat: %f %f %f", camera.lookat().x(), camera.lookat().y(), camera.lookat().z());
    ImGui::Text("Camera Up: %f %f %f", camera.up().x(), camera.up().y(), camera.up().z());

    ImGui::Text("Frame rate: %.3f ms/frame (%.2f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::End();
    ImGui::Render();

    const int32_t w = pgGetWidth();
    const int32_t h = pgGetHeight();

    result_bitmap.draw(0, 0, w, h);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::close()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    pipeline.destroy();
    context.destroy();
}

Vec3f App::rotateByQuaternion(Vec3f& v, Vec4f& r)
{
    Vec4f u(0, v.x(), v.y(), v.z());
    u *= r;
    return Vec3f(u[1], u[2], u[3]);
}


//// ------------------------------------------------------------------
//void App::mousePressed(float x, float y, int button)
//{
//    
//}
//
//// ------------------------------------------------------------------
//void App::mouseReleased(float x, float y, int button)
//{
//    
//}
//
//// ------------------------------------------------------------------
//void App::mouseMoved(float x, float y)
//{
//    
//}
//
//// ------------------------------------------------------------------
//void App::keyPressed(int key)
//{
//
//}
//
//// ------------------------------------------------------------------
//void App::keyReleased(int key)
//{
//
//}



