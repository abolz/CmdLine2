//#define CL_WINDOWS_CONSOLE_COLORS 1
//#define CL_ANSI_CONSOLE_COLORS 1
#include "Cmdline.h"

enum class Standard { cxx11, cxx14, cxx17 };

static bool verbose = false;
static int i = 0;
static Standard standard = Standard::cxx11;
static std::vector<std::string> input_files;

int main(int argc, char* argv[])
{
    cl::Cmdline cmd;

    cmd.Add("v", "Increase output verbosity",
        cl::Assign(verbose),
        cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::optional);

    cmd.Add("i|ints", "Some ints in the range [0,6]",
        cl::Assign(i, cl::check::InRange(0, 6)),
        cl::NumOpts::zero_or_more, cl::HasArg::yes, cl::CommaSeparated::yes);

    cmd.Add("std", "C++ standard version",
        cl::Map(standard, {{"c++11", Standard::cxx11},
                           {"c++14", Standard::cxx14},
                           {"c++17", Standard::cxx17}}),
        cl::HasArg::yes);

    cmd.Add("input-files", "List of input files",
        cl::PushBack(input_files),
        cl::NumOpts::one_or_more, cl::Positional::yes);

    bool const ok = cmd.Parse(argv + 1, argv + argc);
    if (!ok)
    {
        cmd.PrintDiag();          // Print error message(s) to stderr.
        cmd.PrintHelp("Example"); // Print help message to stderr.
        return -1;
    }

    return 0;
}
