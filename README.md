Build:
```
cmake -S . -B build
cmake --build build
```
unify without compression:
```
./build/src/compress-ocl-src unify <directory> <regex> <output-binary>
```
unify with xz-compression:
```
./build/src/compress-ocl-src unify-xz <directory> <regex> <output-binary>
```
deunify (assumes unified array is not compressed):
```
 ./build/src/compress-ocl-src deunify <unified-binary> <output-binary> <gpuname>
```
deunify-xz (assumes unified array is xz-compressed):
```
 ./build/src/compress-ocl-src deunify-xz <unified-binary> <output-file> <gpuname>
```
.xz decompression test:
```
cd build
ctest
```
