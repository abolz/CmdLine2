#define CL_WINDOWS_CONSOLE_COLORS 1
#define CL_ANSI_CONSOLE_COLORS 1
#include "Cmdline.h"

enum class Standard { cxx11, cxx14, cxx17 };

static bool verbose = false;
static int i = 0;
static Standard standard = Standard::cxx11;
static std::vector<std::string> input_files;

#if 0// _MSC_VER
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    cl::Cmdline cli("Example", "Does nothing useful");

    cli.Add("v", "Increase output verbosity",
        cl::Var(verbose),
        cl::Multiple::yes, cl::MayGroup::yes, cl::Arg::optional);

    cli.Add("i|ints", "Some ints in the range [0,6]",
        cl::Var(i, cl::check::InRange(0, 6)),
        cl::Multiple::yes, cl::Arg::required, cl::CommaSeparated::yes);

    cli.Add("std", "C++ standard version",
        cl::Map(standard, {{"c++11", Standard::cxx11}, {"c++14", Standard::cxx14}, {"c++17", Standard::cxx17}}),
        cl::Arg::required);

    cli.Add("input-files", "List of input files",
        cl::Var(input_files),
        cl::Required::yes, cl::Multiple::yes, cl::Positional::yes);

    auto const res = cli.Parse(argv + 1, argv + argc);
    if (!res)
    {
        cli.PrintDiag(); // Print error messages to stderr.
        cli.PrintHelp(); // Print help message to stderr.
        return -1;
    }

    return 0;
}
