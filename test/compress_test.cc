#include <compress.h>
#include <fstream>
#include <vector>
#include <string>
#include <lzma.h>
#include <gtest/gtest.h>

TEST(DecompressTest, BasicAssertions) {
  // Read original binary file
    std::ifstream original_file("../../data/cl/test-gfx103/test1030.bin", std::ios::binary);
    ASSERT_TRUE(original_file.is_open());

    std::vector<uint8_t> original_data((std::istreambuf_iterator<char>(original_file)), std::istreambuf_iterator<char>());
    original_file.close();

    // Read compressed binary file
    std::ifstream compressed_file("../../data/cl/test-gfx103/test1030.bin.xz", std::ios::binary);
    ASSERT_TRUE(compressed_file.is_open());

    std::vector<uint8_t> compressed_data((std::istreambuf_iterator<char>(compressed_file)), std::istreambuf_iterator<char>());
    compressed_file.close();

    // Decompress the compressed data
    std::vector<uint8_t> decompressed_data;
    ASSERT_NO_THROW(decompressed_data = decompressData(std::string(compressed_data.begin(), compressed_data.end())));

    // Assert that decompressed data matches the original data
    ASSERT_EQ(original_data.size(), decompressed_data.size());
    for (size_t i = 0; i < original_data.size(); ++i) {
        EXPECT_EQ(original_data[i], decompressed_data[i]);
    }
}