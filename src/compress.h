#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <lzma.h>

std::vector<unsigned char> decompressData(const std::string& compressed_data) {
    std::vector<unsigned char> decompressed_data;

    lzma_stream stream = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED);

    if (ret != LZMA_OK) {
        std::cerr << "Error initializing LZMA decoder: " << ret << std::endl;
        return decompressed_data;
    }

    stream.next_in = reinterpret_cast<const unsigned char*>(compressed_data.data());
    stream.avail_in = compressed_data.size();

    std::vector<unsigned char> out_buffer(1024 * 1024);
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