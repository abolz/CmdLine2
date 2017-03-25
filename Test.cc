#include "Cmdline.h"
//#include "CmdlineUtils.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    bool flag = false;
    int i = 0;
    std::vector<std::string> input_files;

    cl::Cmdline cmd { &std::cerr };

    auto opt_f  = cmd.AddValue(flag, "f", cl::Opt::Optional, cl::Arg::Optional, cl::MayGroup::No);
    auto opt_i  = cmd.Add(cl::Value(i), "i", cl::Opt::ZeroOrMore, cl::Arg::Required, cl::CommaSeparatedArg::Yes);
    auto opt_in = cmd.AddList(input_files, "input-files", cl::Opt::OneOrMore, cl::Positional::Yes, cl::Arg::Required);

    if (!cmd.Parse(argv + 1, argv + argc, cl::CheckMissing::Yes, cl::IgnoreUnknown::No))
        return -1;

    std::cout << "opt_f: " << opt_f->count() << "\n";
    std::cout << "opt_i: " << opt_i->count() << "\n";
    std::cout << "opt_in: " << opt_in->count() << "\n";

    return 0;
}
