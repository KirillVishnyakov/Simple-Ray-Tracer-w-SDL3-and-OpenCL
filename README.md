# Simple Ray Tracer with SDL3 and OpenCL


A really simple path tracer implementation based on "Ray Tracing in a Weekend" built with SDL3 and OpenCL for GPU acceleration. This is a learning project focused on understanding the fundamentals of ray tracing and GPU computing.<br>
https://raytracing.github.io/books/RayTracingInOneWeekend.html

## Disclaimer

This is a learning project! It's not optimized whatsoever and may contain bugs, inefficiencies, or unexpected behavior. I've only tested this on systems with AMD processors/GPUs.


## Preview for now

https://github.com/user-attachments/assets/1e04a456-9205-49bf-8ddf-8b267fe4a761


## Features

* GPU-accelerated path tracing using OpenCL C

* Real-time rendering with SDL3

* Simple Cornell box scene with spheres

* Progressive rendering with accumulating samples

## Prerequisites
CMake (version 3.15 or higher)

C++17 compatible compiler

* OpenCL SDK (AMD, NVIDIA, or Intel)

* SDL3 library

* GPU with OpenCL support

## Building and Running

```
git clone https://github.com/KirillVishnyakov/Simple-Ray-Tracer-w-SDL3-and-OpenCL.git
cd Simple-Ray-Tracer-w-SDL3-and-OpenCL
mkdir build
cd build
```
#### If libraries are installed in standard locations (SDL3 and openCL)
`cmake ..`
#### If libraries are in custom locations, example:
`cmake .. -DCMAKE_PREFIX_PATH="C:/OpenCL/OpenCL-SDK/install;C:/SDL3"`
#### create exe for config or release
```
cmake --build . --config Release  
cmake --build . --config Debug
``` 

#### or cd Debug
```
cd Release 
./RayTracer.exe
```


