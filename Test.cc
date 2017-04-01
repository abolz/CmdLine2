#include "Cmdline.h"
//#include "CmdlineUtils.h"

#include <forward_list>
#include <iostream>
#include <list>
#include <string>
#include <vector>

static void Usage()
{
    std::cerr << "usage: app.exe [options] input-files\n";
}

static void ShowHelp()
{
    Usage();

    std::cerr << "\n";
    std::cerr << "options:\n";
    std::cerr << "  -h            Display this message\n";
    std::cerr << "  -f [bool]     ?\n";
    std::cerr << "  -F            Same as '-f=true'\n";
    std::cerr << "  -i [int,...]  List of integers\n";
}

int main(int argc, char* argv[])
{
    bool flag = false;
    int i = 0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd { &std::cerr };

    bool show_help = false;
    //auto show_help = [](auto name, auto arg, auto index) -> bool {
    //    Usage(name, arg, index);
    //    exit(EXIT_SUCCESS); // :-(
    //};

    //cmd.Add(show_help, "h");
    cmd.AddValue(show_help, "h");

    auto opt_f  = cmd.AddValue(flag, "f", cl::Opt::Optional, cl::Arg::Optional, cl::MayGroup::No);
    auto opt_F  = cmd.AddValue(flag, "F", cl::Opt::Optional, cl::Arg::None, cl::MayGroup::Yes);
    auto opt_i  = cmd.Add(cl::Value(i), "i", cl::Opt::ZeroOrMore, cl::Arg::Required, cl::CommaSeparatedArg::Yes);
    auto opt_in = cmd.AddList(input_files, "input-files", cl::Opt::OneOrMore, cl::Positional::Yes, cl::Arg::Required);

    //cmd.AddValue(flag, "f", cl::CheckMissing::No);

    auto sink = [&](auto curr, int index) {
        std::cerr << "note(" << index << "): unknown option '" << *curr << "'\n";
        return true; // continue
    };

    std::forward_list<std::string> args{argv + 1, argv + argc};

    //if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::Yes, [](auto...){ return true; }))
    //if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::Yes, sink))
    //if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::No, sink))
    if (!cmd.Parse(args.begin(), args.end(), cl::CheckMissing::No, sink))
    {
        ////Usage();
        //ShowHelp();
        return -1;
    }

    if (show_help)
    {
        ShowHelp();
        return 0;
    }

    if (!cmd.CheckMissingOptions())
    {
        ////Usage();
        //ShowHelp();
        return -1;
    }

    std::cout << "opt_f: " << opt_f->count() << "\n";
    std::cout << "opt_F: " << opt_F->count() << "\n";
    std::cout << "opt_i: " << opt_i->count() << "\n";
    std::cout << "opt_in: " << opt_in->count() << "\n";

    return 0;
}
