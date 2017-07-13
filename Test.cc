#if 0
int main() {}
#else
#include "Cmdline.h"
#if 0
int main() {}
#else
#include <iostream>

int main(int argc, char* argv[])
{
    bool show_help = false;
    bool flag = false;
    int i = 0;
    double f = 0.0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd;

    auto opt_h = cmd.AddValue(show_help, "h|help|?", "Display this message");

    cmd.AddValue(flag, "f", "A boolean flag",
        cl::Opt::zero_or_more,
        cl::MayGroup::yes,
        cl::Arg::optional);

    cmd.Add(cl::Value(i, 0, 5), "i|ints", "Some ints in the range [0,5]",
        cl::Opt::zero_or_more,
        cl::Arg::required,
        cl::CommaSeparatedArg::yes);

    cmd.Add(cl::Value(f, 0.0f, 3.1415f), "floats", "Some floats in the range [0,pi]",
        cl::Opt::zero_or_more,
        cl::Arg::required,
        cl::CommaSeparatedArg::yes);

    cmd.AddList(input_files, "input-files", "List of input files",
        cl::Opt::one_or_more,
        cl::Positional::yes);

    bool const ok = cmd.Parse(argv + 1, argv + argc);
    if (show_help)
    {
        cmd.ShowHelp(std::cout, "Test");
        return 0;
    }
    if (!ok)
    {
        std::cerr << cmd.diag().str() << "\n";
        std::cerr << "use '-" << opt_h->name() << "' for help\n";
        return -1;
    }

    return 0;
}
#endif
#endif
