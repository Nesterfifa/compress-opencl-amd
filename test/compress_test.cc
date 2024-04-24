#include <compress.h>
#include <fstream>
#include <vector>
#include <string>
#include <lzma.h>
#include <gtest/gtest.h>
#include <unify.h>
#include <CL/opencl.hpp>

std::vector<unsigned char> random_array(size_t size);
void test_unify_deunify(
    std::string folder,
    std::string file_template,
    std::string compressed_path,
    std::string decompressed_path,
    const std::vector<std::string> &gpus);

TEST(compression_test, validate_compress_decompress)
{
    const int num_tests = 5;
    const size_t max_array_size = 10000;

    srand(static_cast<unsigned int>(time(NULL)));

    for (int i = 0; i < num_tests; ++i)
    {
        size_t array_size = rand() % max_array_size + 1;
        std::vector<unsigned char> original_data = random_array(array_size);

        std::vector<uint8_t> compressed_data;
        ASSERT_NO_THROW(compressed_data = compressData(original_data));

        std::vector<unsigned char> decompressed_data;
        ASSERT_NO_THROW(decompressed_data = decompressData(compressed_data));

        ASSERT_EQ(original_data.size(), decompressed_data.size());
        for (size_t i = 0; i < original_data.size(); ++i)
        {
            EXPECT_EQ(original_data[i], decompressed_data[i]);
        }
    }
}

void deleteRandomBytes(std::vector<unsigned char> &data, std::vector<unsigned char> &deleted_bytes,
                       std::vector<size_t> &deleted_positions)
{
    for (int i = 0; i < 2; ++i)
    {
        size_t pos = rand() % data.size();
        deleted_positions.push_back(pos);
        deleted_bytes.push_back(data[pos]);
        data.erase(data.begin() + pos);
    }
}

/* TEST(LZWCompressionTest, CompressDecompress)
{
    // Seed random number generator
    srand(static_cast<unsigned int>(time(NULL)));

    // Generate random binary array
    const size_t array_size = 10; // Adjust array size as needed
    std::vector<unsigned char> original_data = random_array(array_size);

    // Delete two random bytes from the array and store their values and positions
    std::vector<unsigned char> deleted_bytes;
    std::vector<size_t> deleted_positions;
    deleteRandomBytes(original_data, deleted_bytes, deleted_positions);

    // Compress the resulting array using compress_lzw
    std::vector<int> compressed_data;
    compress_lzw(original_data, compressed_data);

    std::ifstream kernel_file("../../src/decompress.cl");
    if (!kernel_file.good())
    {
        cerr << "Failed to open file " << endl;
        FAIL();
    }

    std::stringstream kernel_code_stream;
    kernel_code_stream << kernel_file.rdbuf();
    std::string kernel_code = kernel_code_stream.str();

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform platform = platforms[0]; // Use the first platform
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    cl::Device device = devices[0]; // Use the first device
    std::cout << kernel_code << std::endl;
    cl::Context context(device);
    cl::CommandQueue queue(context, device);
    cl::Program program(context, cl::Program::Sources(1, kernel_code));
    program.build({device});
    cl::Kernel decompress_lzw(program, "decompress_lzw");

    // Create buffers for OpenCL function
    cl::Buffer input_buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            sizeof(cl_int) * compressed_data.size(), compressed_data.data());
    cl::Buffer diff_buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           sizeof(cl_int) * deleted_positions.size(), deleted_positions.data());
    cl::Buffer gpu_bytes_buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                sizeof(cl_uchar) * deleted_bytes.size(), deleted_bytes.data());
    cl::Buffer output_buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uchar) * original_data.size());

    // Set kernel arguments
    decompress_lzw.setArg(0, input_buffer);
    decompress_lzw.setArg(1, compressed_data.size());
    decompress_lzw.setArg(2, diff_buffer);
    decompress_lzw.setArg(3, deleted_positions.size());
    decompress_lzw.setArg(4, gpu_bytes_buffer);
    decompress_lzw.setArg(5, output_buffer);

    // Enqueue kernel for execution
    queue.enqueueNDRangeKernel(decompress_lzw, cl::NullRange, cl::NDRange(1));

    // Read output buffer
    std::vector<unsigned char> decompressed_data(array_size);
    queue.enqueueReadBuffer(output_buffer, CL_TRUE, 0, sizeof(cl_uchar) * array_size, decompressed_data.data());

    for (size_t i = 0; i < deleted_positions.size(); i++)
    {
        original_data.insert(original_data.begin() + deleted_positions[i] + i, deleted_bytes[i]);
    }

    // Check that the result equals original array
    ASSERT_EQ(decompressed_data.size(), original_data.size());
    for (size_t i = 0; i < original_data.size(); ++i)
    {
        EXPECT_EQ(decompressed_data[i], original_data[i]);
    }
} */

TEST(unify_600, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx601", "gfx602"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx600", "kernel-gfx6*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_700, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx700", "gfx702", "gfx703", "gfx704", "gfx705"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx700", "kernel-gfx7*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_800, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx801", "gfx802", "gfx803", "gfx805", "gfx810"};
    test_unify_deunify("../../data/cl/solve-gfx800", "kernel-gfx8*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx800", "kernel-gfx8*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_900, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx90c", "gfx900", "gfx902", "gfx904", "gfx906", "gfx909"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx900", "kernel-gfx90*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_940, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx940", "gfx941", "gfx942"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx900", "kernel-gfx94*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1010, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1010", "gfx1011", "gfx1012", "gfx1013"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx101", "kernel-gfx101*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1030, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1030", "gfx1031", "gfx1032", "gfx1033", "gfx1034", "gfx1035", "gfx1036"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx103", "kernel-gfx103*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1100, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1100", "gfx1102"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx1100", "kernel-gfx110*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1102, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1101", "gfx1103"};
    test_unify_deunify("../../data/cl/OpenCL-examples/add_numbers/solve-gfx1101", "kernel-gfx110*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

std::vector<unsigned char> random_array(size_t size)
{
    std::vector<unsigned char> data(size);
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = rand() % 256; // Generate random byte (0-255)
    }
    return data;
}

void test_unify_deunify(
    std::string folder,
    std::string file_template,
    std::string compressed_path,
    std::string decompressed_path,
    const std::vector<std::string> &gpus)
{
    size_t argv = 5;
    char **encode_argv = new char *[5];
    std::string output = compressed_path;
    encode_argv[1] = "unify-xz";
    encode_argv[2] = const_cast<char *>(folder.c_str());
    encode_argv[3] = const_cast<char *>(file_template.c_str());
    encode_argv[4] = const_cast<char *>(output.c_str());
    ASSERT_NO_THROW(unify(5, encode_argv));

    for (std::string gpu : gpus)
    {
        char **decode_argv = new char *[5];
        string input = compressed_path;
        output = decompressed_path;

        decode_argv[1] = "deunify-xz";
        decode_argv[2] = const_cast<char *>(input.c_str());
        decode_argv[3] = const_cast<char *>(output.c_str());
        decode_argv[4] = const_cast<char *>(gpu.c_str());

        ASSERT_NO_THROW(deunify(5, decode_argv));

        std::ifstream original(folder + "/kernel-" + gpu + ".bin", std::ios::binary);
        std::ifstream deunified(output, std::ios::binary);
        std::vector<unsigned char> buffer_original((istreambuf_iterator<char>(original)), (istreambuf_iterator<char>()));
        std::vector<unsigned char> buffer_deunified((istreambuf_iterator<char>(deunified)), (istreambuf_iterator<char>()));

        ASSERT_EQ(buffer_deunified.size(), buffer_original.size());
        for (int i = 0; i < buffer_deunified.size(); i++)
        {
            ASSERT_EQ(buffer_deunified[i], buffer_original[i]);
        }
    }

    fs::remove(decompressed_path);
    fs::remove(compressed_path);
}
