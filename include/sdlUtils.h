#ifndef SDLUTILS_H
#define SDLUTILS_H
#include "utils.h"
#include <CL/opencl.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


using uchar = unsigned char;

struct AppState {
    //raytracer
    int width;
    int height;


    std::vector<uchar> pixels;
    render renderScene;

    //SDL
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    AppState() : width(960), height(static_cast<int>(width* (9.0 / 16.0))), renderScene(width, height) {}

    //openCL

    int* hostSeeds;
    // OpenCL execution environment
    cl::Context context;

    // Represents a compute device(GPU, CPU, accelerator)
    cl::Device device;

    // Manages execution of commands on a device
    cl::CommandQueue queue;

    // Represents the actual computation to be executed
    cl::Kernel kernel;

    cl::Buffer cl_AccumBuffer;

    cl::Buffer cl_cameraBuffer;

    cl::Buffer cl_spheresBuffer;

    int numSpheres = 2;
    render::SphereInfo spheres[2];

    cl::Buffer cl_debugBuffer;

    cl::Buffer cl_seedsBuffer;

    cl::Buffer cl_output;
    
    int maxSamples = 96;
    int samplesPerThread = 16;


    bool moving = false;

    float widthCorrector = 1.7;
    float heightCorrector = 1.7;

    float windowCenterX = (width / 2.0) * widthCorrector;
    float windowCenterY = (height / 2.0) * heightCorrector;

    float lastVirtualX = windowCenterX;
    float lastVirtualY = windowCenterY;

    float currentVirtualX = windowCenterX;
    float currentVirtualY = windowCenterY;

    bool ignoringEvents = false;
    bool cameraNeedsUpdate = false;

    Uint8 data[1] = { 0 };
    Uint8 mask[1] = { 0 };
    SDL_Cursor* invisibleCursor;

};


void initBuffers(AppState* state) {

    //accumulating samples
    state->cl_AccumBuffer = cl::Buffer(state->context, CL_MEM_READ_WRITE, state->width * state->height * sizeof(cl_float3));

    //output to textures
    state->cl_output = cl::Buffer(state->context, CL_MEM_READ_WRITE, state->width * state->height * (sizeof(uchar)) * 3);

    //debug in host
    state->cl_debugBuffer = cl::Buffer(state->context, CL_MEM_WRITE_ONLY, 10 * sizeof(cl_float));
    
    //camera
    state->cl_cameraBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(state->renderScene.cameraInfo), &state->renderScene.cameraInfo);

    //spheres
    state->spheres[0] = state->renderScene.sphereOneInfo;
    state->spheres[1] = state->renderScene.sphereTwoInfo;
    state->cl_spheresBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2 * sizeof(render::SphereInfo), &state->spheres);

    //random number seed
    state->hostSeeds = (int*)malloc(state->width * state->height * sizeof(int));
    srand(time(NULL));
    for (int i = 0; i < state->width * state->height; i++) {
        int random_value = rand();
        state->hostSeeds[i] = 1 + (random_value % 2147483646);
    }
    state->cl_seedsBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, state->width * state->height * sizeof(int), state->hostSeeds);


}
















//Debug console prints
std::string read_kernel_file(const std::string& filename) {
    std::ifstream file(filename);
    try {
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open kernel file: " + filename);
        }
    }
    catch (const std::exception& e) {
        std::exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printDeviceInfo(const cl::Device& device) {


    std::string name, vendor, version, profile;
    cl_uint compute_units;
    cl_ulong global_mem, local_mem;
    size_t max_workgroup;
    cl_bool available;
    cl_device_type type;

    device.getInfo(CL_DEVICE_NAME, &name);
    device.getInfo(CL_DEVICE_TYPE, &type);
    device.getInfo(CL_DEVICE_VENDOR, &vendor);
    device.getInfo(CL_DEVICE_VERSION, &version);
    device.getInfo(CL_DEVICE_PROFILE, &profile);
    device.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &compute_units);
    device.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &global_mem);
    device.getInfo(CL_DEVICE_LOCAL_MEM_SIZE, &local_mem);
    device.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_workgroup);
    device.getInfo(CL_DEVICE_AVAILABLE, &available);

    std::cout << "Device: " << name << std::endl;
    std::cout << "Device type: " << (type == CL_DEVICE_TYPE_GPU ? "GPU" : "Other") << std::endl;
    std::cout << "Vendor: " << vendor << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "Profile: " << profile << std::endl;
    std::cout << "Compute Units: " << compute_units << std::endl;
    std::cout << "Global Memory: " << global_mem / (1024 * 1024) << " MB" << std::endl;
    std::cout << "Local Memory: " << local_mem / 1024 << " KB" << std::endl;
    std::cout << "Max Workgroup: " << max_workgroup << std::endl;
    std::cout << "Available: " << (available ? "Yes" : "No") << std::endl;
}

static inline void printPlatform(cl::Platform& platform, std::string& name, std::string& vendor, std::string& version) {

    platform.getInfo(CL_PLATFORM_NAME, &name);
    platform.getInfo(CL_PLATFORM_VENDOR, &vendor);
    platform.getInfo(CL_PLATFORM_VERSION, &version);
    std::cout << "===============================================\n";
    std::cout << "Platform: " << name << "\n";
    std::cout << "Vendor: " << vendor << "\n";
    std::cout << "Version: " << version << "\n";
    std::cout << "===============================================\n";

    std::cout << "\n        contained devices: \n";
    std::cout << "-----------------------------------------------------\n";
    std::vector<cl::Device> currentDevices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &currentDevices);
    for (const auto& device : currentDevices) {
        printDeviceInfo(device);
    }
    std::cout << "-----------------------------------------------------\n";


}
bool debugRandomNumbers(AppState* state) {

    static bool initialized = [state]() {

        float* debugArr = (float*)malloc(10 * sizeof(cl_float));
        state->queue.enqueueReadBuffer(state->cl_debugBuffer, CL_TRUE, 0, 10 * sizeof(cl_float), debugArr);
        std::cout << "debug array: ";
        for (int i = 0; i < 10; i++) {
            std::cout << debugArr[i] << " ";
        }
        std::cout << "\n";

        free(debugArr);
        return true;
        }();

    return initialized;

}

#endif 

