#include <unify.h>

namespace fs = std::filesystem;
using namespace std;

void print_usage();

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return EXIT_USAGE;
    }

    const std::string module = argv[1];

    if (module.starts_with("unify"))
    {
        int status = unify(argc, argv);
        if (status == EXIT_USAGE)
        {
            print_usage();
        }
        return status;
    }
    else if (module.starts_with("deunify"))
    {
        int status = deunify(argc, argv);
        if (status == EXIT_USAGE)
        {
            print_usage();
        }
        return status;
    }
    else
    {
        print_usage();
        return EXIT_USAGE;
    }
}

void print_usage()
{
    printf("Usage: unify input-directory input-template output\n");
    printf("Or: deunify input output gpu-name\n");
    printf("Usage: unify-xz input-directory input-template output\n");
    printf("Or: deunify-xz input output gpu-name\n");
}
