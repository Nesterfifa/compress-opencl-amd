#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstring>
#include <compress.h>

#define EXIT_USAGE -1

namespace fs = std::filesystem;
using namespace std;

unordered_map<string, unsigned char> gpu_bytes = {
    {"gfx601", 33},
    {"gfx602", 58},

    {"gfx700", 34},
    {"gfx702", 36},
    {"gfx703", 37},
    {"gfx704", 38},
    {"gfx705", 59},

    {"gfx801", 40},
    {"gfx802", 41},
    {"gfx803", 42},
    {"gfx805", 60},
    {"gfx810", 43},

    {"gfx900", 44},
    {"gfx902", 45},
    {"gfx904", 46},
    {"gfx906", 47},
    {"gfx909", 49},
    {"gfx90c", 50},

    {"gfx940", 64},
    {"gfx941", 75},
    {"gfx942", 76},

    {"gfx1010", 51},
    {"gfx1011", 52},
    {"gfx1012", 53},
    {"gfx1013", 66},

    {"gfx1030", 54},
    {"gfx1031", 55},
    {"gfx1032", 56},
    {"gfx1033", 57},
    {"gfx1034", 62},
    {"gfx1035", 61},
    {"gfx1036", 69},

    {"gfx1100", 65},
    {"gfx1102", 71},

    {"gfx1101", 70},
    {"gfx1103", 68}};

bool match(const wstring &wildcard, const wstring &fileName);
vector<uint8_t> int_le(int x);

int unify(int argc, char **argv)
{
    auto status = EXIT_SUCCESS;
    if (5 != argc)
    {
        return EXIT_USAGE;
    }

    const string folder(argv[2]);
    const string inputFileTemplate(argv[3]);
    const string outputFileName(argv[4]);

    const wstring wInputFileTemplate(inputFileTemplate.begin(), inputFileTemplate.end());

    ofstream output(outputFileName, ios::binary);

    if (!output.good())
    {
        cerr << "Could not write to file " << outputFileName << endl;
        return EXIT_FAILURE;
    }

    try
    {
        const fs::path folderPath(folder);
        vector<vector<unsigned char>> buffers;
        size_t fileNumber = 0;

        vector<fs::directory_entry> binFiles;
        for (auto &file : fs::directory_iterator(fs::absolute(folderPath)))
        {
            if (is_regular_file(file))
            {
                binFiles.push_back(file);
            }
        }
        std::sort(begin(binFiles), end(binFiles), [](const fs::directory_entry &left, const fs::directory_entry &right)
                  { return left.path().string() <= right.path().string(); });

        for (const auto &file : binFiles)
        {
            const fs::path &p(file);
            const wstring name(p.filename().wstring());
            if (match(wInputFileTemplate, name))
            {
                ifstream ifs(file.path(), ios::binary);
                if (!ifs.good())
                {
                    cerr << "Could not read file " << file.path() << endl;
                }
                vector<unsigned char> buffer((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));

                if (fileNumber > 0 && buffer.size() != buffers.back().size())
                {
                    cerr << "Sizes don't match in " << inputFileTemplate << " in " << folder << endl;
                }

                // save all buffers
                buffers.emplace_back(buffer);

                fileNumber++;
            }
        }

        if (0 == fileNumber)
        {
            std::cerr << "No matches found for " << inputFileTemplate << " in " << folder << std::endl;
            return EXIT_FAILURE;
        }

        vector<int> diff_pos;
        size_t size = buffers[0].size();
        for (size_t i = 0; i < size; i++)
        {
            for (size_t j = 1; j < fileNumber; j++)
            {
                if (buffers[j][i] != buffers[j - 1][i] && (diff_pos.empty() || diff_pos.back() != i))
                {
                    diff_pos.emplace_back(i);
                }
            }
        }

        size_t sz = diff_pos.size();
        if (sz > 100)
        {
            cerr << "The binaries are not similar enough :(" << endl;
            return EXIT_FAILURE;
        }

        vector<unsigned char> result;
        result.push_back(buffers.size()); // assuming we're not compressing more than 255 files
        result.push_back(sz);
        for (size_t i = 0; i < sz; i++)
        {
            vector<uint8_t> position = int_le(diff_pos[i]);
            result.insert(result.end(), position.begin(), position.end());
        }

        for (size_t i = 0; i < buffers.size(); i++)
        {
            for (size_t j = 0; j < sz; j++)
            {
                result.push_back(buffers[i][diff_pos[j]]);
            }
        }

        for (size_t i = 0; i < diff_pos.size(); i++)
        {
            buffers[0].erase(buffers[0].begin() + diff_pos[i] - i);
        }

        result.insert(result.end(), buffers[0].begin(), buffers[0].end());

        if (strcmp(argv[1], "unify-xz") == 0)
        {
            result = compressData(result);
        }

        for (char const c : result)
        {
            output << c;
        }
    }
    catch (const std::exception &e)
    {
        cerr << "An error occurred! Sorry ):" << e.what() << endl;
        status = EXIT_FAILURE;
    }

    return status;
}

vector<unsigned char> deunifyBinary(ifstream &input, string gpu_name, bool decompression)
{
    vector<unsigned char> buffer((istreambuf_iterator<char>(input)), (istreambuf_iterator<char>()));

    if (decompression)
    {
        buffer = decompressData(buffer);
    }

    uint8_t binaries_size = buffer[0];
    uint8_t diff_size = buffer[1];
    size_t pointer = 2;
    uint8_t first_byte = gpu_bytes[gpu_name];
    unordered_map<uint8_t, vector<uint8_t>> bytes_to_insert;
    vector<size_t> positions;
    for (size_t i = 0; i < diff_size; i++)
    {
        size_t position = 0;
        position |= buffer[pointer++];
        position |= static_cast<size_t>(buffer[pointer++]) << 8;
        position |= static_cast<size_t>(buffer[pointer++]) << 16;
        position |= static_cast<size_t>(buffer[pointer++]) << 24;
        positions.push_back(position);
    }

    for (size_t i = 0; i < binaries_size; i++)
    {
        uint8_t first = buffer[pointer++];
        for (size_t j = 1; j < diff_size; j++)
        {
            bytes_to_insert[first].push_back(buffer[pointer++]);
        }
    }

    vector<uint8_t> result = vector<uint8_t>(buffer.begin() + pointer, buffer.end());
    result.insert(result.begin() + positions[0], first_byte);
    for (int i = 1; i < diff_size; i++)
    {
        result.insert(result.begin() + positions[i], bytes_to_insert[first_byte][i - 1]);
    }

    return result;
}

int deunify(int argc, char **argv)
{
    auto status = EXIT_SUCCESS;
    if (5 != argc)
    {
        return EXIT_USAGE;
    }

    const string input_file_name(argv[2]);
    const string output_file_name(argv[3]);
    const string gpu_name(argv[4]);

    if (!gpu_bytes.count(gpu_name))
    {
        printf("Sorry, this GPU is not supported yet!\nSupported GPUs:\n");
        for (const auto &gpu : gpu_bytes)
        {
            printf("%s\n", gpu.first.c_str());
        }
        return EXIT_FAILURE;
    }

    ifstream input(input_file_name, ios::binary);

    if (!input.good())
    {
        cerr << "Could not read from file " << input_file_name << endl;
        return EXIT_FAILURE;
    }

    vector<unsigned char> buffer = deunifyBinary(input, gpu_name, strcmp(argv[1], "deunify-xz") == 0);

    // write decoded binary array
    ofstream output(output_file_name, ios::binary);

    if (!output.good())
    {
        cerr << "Could not write to file " << output_file_name << endl;
        return EXIT_FAILURE;
    }

    for (unsigned char c : buffer)
    {
        output << c;
    }

    return status;
}

vector<uint8_t> int_le(int x)
{
    return vector<uint8_t>{
        static_cast<uint8_t>(x & 255),
        static_cast<uint8_t>((x >> 8) & 255),
        static_cast<uint8_t>((x >> 16) & 255),
        static_cast<uint8_t>((x >> 24) & 255)};
}

bool recursiveMatch(const wstring &wildcard, const wstring::const_iterator &wildcardIter, const wstring &fileName, const wstring::const_iterator &fileNameIter)
{
    auto iFileName = fileNameIter;
    for (auto iWildcard = wildcardIter; iWildcard != wildcard.end(); ++iWildcard)
    {
        switch (*iWildcard)
        {
        case '?':
            if (iFileName == fileName.end())
                return false;
            ++iFileName;
            break;
        case '*':
        {
            if ((iWildcard + 1) == wildcard.end())
                return true;
            size_t max = fileName.end() - iFileName;
            for (size_t i = 0; i < max; i++)
                if (recursiveMatch(wildcard, iWildcard + 1, fileName, iFileName + i))
                    return true;
            return false;
        }
        default:
            if (*iFileName != *iWildcard)
                return false;
            ++iFileName;
        }
    }
    return iFileName == fileName.end();
}

bool match(const wstring &wildcard, const wstring &fileName)
{
    auto iw = wildcard.begin();
    return recursiveMatch(wildcard, iw, fileName, fileName.begin());
}
