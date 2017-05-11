#include "Cmdline.h"
//#include "CmdlineUtils.h"

#include <iostream>

int main(int argc, char* argv[])
{
    bool show_help = false;
    bool flag = false;
    int i = 0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd;

    auto opt_h = cmd.AddValue(show_help, "h|help|?", "Display this message");

    auto opt_f = cmd.AddValue(flag, "f", "A boolean flag",
        cl::ZeroOrMore, cl::MayGroup::Yes, cl::Arg::Optional);

    auto opt_i = cmd.AddValue(i, "i|ints", "Some ints",
        cl::ZeroOrMore, cl::Arg::Required, cl::CommaSeparatedArg::Yes);

    auto opt_in = cmd.AddList(input_files, "input-files", "List of input files",
        cl::OneOrMore, cl::Positional::Yes);

    bool const ok = cmd.Parse(argv + 1, argv + argc);
    if (show_help) {
        cmd.ShowHelp(std::cout, "Test");
        return 0;
    }
    if (!ok) {
        std::cerr << cmd.diag().str() << "\n";
        std::cerr << "use '-" << opt_h->name() << "' for help\n";
        return -1;
    }

    std::cout << "opt_f: " << opt_f->count() << "\n";
    std::cout << "  f = " << flag << "\n";
    std::cout << "opt_i: " << opt_i->count() << "\n";
    std::cout << "  i = " << i << "\n";
    std::cout << "opt_in: " << opt_in->count() << "\n";
    for (auto& str : input_files)
        std::cout << "  '" << str << "'\n";

    return 0;
}
