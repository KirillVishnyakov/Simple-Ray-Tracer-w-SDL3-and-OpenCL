cd ..
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/OpenCL/OpenCL-SDK/install"
cmake --build . --config Debug
cd Debug
RayTracer.exe
cd ..
cd ..
cd customBuildExecutables