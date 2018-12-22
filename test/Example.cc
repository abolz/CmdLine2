#include "Cmdline.h"

enum class Standard { cxx11, cxx14, cxx17 };

static bool verbose = false;
static int i = 0;
static Standard standard = Standard::cxx11;
static std::vector<std::string> input_files;

int main(int argc, char* argv[])
{
    cl::Cmdline cli("Example", "Does nothing useful");

    cli.Add("v", "Increase output verbosity", cl::Multiple | cl::MayGroup | cl::Arg::optional,
        cl::Var(verbose));

    cli.Add("i|ints", "Some ints in the range [0,6]", cl::Multiple | cl::Arg::required | cl::CommaSeparated,
        cl::Var(i, cl::check::InRange(0, 6)));

    cli.Add("std", "C++ standard version", cl::Arg::required,
        cl::Map(standard, {{"c++11", Standard::cxx11}, {"c++14", Standard::cxx14}, {"c++17", Standard::cxx17}}));

    cli.Add("input-files", "List of input files", cl::Required | cl::Multiple | cl::Positional,
        cl::Var(input_files));

    auto const res = cli.Parse(argv + 1, argv + argc);
    cli.PrintDiag(); // Print error and/or warning messages to stderr.
    if (!res) {
        cli.PrintHelp(); // Print help message to stderr.
        return -1;
    }

    return 0;
}
