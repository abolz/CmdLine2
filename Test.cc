#include "Cmdline.h"
//#include "CmdlineUtils.h"

#include <forward_list>
#include <iostream>
#include <list>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    bool show_help = false;
    bool flag = false;
    int i = 0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd { &std::cerr };

    /*auto opt_h =*/ cmd.AddValue(show_help, "h", // "h|help|?",   <===  TODO
        cl::HelpText("Display this message"),
        cl::Opt::Optional,
        cl::Arg::None);
    auto opt_f = cmd.AddValue(flag, "f",
        cl::HelpText("Some flags"),
        cl::Opt::Optional,
        cl::Arg::Optional,
        cl::MayGroup::No);
    auto opt_F = cmd.AddValue(flag, "F",
        cl::HelpText("A boolean flag"),
        cl::Opt::Required,
        cl::Arg::None,
        cl::MayGroup::Yes);
    auto opt_i = cmd.Add(cl::Value(i), "i",
        cl::HelpText("Some ints"),
        cl::Opt::ZeroOrMore,
        cl::Arg::Required,
        cl::CommaSeparatedArg::Yes);
    auto opt_in = cmd.AddList(input_files, "input-files",
        cl::HelpText("List of input files"),
        cl::Opt::OneOrMore,
        cl::Positional::Yes,
        cl::Arg::Required);

    //cmd.AddValue(flag, "f", cl::CheckMissing::No);

    auto sink = [&](auto curr, int index) {
        std::cerr << "note(" << index << "): unknown option '" << *curr << "'\n";
        return true; // continue
    };

    std::forward_list<std::string> args{argv + 1, argv + argc};

    //if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::No, sink))
    if (!cmd.Parse(args.begin(), args.end(), cl::CheckMissing::No, sink))
        return -1;

    if (show_help) {
        cmd.ShowHelp(std::cout, "Test");
        return 0;
    }

    if (!cmd.CheckMissingOptions()) {
        cmd.ShowHelp(std::cout, "Test");
        return -1;
    }

    std::cout << "opt_f:  " << opt_f->count() << "\n";
    std::cout << "opt_F:  " << opt_F->count() << "\n";
    std::cout << "opt_i:  " << opt_i->count() << "\n";
    std::cout << "opt_in: " << opt_in->count() << "\n";

    return 0;
}
