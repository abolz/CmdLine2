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

static ParseResult Parse(cl::Cmdline& cli, ArgIterator next, ArgIterator last)
{
    auto const res = cli.Parse(next, last);
    if (!res) {
        cli.PrintDiag();
        cli.PrintHelp();
    }
    return res;
}

static ParseResult ParseMakeCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli("finder make");

    cli.Add("wordfile", "", cl::Var(input), cl::Positional::yes, cl::HasArg::required, cl::NumOpts::one_or_more);
    cli.Add("dict", "", cl::Var(dict), cl::HasArg::required, cl::NumOpts::required);
    cli.Add("progress", "show progress", cl::Var(progr));

    return Parse(cli, next, last);
}

static ParseResult ParseFindCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli("finder find");

    cli.Add("infile", "", cl::Var(input), cl::NumOpts::one_or_more, cl::HasArg::required, cl::Positional::yes);
    cli.Add("dict", "", cl::Var(dict), cl::HasArg::required, cl::NumOpts::required);
    cli.Add("o", "write to file instead of stdout", cl::Var(out), cl::HasArg::required);
    cli.Add("split|nosplit", "(do not) split output", cl::Flag(split, /*inverse_prefix*/ "no"));

    return Parse(cli, next, last);
}

static bool ParseCommandLine(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli;

    cli.Add("v|version", "show version", [](cl::ParseContext const& /*ctx*/) { printf("version 1.0\n"); }, cl::HasArg::no);
    cli.Add("mode",
        "Can be make|find|help\n"
        "  <make>  \tMake a new finder?!\n"
        "  <find>  \tFind an existing finder.\n"
        "  <help>  \tShow help menu",
        cl::Map(selected, { {"make", mode::make},
                            {"find", mode::find},
                            {"help", mode::help} }),
        cl::NumOpts::required, cl::Positional::yes,
        cl::StopParsing::yes);

    if (auto const res = Parse(cli, next, last, "finder"))
    {
        switch (selected) {
        case mode::make:
            return (bool)ParseMakeCommand(res.next, last);
        case mode::find:
            return (bool)ParseFindCommand(res.next, last);
        case mode::help:
            return 0;
        }
    }

    return false;
}

int main(int argc, char* argv[])
{
    ParseCommandLine(argv + 1, argv + argc);
}
