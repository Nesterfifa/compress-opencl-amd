#include <compress.h>
#include <fstream>
#include <vector>
#include <string>
#include <lzma.h>
#include <gtest/gtest.h>
#include <unify.h>

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

TEST(unify_600, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx601", "gfx602"};
    test_unify_deunify("../../data/cl/solve-gfx600", "kernel-gfx6*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_700, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx700", "gfx702", "gfx703", "gfx704", "gfx705"};
    test_unify_deunify("../../data/cl/solve-gfx700", "kernel-gfx7*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_800, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx801", "gfx802", "gfx803", "gfx805", "gfx810"};
    test_unify_deunify("../../data/cl/solve-gfx800", "kernel-gfx8*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_900, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx90c", "gfx900", "gfx902", "gfx904", "gfx906", "gfx909"};
    test_unify_deunify("../../data/cl/solve-gfx900", "kernel-gfx90*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_940, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx940", "gfx941", "gfx942"};
    test_unify_deunify("../../data/cl/solve-gfx900", "kernel-gfx94*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1010, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1010", "gfx1011", "gfx1012", "gfx1013"};
    test_unify_deunify("../../data/cl/solve-gfx101", "kernel-gfx101*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
}

TEST(unify_1030, validate_unify_deunify)
{
    std::vector<std::string> gpus = std::vector<std::string>{"gfx1030", "gfx1031", "gfx1032", "gfx1033", "gfx1034", "gfx1035", "gfx1036"};
    test_unify_deunify("../../data/cl/solve-gfx103", "kernel-gfx103*.bin", "compressed.bin.cpr", "decompressed.bin", gpus);
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
    encode_argv[2] = const_cast<char *>(folder.c_str());
    encode_argv[3] = const_cast<char *>(file_template.c_str());
    encode_argv[4] = const_cast<char *>(output.c_str());
    ASSERT_NO_THROW(unify(5, encode_argv));
    std::cout << 1 << endl;

    for (std::string gpu : gpus)
    {
        char **decode_argv = new char *[5];
        string input = compressed_path;
        output = decompressed_path;

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
