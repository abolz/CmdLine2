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

    auto InRange = [&](auto lower, auto upper) {
        return [&, lower, upper](auto const& value) -> bool {
            if (!(value < lower) && !(upper < value))
                return true;
            cmd.diag() << "note: '" << value << "' is not in the range [" << lower << ", " << upper << "]\n";
            return false;
        };
    };

    auto IsEven = [&cmd](int i) {
        if (i % 2 == 0)
            return true;
        cmd.diag() << "note: '" << i << "' is not an even integer argument\n";
        return false;
    };

    auto opt_h = cmd.Add(cl::Value(show_help), "h|help|?", "Display this message");

    cmd.Add(cl::Value(flag), "f", "A boolean flag",
        cl::Opt::zero_or_more,
        cl::MayGroup::yes,
        cl::Arg::optional);

    auto Times2 = [](int& i) { i += i; return true; };

    cmd.Add(cl::Value(i, InRange(0,6), IsEven, Times2), "i|ints", "Some even ints in the range [0,5]",
        cl::Opt::zero_or_more,
        cl::Arg::required,
        cl::CommaSeparatedArg::yes);

    cmd.Add(cl::Value(f, InRange(0, 3.1415)), "floats", "Some floats in the range [0,pi]",
        cl::Opt::zero_or_more,
        cl::Arg::required,
        cl::CommaSeparatedArg::yes);

    cmd.Add(cl::List(input_files), "input-files", "List of input files",
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

    std::cout << "i = " << i << "\n";

    return 0;
}
#endif
#endif
