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

unordered_map<string, vector<unsigned char>> gpu_bytes = {
    {"gfx601", vector<unsigned char>{33, 49}},
    {"gfx602", vector<unsigned char>{58, 50}},

    {"gfx700", vector<unsigned char>{34, 48}},
    {"gfx702", vector<unsigned char>{36, 50}},
    {"gfx703", vector<unsigned char>{37, 51}},
    {"gfx704", vector<unsigned char>{38, 52}},
    {"gfx705", vector<unsigned char>{59, 53}},

    {"gfx801", vector<unsigned char>{40, 1, 26, 48, 49, 203, 0, 175}},
    {"gfx802", vector<unsigned char>{41, 0, 96, 48, 50, 203, 2, 172}},
    {"gfx803", vector<unsigned char>{42, 0, 24, 48, 51, 139, 0, 172}},
    {"gfx805", vector<unsigned char>{60, 0, 96, 48, 53, 203, 2, 172}},
    {"gfx810", vector<unsigned char>{43, 1, 26, 49, 48, 203, 0, 172}},

    {"gfx900", vector<unsigned char>{44, 1, 48}},
    {"gfx902", vector<unsigned char>{45, 1, 50}},
    {"gfx904", vector<unsigned char>{46, 1, 52}},
    {"gfx906", vector<unsigned char>{47, 5, 54}},
    {"gfx909", vector<unsigned char>{49, 1, 57}},
    {"gfx90c", vector<unsigned char>{50, 1, 99}},

    {"gfx940", vector<unsigned char>{64, 48, 117, 222, 117, 222, 117, 222}},
    {"gfx941", vector<unsigned char>{75, 49, 117, 222, 117, 222, 117, 222}},
    {"gfx942", vector<unsigned char>{76, 50, 116, 220, 116, 220, 116, 220}},

    {"gfx1010", vector<unsigned char>{51, 48}},
    {"gfx1011", vector<unsigned char>{52, 49}},
    {"gfx1012", vector<unsigned char>{53, 50}},
    {"gfx1013", vector<unsigned char>{66, 51}},

    {"gfx1030", vector<unsigned char>{54, 48}},
    {"gfx1031", vector<unsigned char>{55, 49}},
    {"gfx1032", vector<unsigned char>{56, 50}},
    {"gfx1033", vector<unsigned char>{57, 51}},
    {"gfx1034", vector<unsigned char>{62, 52}},
    {"gfx1035", vector<unsigned char>{61, 53}},
    {"gfx1036", vector<unsigned char>{69, 54}},

    {"gfx1100", vector<unsigned char>{65, 48}},
    {"gfx1102", vector<unsigned char>{71, 50}},

    {"gfx1101", vector<unsigned char>{70, 49}},
    {"gfx1103", vector<unsigned char>{68, 51}}};

bool match(const wstring &wildcard, const wstring &fileName);

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

        // delete different bytes
        vector<unsigned char> result = buffers[0];
        for (size_t i = 0; i < diff_pos.size(); i++)
        {
            result.erase(result.begin() + diff_pos[i] - i);
        }

        if (strcmp(argv[1], "unify-xz") == 0)
        {
            result = compressData(result);
        }

        // write erased bytes positions and the resulting array into a binary file
        size_t sz = diff_pos.size();

        if (sz > 10)
        {
            cerr << "The binaries are not similar enough :(" << endl;
            return EXIT_FAILURE;
        }
        output.write(reinterpret_cast<const char *>(&sz), sizeof(size_t));

        for (int const i : diff_pos)
        {
            output.write(reinterpret_cast<const char *>(&i), sizeof(int));
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
    // read erased bytes positions
    size_t size;
    input.read(reinterpret_cast<char *>(&size), sizeof(size_t));
    vector<size_t> positions(size);
    for (size_t i = 0; i < size; i++)
    {
        input.read(reinterpret_cast<char *>(&positions[i]), sizeof(int));
    }
    vector<unsigned char> buffer((istreambuf_iterator<char>(input)), (istreambuf_iterator<char>()));

    if (decompression)
    {
        buffer = decompressData(buffer);
    }

    for (size_t i = 0; i < size; i++)
    {
        buffer.insert(buffer.begin() + positions[i], gpu_bytes[gpu_name][i]);
    }

    return buffer;
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
