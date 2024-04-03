#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <lzma.h>
#include <sstream>
#include <CL/opencl.hpp>

std::vector<uint8_t> decompressData(const std::string& compressed_data) {
    std::vector<uint8_t> decompressed_data;

    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED);

    if (ret != LZMA_OK) {
        std::cerr << "Error initializing LZMA decoder: " << ret << std::endl;
        return decompressed_data;
    }

    stream.next_in = reinterpret_cast<const uint8_t*>(compressed_data.data());
    stream.avail_in = compressed_data.size();

    std::vector<uint8_t> out_buffer(1024 * 1024);
    stream.next_out = out_buffer.data();
    stream.avail_out = out_buffer.size();

    ret = lzma_code(&stream, LZMA_FINISH);

    if (ret == LZMA_OK || ret == LZMA_STREAM_END) {
        decompressed_data.insert(decompressed_data.end(), out_buffer.begin(), out_buffer.begin() + stream.total_out);
    } else {
        std::cerr << "Error decompressing data: " << ret << std::endl;
    }

    lzma_end(&stream);

    return decompressed_data;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <compressed_kernel.cl.xz>" << "<kernel_name>" << std::endl;
        return 1;
    }

    std::ifstream kernel_file(argv[1], std::ios::binary);
    if (!kernel_file.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream compressed_kernel_stream;
    compressed_kernel_stream << kernel_file.rdbuf();
    std::string compressed_kernel = compressed_kernel_stream.str();

    std::vector<uint8_t> decompressed_kernel;
    try {
        decompressed_kernel = decompressData(compressed_kernel);
    } catch (const std::exception& e) {
        std::cerr << "Error decompressing xz-compressed kernel: " << e.what() << std::endl;
        return 1;
    }

    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        cl::Platform platform = platforms.front();

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty()) {
            platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        }

        cl::Device device = devices.front();

        cl::Context context(device);

        cl::CommandQueue queue(context, device);


        cl::Program::Binaries binaries;
        binaries.push_back(std::vector<uint8_t>(compressed_kernel.begin(), compressed_kernel.end()));
        cl_int err;
        cl::Program program(context, devices, binaries, NULL, &err);
        std::cout << err << std::endl;

        err = program.build();
        if (err != CL_SUCCESS) {
            size_t log_size;
            clGetProgramBuildInfo(program.get(), device.get(), CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

            char *log = (char *) malloc(log_size);

            clGetProgramBuildInfo(program.get(), device.get(), CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

            printf("%s\n", log);
        }
        
        cl::Kernel kernel(program, argv[2], &err);

        cl_float A[16] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0};
        cl_float x[4] = {1.0, 2.0, 3.0, 4.0};
        cl_float y[4] = {0.0, 0.0, 0.0, 0.0};
        cl::Buffer buffer_A(context, CL_MEM_READ_ONLY, sizeof(A), A);
        cl::Buffer buffer_x(context, CL_MEM_READ_ONLY, sizeof(x), x);
        cl::Buffer buffer_y(context, CL_MEM_WRITE_ONLY, sizeof(y), y);
        kernel.setArg(0, buffer_A);
        kernel.setArg(1, buffer_x);
        kernel.setArg(2, 4);
        kernel.setArg(3, buffer_y);

        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NullRange, cl::NullRange);

        queue.finish();

        std::cout << "Kernel executed successfully!" << std::endl;
    } catch (const std::exception& err) {
        std::cerr << "OpenCL error: " << err.what() << std::endl;
        return 1;
    }

    return 0;
}
