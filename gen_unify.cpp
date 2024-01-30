#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

unordered_map<string, vector<char> > gpu_bytes = {
    {"gfx1030", vector<char>{54, 48}},
    {"gfx1031", vector<char>{55, 49}},
    {"gfx1032", vector<char>{56, 50}},
    {"gfx1033", vector<char>{57, 51}},
    {"gfx1034", vector<char>{62, 52}},
    {"gfx1035", vector<char>{61, 53}},
    {"gfx1036", vector<char>{69, 54}}
};

void print_usage();
int encode(int argc, char** argv);
int decode(int argc, char** argv);
int test(int argc, char** argv);
bool match(const wstring &wildcard, const wstring &fileName);

int main(int argc, char** argv) {
  const std::string module = argv[1];
  if (module == "encode") {
    return encode(argc, argv);
  } else if (module == "decode") {
    return decode(argc, argv);
  } else if (module == "test") {
    return test(argc, argv);
  } else {
    print_usage();
    return EXIT_FAILURE;
  }
}

void print_usage() {
  printf("Usage: encode input-directory input-template output\n");
  printf("Or: decode input output gpu-name\n");
  printf("Or: test\n");
}

int encode(int argc, char** argv) {
  auto status = EXIT_SUCCESS;
  if (5 != argc) {
    print_usage();
    return EXIT_FAILURE;
  }

  const string folder(argv[2]);
  const string inputFileTemplate(argv[3]);
  const string outputFileName(argv[4]);

  const wstring wInputFileTemplate(inputFileTemplate.begin(), inputFileTemplate.end());

  ofstream output(outputFileName, ios::binary);

  if (!output.good()) {
    cerr << "Could not write to file " << outputFileName << endl;
    return EXIT_FAILURE;
  }

  try
  {
    const fs::path folderPath(folder);
    vector<vector<char> > buffers;
    size_t fileNumber = 0;

    vector<fs::directory_entry> binFiles;
    for (auto& file : fs::directory_iterator(fs::absolute(folderPath))) {
      if (is_regular_file(file)) {
        binFiles.push_back(file);
      }
    }
    std::sort(begin(binFiles), end(binFiles), [](const fs::directory_entry &left, const fs::directory_entry &right) {return left.path().string() <= right.path().string(); });

    for (const auto& file : binFiles){
      const fs::path& p(file);
      const wstring name(p.filename().wstring());
      if (match(wInputFileTemplate, name)) {
        ifstream ifs(file.path(), ios::binary);
        if (!ifs.good()) {
          cerr << "Could not read file " << file.path() << endl;
        }
        vector<char> buffer((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));

        if (fileNumber > 0 && buffer.size() != buffers.back().size()) {
          cerr << "Sizes don't match in " << inputFileTemplate << " in " << folder << endl;
        }

        //save all buffers
        buffers.emplace_back(buffer);

        fileNumber++;
      }
    }

    if (0 == fileNumber) {
      std::cerr << "No matches found for " << inputFileTemplate << " in " << folder << std::endl;
      return EXIT_FAILURE;
    }

    // find positions with GPUname-defined bytes
    vector<int> diff_pos;
    size_t size = buffers[0].size();
    for (size_t i = 0; i < size; i++) {
      for (size_t j = 1; j < fileNumber; j++) {
        if (buffers[j][i] != buffers[j - 1][i] && (diff_pos.empty() || diff_pos.back() != i)) {
          diff_pos.emplace_back(i);
        }
      }
    }

    // delete different bytes
    vector<char> result = buffers[0];
    for (size_t i = 0; i < diff_pos.size(); i++) {
      result.erase(result.begin() + diff_pos[i] - i);
    }

    // write erased bytes positions and the resulting array into a binary file
    size_t sz = diff_pos.size();
    output.write(reinterpret_cast<const char*> (&sz), sizeof(size_t));

    for (int const i : diff_pos) {
      output.write(reinterpret_cast<const char*>(&i), sizeof(int));
    }

    for (char const c : result) {
      output << c;
    }
  }
  catch (const std::exception&)
  {
    cerr << "An error occurred! Sorry ):" << endl;
    status = EXIT_FAILURE;
  }

  return status;
}

int decode(int argc, char** argv) {
  auto status = EXIT_SUCCESS;
  if (5 != argc) {
    print_usage();
    return EXIT_FAILURE;
  }

  const string input_file_name(argv[2]);
  const string output_file_name(argv[3]);
  const string gpu_name(argv[4]);

  if (!gpu_bytes.count(gpu_name)) {
    printf("Sorry, this GPU is not supported yet!\nSupported GPUs:\n");
    for (const auto& gpu : gpu_bytes) {
      printf("%s\n", gpu.first.c_str());
    }
    return EXIT_FAILURE;
  }

  ifstream input(input_file_name, ios::binary);

  if (!input.good()) {
    cerr << "Could not read from file " << input_file_name << endl;
    return EXIT_FAILURE;
  }

  // read erased bytes positions
  size_t size;
  input.read(reinterpret_cast<char*>(&size), sizeof(size_t));
  vector<int> positions(size);
  for (size_t i = 0; i < size; i++) {
    input.read(reinterpret_cast<char*>(&positions[i]), sizeof(int));
  }
  vector<char> buffer((istreambuf_iterator<char>(input)), (istreambuf_iterator<char>()));

  // insert GPUname-defined bytes
  for (size_t i = 0; i < size; i++) {
    buffer.insert(buffer.begin() + positions[i], gpu_bytes[gpu_name][i]);
  }

  // write decoded binary array
  ofstream output(output_file_name, ios::binary);

  if (!output.good()) {
    cerr << "Could not write to file " << output_file_name << endl;
    return EXIT_FAILURE;
  }

  for (char c : buffer) {
    output << c;
  }

  return status;
}

int test(int argc, char** argv) {
  try {
    fs::create_directory(fs::path("kernel-gfx103"));

    for (const auto &gfx: gpu_bytes) {
      string cmd = "/opt/rocm-6.0.0/bin/clang-ocl solve.cl -mcpu="
                   + gfx.first
                   + " -o kernel-gfx103/kernel-"
                   + gfx.first
                   + ".bin";
      system(cmd.c_str());
    }
    system("rm kernel-gfx103/*orig*");

    char **encode_argv = new char *[5];
    string folder = "kernel-gfx103";
    string file_template = "kernel-gfx*";
    string output = "compressed.bin.cpr";
    encode_argv[2] = const_cast<char *>(folder.c_str());
    encode_argv[3] = const_cast<char *>(file_template.c_str());
    encode_argv[4] = const_cast<char *>(output.c_str());

    encode(5, encode_argv);
    delete[](encode_argv);

    bool success = true;
    for (const auto &gfx: gpu_bytes) {
      char **decode_argv = new char *[5];
      string input = "compressed.bin.cpr";
      output = "decompressed.bin";
      string gpuname = gfx.first;
      decode_argv[2] = const_cast<char *>(input.c_str());
      decode_argv[3] = const_cast<char *>(output.c_str());
      decode_argv[4] = const_cast<char *>(gpuname.c_str());

      decode(5, decode_argv);
      delete[](decode_argv);
      ifstream ifs1("kernel-gfx103/kernel-" + gfx.first + ".bin", ios::binary);
      ifstream ifs2("decompressed.bin", ios::binary);
      vector<char> buf1((istreambuf_iterator<char>(ifs1)), (istreambuf_iterator<char>()));
      vector<char> buf2((istreambuf_iterator<char>(ifs2)), (istreambuf_iterator<char>()));
      if (buf1 != buf2) {
        cout << "FAIL" << endl;
        return EXIT_SUCCESS;
      }
    }

    system("rm *compressed*");
  } catch (const std::exception&) {
    cerr << "An error occurred! Sorry ):" << endl;
    return EXIT_FAILURE;
  }

  cout << "SUCCESS" << endl;

  return EXIT_SUCCESS;
}

bool recursiveMatch(const wstring &wildcard, const wstring::const_iterator &wildcardIter, const wstring &fileName, const wstring::const_iterator &fileNameIter)
{
  auto iFileName = fileNameIter;
  for (auto iWildcard = wildcardIter; iWildcard != wildcard.end(); ++iWildcard) {
    switch (*iWildcard) {
      case '?':
        if (iFileName == fileName.end())
          return false;
        ++iFileName;
        break;
      case '*': {
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


