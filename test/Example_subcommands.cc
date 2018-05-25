#define CL_WINDOWS_CONSOLE_COLORS 1
#define CL_ANSI_CONSOLE_COLORS 1
#include "../src/Cmdline.h"
#include <cstdio>

// Example from
// https://github.com/muellan/clipp

enum class mode {make, find, help};
static mode selected = mode::help;
static std::vector<std::string> input;
static std::string dict;
static std::string out;
static bool split = false;
static bool progr = false;

using ArgIterator = char const* const*;
using ParseResult = cl::Cmdline::ParseResult<ArgIterator>;

static ParseResult Parse(cl::Cmdline& cli, ArgIterator next, ArgIterator last, cl::string_view program_name)
{
    auto const res = cli.Parse(next, last);
    if (!res) {
        cli.PrintDiag();
        cli.PrintHelp(program_name);
    }
    return res;
}

static ParseResult ParseMakeCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli;

    cli.Add("wordfile", "", cl::Var(input), cl::Positional::yes, cl::HasArg::required, cl::NumOpts::one_or_more);
    cli.Add("dict", "", cl::Var(dict), cl::HasArg::required, cl::NumOpts::required);
    cli.Add("progress", "show progress", cl::Var(progr));

    return Parse(cli, next, last, "finder make");
}

static ParseResult ParseFindCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli;

    cli.Add("infile", "", cl::Var(input), cl::NumOpts::one_or_more, cl::HasArg::required, cl::Positional::yes);
    cli.Add("dict", "", cl::Var(dict), cl::HasArg::required, cl::NumOpts::required);
    cli.Add("o", "write to file instead of stdout", cl::Var(out), cl::HasArg::required);
    cli.Add("split|nosplit", "(do not) split output", cl::Flag(split, /*inverse_prefix*/ "no"));

    return Parse(cli, next, last, "finder find");
}

static bool ParseCommandLine(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli;

    cli.Add("v|version", "show version", [](cl::ParseContext const& /*ctx*/) { printf("version 1.0\n"); }, cl::HasArg::no);
    cli.Add("mode", "",
        cl::Map(selected, { {"make", mode::make},
                            {"find", mode::find},
                            {"help", mode::help} }),
        cl::NumOpts::required, cl::Positional::yes,
        cl::StopParsing::yes);

    if (auto const res = Parse(cli, next, last, "finder"))
    {
        switch (selected) {
        case mode::make:
            return ParseMakeCommand(res.next, last) ? 0 : -1;
        case mode::find:
            return ParseFindCommand(res.next, last) ? 0 : -1;
        case mode::help:
            cli.PrintHelp("finder");
            return 0;
        }
    }

    return -1;
}

int main(int argc, char* argv[])
{
    ParseCommandLine(argv + 1, argv + argc);
}
