cd..
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/OpenCL/OpenCL-SDK/install"
cmake --build . --config Release
cd Release
RayTracer.exe
cd ..
cd ..
cd customBuildExecutables