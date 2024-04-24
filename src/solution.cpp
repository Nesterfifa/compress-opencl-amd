#include <compress.h>
#include <unify.h>
#include <sstream>
#include <fstream>
#include <CL/opencl.hpp>
#include <zlib.h>

int main(int argc, char *argv[])
{
    std::ifstream kernel_file("decompress.cl");
    if (!kernel_file.is_open())
    {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    
    std::stringstream kernel_code_stream;
    kernel_code_stream << kernel_file.rdbuf();

    try
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        cl::Platform platform = platforms[0];

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty())
        {
            platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        }

        cl::Device device = devices.front();

        cl::Context context(device);

        cl::CommandQueue queue(context, device);

        cl::Program::Sources sources;
        sources.push_back(kernel_code_stream.str());

        cl_int err;
        cl::Program program(context, sources);

        err = program.build();
        std::cout << err << std::endl;

        cl::Kernel kernel(program, "decompress_lzw", &err);

        cl_float A[16] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0};
        cl_float x[4] = {1.0, 2.0, 3.0, 4.0};
        cl_float y[4] = {0.0, 0.0, 0.0, 0.0};
        cl::Buffer buffer_A(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(A), A);
        cl::Buffer buffer_x(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(x), x);
        cl::Buffer buffer_y(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(y), y);
        kernel.setArg(0, buffer_A);
        kernel.setArg(1, buffer_x);
        kernel.setArg(2, 4);
        kernel.setArg(3, buffer_y);

        queue.enqueueNDRangeKernel(kernel, cl::NDRange(128 * 128), cl::NullRange, cl::NullRange);

        queue.finish();

        std::cout << "Kernel executed successfully!" << std::endl;
    }
    catch (const std::exception &err)
    {
        std::cerr << "OpenCL error: " << err.what() << std::endl;
        return 1;
    }

    return 0;
}
