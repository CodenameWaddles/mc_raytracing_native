#pragma once

#include <prayground/prayground.h>

#include "params.h"
// ImGui
#include <prayground/ext/imgui/imgui.h>
#include <prayground/ext/imgui/imgui_impl_glfw.h>
#include <prayground/ext/imgui/imgui_impl_opengl3.h>

using namespace std;

using RaygenRecord = Record<RaygenData>;
using HitgroupRecord = Record<HitgroupData>;
using MissRecord = Record<MissData>;
using EmptyRecord = Record<EmptyData>;

using SBT = ShaderBindingTable<RaygenRecord, MissRecord, HitgroupRecord, EmptyRecord, EmptyRecord, (uint32_t)RayType::N_RAY>;

class App : public BaseApp
{
public:
    App() {}
    void setup();
    void update();
    void draw();
    void close();
    Vec3f rotateByQuaternion(Vec3f& v, Vec4f& r);

    //void mousePressed(float x, float y, int button);
    //void mouseDragged(float x, float y, int button);
    //void mouseReleased(float x, float y, int button);
    //void mouseMoved(float x, float y);
    //void mouseScrolled(float xoffset, float yoffset);

    //void keyPressed(int key);
    //void keyReleased(int key);
private:

    enum class ObjectType : int {
        eMesh = 0,
        eBlock = 1,
    };
    struct Object {
        ObjectType objectType;
        Vec3f position, scale;
        Vec4f rotation;
        std::string objectFileName;
    };

    void initResultBufferOnDevice();
    void handleCameraUpdate();
    void initData(std::vector<Object> objects);
    void updateData();

    LaunchParams params;
    CUDABuffer<LaunchParams> d_params;
    Pipeline pipeline;
    Context context;
    CUstream stream;
    SBT sbt;
    InstanceAccel ias;

    Bitmap result_bitmap;

    Camera camera;
    bool camera_update;

    std::vector<std::shared_ptr<ShapeInstance>> _mesh;
    std::vector<float3> _mesh_pos;
    std::vector<float3> _mesh_scale;
};