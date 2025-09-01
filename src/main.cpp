
#define CL_HPP_ENABLE_EXCEPTIONS
#define SDL_MAIN_USE_CALLBACKS 1 
#include <CL/opencl.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <iostream>
#include "render.h"
#include <fstream>
#include <sstream>
#include <string>
#include "embedded_kernels.h" 

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

    AppState() : width(1280), height(static_cast<int>(width* (9.0 / 16.0))), renderScene(width, height) {}

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

    // Represents memory allocation on the device
    cl::Buffer cl_AccumBuffer;

    cl::Buffer cl_cameraBuffer;

    cl::Buffer cl_spheresBuffer;

    int numSpheres = 2;
    render::SphereInfo spheres[2];

    cl::Buffer cl_debugBuffer;

    cl::Buffer cl_seedsBuffer;

    cl::Buffer cl_output;

    int maxSamples = 128;
    int samplesPerThread = 16;

        
    
};

void printDeviceInfo(const cl::Device& device);
bool debugRandom(AppState* state);
SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv){
    
    auto* state = new AppState;
    

    // create a window
    state->window = SDL_CreateWindow("Ray Tracer", state->width * 1.3, state->height * 1.3, 0);
    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, state->width, state->height);
    state->pixels.resize(state->width * state->height * 3);

    // Initialize OpenCL
    try {
        // Get available platforms.
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        if (platforms.empty()) {
            std::cerr << "No OpenCL platforms found!" << std::endl;
            return SDL_APP_FAILURE;
        }

        std::cout << "Platforms: \n";
        std::string name, vendor, version;
        for (auto& platform : platforms) {
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
        
        
        // Get GPU device, I guess if you have multiple devices it selects the first one from the first platform.
        std::vector<cl::Device> devices;
        platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
        
        if (devices.empty()) {
            std::cerr << "No OpenCL devices found!" << std::endl;
            return SDL_APP_FAILURE;
        }

        state->device = devices[0];

        //context? manages resources for device(s)
        state->context = cl::Context(state->device);
        // A channel for sending commands to the device?
        // All kernel executions and memory transfers go through this queue
        state->queue = cl::CommandQueue(state->context, state->device);
        
        cl::Program program;
        
        //get the kernel file
        try {
            std::string ray_trace_kernel = Kernels::common_cl + "\n" + Kernels::ray_cl + "\n" + Kernels::render_cl;
            
            program = cl::Program(state->context, ray_trace_kernel, true);
            program.build();


        }
        catch (cl::Error& e) {
            std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(state->device);
            std::cout << "Build log:\n" << buildLog << std::endl;
            return SDL_APP_FAILURE;
        }
        
        // Create kernel
        state->kernel = cl::Kernel(program, "ray_trace");

        // Create buffers 
        
        //each color byte is of size (sizeof(uint8_t) * 3). RGB
        state->cl_AccumBuffer = cl::Buffer(state->context, CL_MEM_READ_WRITE, state->width * state->height * sizeof(cl_float3));
        state->cl_output = cl::Buffer(state->context, CL_MEM_READ_WRITE, state->width * state->height * (sizeof(uchar)) * 3);

        state->cl_debugBuffer = cl::Buffer(state->context, CL_MEM_WRITE_ONLY, 10 * sizeof(cl_float));


        state->cl_cameraBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(state->renderScene.cameraInfo), &state->renderScene.cameraInfo);

        state->spheres[0] = state->renderScene.sphereOneInfo;
        state->spheres[1] = state->renderScene.sphereTwoInfo;

        state->cl_spheresBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 2* sizeof(render::SphereInfo), &state->spheres);

        state->hostSeeds = (int*)malloc(state->width * state->height * sizeof(int));
        srand(time(NULL));
        for (int i = 0; i < state->width * state->height; i++) {
            int random_value = rand();
            state->hostSeeds[i] = 1 + (random_value % 2147483646);
        }

        state->cl_seedsBuffer = cl::Buffer(state->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, state->width * state->height * sizeof(int), state->hostSeeds);

        
        
            
        std::cout << "OpenCL initialized successfully!" << std::endl;

    }
    catch (const cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")" << std::endl;
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate){

    AppState* state = (AppState*)appstate;

    static Uint64 lastTime = 0;
    static Uint64 frameCount = 0;
    static float fps = 0.0f;
    

    Uint64 currentTime = SDL_GetPerformanceCounter();

    if (lastTime != 0) {
        float deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        fps = 1.0f / deltaTime;
    }
    lastTime = currentTime;

    try {
        cl::NDRange local(64, 4);
        cl::NDRange global_size(state->width, state->height);

        state->kernel.setArg(0, 0);
        state->kernel.setArg(1, state->cl_AccumBuffer);
        state->kernel.setArg(2, state->width);
        state->kernel.setArg(3, state->height);
        state->kernel.setArg(4, state->cl_cameraBuffer);
        state->kernel.setArg(5, state->cl_spheresBuffer);
        state->kernel.setArg(6, state->numSpheres);
        state->kernel.setArg(7, state->cl_seedsBuffer);
        state->kernel.setArg(8, state->cl_debugBuffer);
        state->kernel.setArg(9, state->cl_output);
        state->kernel.setArg(10, state->maxSamples);
        state->kernel.setArg(11, state->samplesPerThread);
        


        //first clear the outputBuffer
        state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);


        //now render the pixels
        state->kernel.setArg(0, 1);
        for (int sample = 0; sample < state->maxSamples/state->samplesPerThread; sample++) {
            state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);
        }


        //then convert accum -> output (convert to uchars for fast memcpy)
        state->kernel.setArg(0, 2);
        state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);


        //read from GPU
        state->queue.enqueueReadBuffer(state->cl_output, CL_TRUE, 0, state->width * state->height * sizeof(uchar) * 3, state->pixels.data());
        //apparently redundant, because of CL_TRUE
        state->queue.finish();


        void* texPixels;
        int pitch;
        if (SDL_LockTexture(state->texture, nullptr, &texPixels, &pitch) == 1) {

            uchar* src = state->pixels.data();
            uchar* dst = (uchar*)texPixels;
            for (int y = 0; y < state->height; y++) {
                memcpy(dst, src, state->width * 3);
                src += state->width * 3;
                dst += pitch;
            }
            SDL_UnlockTexture(state->texture);
        }

    }
    catch (const cl::Error& e) {
        std::cerr << "OpenCL runtime error: " << e.what() << " (code: " << e.err() << ")" << std::endl;
        std::cout << "Kernel expects " << state->kernel.getInfo<CL_KERNEL_NUM_ARGS>() << " arguments" << std::endl;
    }

    SDL_RenderClear(state->renderer);
    SDL_RenderTexture(state->renderer, state->texture, nullptr, nullptr);
    SDL_RenderPresent(state->renderer);

    frameCount++;
    if (frameCount % 60 == 0) {  // Update every 60 frames
        printf("FPS: %.2f\n", fps);
        // Or render to screen:
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.2f", fps);
        // Render this text to your SDL window
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){

    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    AppState* state = (AppState*)appstate;

    SDL_DestroyTexture(state->texture);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    free(state->hostSeeds);
    delete state;
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

bool debugRandom(AppState* state) {

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