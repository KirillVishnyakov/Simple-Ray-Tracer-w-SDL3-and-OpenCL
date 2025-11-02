
#define CL_HPP_ENABLE_EXCEPTIONS
#define SDL_MAIN_USE_CALLBACKS 1 

#include <fstream>
#include <sstream>

#include <CL/opencl.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "render.h"
#include "embedded_kernels.h" 
#include "sdlUtils.h"


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    auto* state = new AppState;

    state->window = SDL_CreateWindow("Ray Tracer", state->width * state->widthCorrector, state->height * state->heightCorrector, 0);
    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, state->width, state->height);
    state->pixels.resize(state->width * state->height * 3);
    cl::Program program;
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) {
            std::cerr << "No OpenCL platforms found!\n";
            return SDL_APP_FAILURE;
        }

        std::vector<cl::Device> devices;
        platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty()) {
            std::cerr << "No OpenCL GPU devices found!\n";
            return SDL_APP_FAILURE;
        }

        state->device = devices[0];
        state->context = cl::Context({ state->device });
        state->queue = cl::CommandQueue(state->context, state->device);

        std::string ray_trace_kernel =
            Kernels::common_cl + "\n" + Kernels::ray_cl + "\n" + Kernels::render_cl;

        program = cl::Program(state->context, ray_trace_kernel);
        cl_int err = program.build({ state->device });
        if (err != CL_SUCCESS) {
            std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(state->device);
            std::cerr << "Build failed:\n" << buildLog << std::endl;
            return SDL_APP_FAILURE;
        }

        state->kernel = cl::Kernel(program, "ray_trace");

        initBuffers(state);
        std::cout << "OpenCL initialized successfully!\n";
    }
    catch (cl::Error& e) {
        if (e.err() == CL_BUILD_PROGRAM_FAILURE && program()) {
            std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(state->device);
            std::cerr << "Build log:\n" << log << std::endl;
        } else {
            std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")\n";
        }
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
        


        state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);


        state->kernel.setArg(0, 1);
        for (int sample = 0; sample < state->maxSamples/state->samplesPerThread; sample++) {
            state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);
        }
        if (state->cameraNeedsUpdate) {
            state->renderScene.buildCamStruct();
            state->queue.enqueueWriteBuffer(state->cl_cameraBuffer, CL_TRUE, 0, sizeof(state->renderScene.cameraInfo), &state->renderScene.cameraInfo);
            state->cameraNeedsUpdate = false;
        }
        state->kernel.setArg(0, 2);
        state->queue.enqueueNDRangeKernel(state->kernel, cl::NullRange, global_size, local);


        state->queue.enqueueReadBuffer(state->cl_output, CL_TRUE, 0, state->width * state->height * sizeof(uchar) * 3, state->pixels.data());
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
    if (frameCount % 60 == 0) {  
        //printf("FPS: %.2f\n", fps);
        char fpsText[32];
        //snprintf(fpsText, sizeof(fpsText), "FPS: %.2f", fps);
    }
    return SDL_APP_CONTINUE;
}


/*
SDL_CreateCursor(data, mask, 8, 1, 0, 0);
*/

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){
    AppState* state = (AppState*) appstate;


    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED){
        return SDL_APP_SUCCESS;
    }

    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        state->moving = !state->moving;
        if (state->moving) {
            state->invisibleCursor = SDL_CreateCursor(state->data, state->mask, 8, 1, 0, 0);
            SDL_SetCursor(state->invisibleCursor);

        }
        else {
            SDL_DestroyCursor(state->invisibleCursor);
            state->invisibleCursor = nullptr;
            
        }
          
        std::cout << "left click\n";
    }

    else if (event->type == SDL_EVENT_MOUSE_MOTION ) {
        
        if (state->moving && !state->ignoringEvents) {
            float deltaX = state->windowCenterX - event->motion.x;
            float deltaY = state->windowCenterY - event->motion.y;

            if (abs(deltaX) > 1.0f || abs(deltaY) > 1.0f) {


                vec3 updateDelta(-deltaX, deltaY, 0);
                state->renderScene.cam.updateCamera(updateDelta, state->heightCorrector);
                state->currentVirtualX += deltaX;
                state->currentVirtualY += deltaY;
                state->cameraNeedsUpdate = true;

                state->ignoringEvents = true;
                SDL_WarpMouseInWindow(state->window, state->windowCenterX, state->windowCenterY);
            }
            
        }
        else {
            state->ignoringEvents = false;
        }
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
