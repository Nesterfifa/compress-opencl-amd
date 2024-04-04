Build:
```
cmake -S . -B build
cmake --build build
```
Run (decompresses and runs the kernel which multiplies a matrix by a vector):
```
./build/src/compress-ocl-src data/cl/test-gfx103/test1030.bin.xz
```
.xz decompression test:
```
cd build
ctest
```
