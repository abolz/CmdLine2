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
    cli.PrintDiag();
    if (!res) {
        cli.PrintHelp();
    }
    return res;
}

static ParseResult ParseMakeCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli("finder make", "Make a new finder");

    cli.Add("wordfile", "", cl::Positional::yes | cl::Arg::required | cl::Required::yes | cl::Multiple::yes, cl::Var(input));
    cli.Add("dict", "", cl::Arg::required | cl::Required::yes, cl::Var(dict));
    cli.Add("progress", "show progress", {}, cl::Var(progr));

    return Parse(cli, next, last);
}

static ParseResult ParseFindCommand(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli("finder find", "Find an existing finder");

    cli.Add("infile", "", cl::Required::yes | cl::Multiple::yes | cl::Arg::required | cl::Positional::yes, cl::Var(input));
    cli.Add("dict", "", cl::Arg::required | cl::Required::yes, cl::Var(dict));
    cli.Add("o", "write to file instead of stdout", cl::Arg::required, cl::Var(out));
    cli.Add("split|nosplit", "(do not) split output", {}, cl::Flag(split, /*inverse_prefix*/ "no"));

    return Parse(cli, next, last);
}

static bool ParseCommandLine(ArgIterator next, ArgIterator last)
{
    cl::Cmdline cli("finder", "");

    cli.Add("v|version", "show version", cl::Arg::no, [](cl::ParseContext const& /*ctx*/) { printf("version 1.0\n"); });
    cli.Add("mode",
        "Can be make|find|help\n"
        "  <make>  \tMake a new finder?!\n"
        "  <find>  \tFind an existing finder.\n"
        "  <help>  \tShow help menu",
        cl::Required::yes | cl::Positional::yes | cl::StopParsing::yes,
        cl::Map(selected, { {"make", mode::make},
                            {"find", mode::find},
                            {"help", mode::help} }));

    if (auto const res = Parse(cli, next, last))
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
