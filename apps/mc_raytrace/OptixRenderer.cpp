#include <thread>
#include <atomic>
#include <iostream>
#include "app.h"
#include "com_example_OptixRenderer.h"
#include "dataManager.h"

// Global variables to manage the rendering thread
std::thread renderThread;
std::atomic<bool> isRunning(false);
DataManager dataManager;

extern "C" {

    JNIEXPORT void JNICALL Java_com_example_OptixRenderer_startRendering(JNIEnv* env, jobject obj) {
        // rendering code

        if (isRunning.load()) {
            std::cerr << "Rendering process is already running!" << std::endl;
            return;
        }

        isRunning.store(true);
        renderThread = std::thread([]() {
            std::cout << "C++ : Rendering thread started!" << std::endl;

            pgSetAppDir(APP_DIR);

            auto window = std::make_shared<Window>("MC Raytrace", 1024, 768);
            auto app = std::make_shared<App>();

            pgRunApp(app, window);

            while (isRunning.load()) {
                // Simulate rendering work
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                //std::cout << "C++ : Rendering stuff" << std::endl;
            }

            std::cout << "C++ : Rendering thread stopped!" << std::endl;
            });
    }

    JNIEXPORT void JNICALL Java_com_example_OptixRenderer_stopRendering(JNIEnv* env, jobject obj) {
        if (!isRunning.load()) {
            std::cerr << "Rendering process is not running!" << std::endl;
            return;
        }

        pgExit();

        // Clean up data

        isRunning.store(false);
        if (renderThread.joinable()) {
            renderThread.join(); // Wait for the thread to finish
        }
    }

    JNIEXPORT void JNICALL Java_com_example_OptixRenderer_renderFrame(JNIEnv* env, jobject obj) {
        std::cout << "C++ : Rendering frame" << std::endl;
    }

    JNIEXPORT jboolean JNICALL Java_com_example_OptixRenderer_isRendering(JNIEnv* env, jobject obj) {
        return isRunning.load() ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT void JNICALL Java_com_example_OptixRenderer_loadChunk(JNIEnv* env, jobject obj, jbyteArray chunkData, jint size) {
        jbyte* nativeJData = env->GetByteArrayElements(chunkData, NULL);
        uint8_t* nativeData = reinterpret_cast<uint8_t*>(nativeJData);
        
        dataManager.readChunk(nativeData, size);
    }

    JNIEXPORT void JNICALL Java_com_example_OptixRenderer_updateData(JNIEnv* env, jobject obj, jbyteArray updateData, jint size) {
        jbyte* nativeJData = env->GetByteArrayElements(updateData, NULL);
        uint8_t* nativeData = reinterpret_cast<uint8_t*>(nativeJData);

        dataManager.readData(nativeData, size);
    }
}

