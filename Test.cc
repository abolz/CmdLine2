#include "Cmdline.h"
//#include "CmdlineUtils.h"

#include <iostream>

int main(int argc, char* argv[])
{
    bool flag = false;
    int i = 0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd { &std::cerr };

    auto on_help = [&](auto...)
    {
        cmd.ShowHelp(std::cout, "Test");
        exit(-1);
    };

    auto opt_h = cmd.Add(
        on_help, "h|help|?",
        cl::HelpText("Display this message"),
        cl::Opt::Optional,
        cl::Arg::None);
    auto opt_f = cmd.AddValue(
        flag, "f",
        cl::HelpText("Some flags"),
        cl::Opt::Optional,
        cl::Arg::Optional,
        cl::MayGroup::No);
    auto opt_F = cmd.AddValue(
        flag, "F",
        cl::HelpText("A boolean flag"),
        cl::Opt::Required,
        cl::Arg::None,
        cl::MayGroup::Yes);
    auto opt_i = cmd.Add(
        cl::Value(i), "i|ints",
        cl::HelpText("Some ints"),
        cl::Opt::ZeroOrMore,
        cl::Arg::Required,
        cl::CommaSeparatedArg::Yes);
    auto opt_in = cmd.AddList(
        input_files, "input-files",
        cl::HelpText("List of input files"),
        cl::Opt::OneOrMore,
        cl::Positional::Yes,
        cl::Arg::Required);

    auto sink = [&](auto curr, int index)
    {
        std::cerr << "note(" << index << "): unknown option '" << std::string_view(*curr) << "'\n";
        return true; // continue parsing
    };

    if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::Yes, sink)) {
        //std::cerr << "Use " << opt_h->name() << " for help\n";
        //return -1;
        on_help(0/*for g++...*/);
    }

    std::cout << "opt_f:  " << opt_f->count() << "\n";
    std::cout << "opt_F:  " << opt_F->count() << "\n";
    std::cout << "opt_i:  " << opt_i->count() << "\n";
    std::cout << "opt_in: " << opt_in->count() << "\n";

    return 0;
}
