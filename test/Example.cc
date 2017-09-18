#include "Cmdline.h"

#include <unordered_map>

enum class Standard { cxx11, cxx14, cxx17 };

static bool verbose = false;
static int i = 0;
static Standard standard = Standard::cxx11;
static std::unordered_map<std::string, int> string_map;
static std::vector<std::string> input_files;

int main(int argc, char* argv[])
{
    cl::Cmdline cmd;

    // OK   "-v"
    // OK   "-v=1"
    // OK   "-v=no"
    // FAIL "-v=abcd"
    cmd.Add("v", "Increase output verbosity",
        cl::Assign(verbose),
        cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::optional);

    // OK   "-i 0 -i=1,2 -i=0x3 -i 4,5,6"
    // FAIL "-i=-1"
    // FAIL "-i=0x7"
    // FAIL "-i=abc"
    cmd.Add("i|ints", "Some ints in the range [0,6]",
        cl::Assign(i, cl::check::InRange(0, 6)),
        cl::NumOpts::zero_or_more, cl::HasArg::required, cl::CommaSeparated::yes);

    // OK   "-std c++11"
    // OK   "-std=c++17"
    // FAIL "-std"
    // FAIL "-std=c++20"
    // FAIL "-std=c++11 -std=c++11"
    cmd.Add("std", "C++ standard version",
        cl::Map(standard, {{ "c++11", Standard::cxx11 },
                           { "c++14", Standard::cxx14 },
                           { "c++17", Standard::cxx17 }}),
        cl::HasArg::required);

    // OK   "-si=eins:1"
    // OK   "-si eins:0x1,zwei:2"
    // FAIL "-si"
    // FAIL "-si null"
    // FAIL "-si null:abcd"
    cmd.Add("si", "String-Int pairs",
        cl::PushBack(string_map),
        cl::HasArg::required, cl::NumOpts::zero_or_more, cl::CommaSeparated::yes);

    // OK   "eins zwei drei"
    // FAIL ""
    cmd.Add("input-files", "List of input files",
        cl::PushBack(input_files), cl::NumOpts::one_or_more, cl::Positional::yes);

    bool const ok = cmd.Parse(argv + 1, argv + argc);
    if (!ok)
    {
        cmd.PrintDiag();          // Print error message(s) to stderr.
        cmd.PrintHelp("Example"); // Print help message to stderr.
        return -1;
    }

    return 0;
}
