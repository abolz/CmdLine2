#include "../src/Cmdline.h"
//#include <iostream>
#include <unordered_map>
//#include <sstream>

enum class Simpson {
    homer,
    marge,
    bart,
    lisa,
    maggie,
};

int main(int argc, char* argv[])
{
    bool show_help = false;
    bool flag = false;
    int i = 0;
    double f = 0.0;
    std::vector<std::string> input_files;
    Simpson simpson;
    std::unordered_map<std::string, int> hmap;

    cl::Cmdline cmd;

    /*auto opt_h =*/ cmd.Add("h|help|?", "Display this message", cl::Assign(show_help));

    cmd.Add("f", "A boolean flag", 
        cl::Assign(flag), 
        cl::Opt::zero_or_more, cl::MayGroup::yes, cl::Arg::optional);

    auto IsEven = [](cl::ParseContext& ctx, auto i) {
        if (i % 2 == 0)
            return true;
        ctx.cmdline->EmitDiag(cl::Diagnostic::error, ctx.index, "Argument must be an even integer");
        return false;
    };

    auto Times2 = [](cl::ParseContext const& /*ctx*/, int& i) { i += i; return true; };

    cmd.Add("i|ints", "Some even ints in the range [0,6]",
        cl::Assign(i, cl::check::InRange(0, 6), IsEven, Times2),
        cl::Opt::zero_or_more, cl::Arg::required, cl::CommaSeparatedArg::yes);

    cmd.Add("floats", "Some floats in the range [0,pi]",
        cl::Assign(f, cl::check::InRange(0, 3.1415)),
        cl::Opt::zero_or_more, cl::Arg::required, cl::CommaSeparatedArg::yes);

    cmd.Add("input-files", "List of input files",
        cl::PushBack(input_files),
        cl::Opt::one_or_more, cl::Positional::yes);

    cmd.Add("simpson", "One of the Simpsons",
        cl::Map(simpson, {{"homer",    Simpson::homer },
                          {"marge",    Simpson::marge },
                          {"bart",     Simpson::bart  },
                          {"el barto", Simpson::bart  },
                          {"lisa",     Simpson::lisa  },
                          {"maggie",   Simpson::maggie}}),
        cl::Opt::zero_or_more, cl::Arg::required);

    cmd.Add("si", "String-Int pairs",
        cl::PushBack(hmap),
        cl::Opt::zero_or_more, cl::Arg::required);

    bool const ok = cmd.Parse(argv + 1, argv + argc);
    //if (show_help)
    //{
    //    std::cerr << cmd.HelpMessage("Test");
    //    return 0;
    //}
    if (!ok)
    {
        cmd.PrintDiag();
        cmd.PrintHelp("Example"); //cmd.PrintHelpMessage(argv[0]);
        return -1;
    }

    //std::cout << "i = " << i << "\n";

    return 0;
}
