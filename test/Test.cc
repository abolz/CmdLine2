#include "../src/Cmdline.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("Flags")
{
    bool a = false;
    bool b = false;
    bool c = false;
    bool d = false;

    cl::Cmdline cl;
    cl.Add(cl::Value(a), "a", cl::Opt::zero_or_more, cl::Arg::no,       cl::MayGroup::yes);
    cl.Add(cl::Value(b), "b", cl::Opt::zero_or_more, cl::Arg::optional, cl::MayGroup::yes);
    cl.Add(cl::Value(c), "c", cl::Opt::zero_or_more, cl::Arg::required, cl::MayGroup::yes);
    cl.Add(cl::Value(d), "d", cl::Opt::zero_or_more, cl::Arg::required, cl::JoinArg::yes);

    std::initializer_list<char const*> args = {
        "-a",
        "--b",
        "-b",
        "-b=true",
        "-b=0",
        "-b=on",
        "-c", "false",
        "-c=0",
        "-c=1",
        "-c=true",
        "-c=false",
        "-c=on",
        "--c=off",
        "-c=yes",
        "--c=no",
        "-ac", "true",
        "-ab",
        "-ab=true",
        "-dtrue",
        "-dno",
        "-d1",
    };

    CHECK(true == cl.Parse(args.begin(), args.end()));
    cl.PrintErrors();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == true);
}

//------------------------------------------------------------------------------
// HACK
//
#include "../src/Cmdline.cc"
#include "../src/StringSplit.cc"
