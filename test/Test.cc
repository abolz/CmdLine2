#include "../src/Cmdline.h"
//#include "../src/Cmdline_utils.h"
#include <iterator>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("Flags")
{
    bool a = false;
    bool b = false;
    bool c = false;
    bool d = false;

    cl::Cmdline cl;
    cl.Add("a", "<descr>", cl::Assign(a), cl::Opt::zero_or_more, cl::Arg::no,       cl::MayGroup::yes);
    cl.Add("b", "<descr>", cl::Assign(b), cl::Opt::zero_or_more, cl::Arg::optional, cl::MayGroup::yes);
    cl.Add("c", "<descr>", cl::Assign(c), cl::Opt::zero_or_more, cl::Arg::required, cl::MayGroup::yes);
    cl.Add("d", "<descr>", cl::Assign(d), cl::Opt::zero_or_more, cl::Arg::required, cl::JoinArg::yes);

    char const* argv[] = {
        "program_name",
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
    int argc = static_cast<int>(std::distance(std::begin(argv), std::end(argv)));

    CHECK(true == cl.Parse(argv + 1, argv + argc));
    cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == true);
}

TEST_CASE("Numbers")
{
    int a = 0;
    long long b = 0;
    unsigned char c = 0;
    double d = 0;

    cl::Cmdline cl;
    cl.Add("a", "<descr>", cl::Assign(a), cl::Opt::zero_or_more, cl::Arg::required);
    cl.Add("b", "<descr>", cl::Assign(b), cl::Opt::zero_or_more, cl::Arg::required);
    cl.Add("c", "<descr>", cl::Assign(c), cl::Opt::zero_or_more, cl::Arg::required);
    cl.Add("d", "<descr>", cl::Assign(d), cl::Opt::zero_or_more, cl::Arg::required);

    char const* argv[] = {
        "program_name",
        "-a=1",
        "-a=10001",
        "-a=0x10001",
        "-a=-0x1",
        "-b", "-100",
        "-b", "+100",
        "-c", "0",
        "-c", "255",
        "-d=1",
        "-d=-1",
        "-d=1.234",
        "-d=-1.234",
        "-d=+1.234",
        "-d=-1.234e+05",
        "-d=+1.234e+05",
    };
    int argc = static_cast<int>(std::distance(std::begin(argv), std::end(argv)));

    CHECK(true == cl.Parse(argv + 1, argv + argc));
    cl.PrintDiag();
}

enum class Simpson {
    Homer,
    Marge,
    Bart,
    Lisa,
    Maggie,
};

TEST_CASE("Map")
{
    Simpson simpson = Simpson::Homer;

    cl::Cmdline cl;

    cl.Add("simpson", "<descr>",
        cl::Map(simpson, {{"Homer",    Simpson::Homer },
                          {"Marge",    Simpson::Marge },
                          {"Bart",     Simpson::Bart  },
                          {"El Barto", Simpson::Bart  },
                          {"Lisa",     Simpson::Lisa  },
                          {"Maggie",   Simpson::Maggie}}),
        cl::Opt::zero_or_more, cl::Arg::required);

    std::vector<char const*> argvs[] = {
        {"-simpson", "Homer"},
        {"-simpson=Marge"},
        {"-simpson", "El Barto"},
        //{"-simpson", "Granpa"},
    };

    for (auto const& argv : argvs)
    {
       CHECK(true == cl.Parse(argv.begin(), argv.end()));
       cl.PrintDiag();
    }
}

#if 0

struct c_str_end {
    friend bool operator==(c_str_end, c_str_end) { return true; }
    friend bool operator!=(c_str_end, c_str_end) { return false; }
    friend bool operator==(char const* lhs, c_str_end) { return *lhs == '\0'; }
    friend bool operator!=(char const* lhs, c_str_end) { return *lhs != '\0'; }
};

TEST_CASE("Flags_Tokenize")
{
    bool a = false;
    bool b = false;
    bool c = false;
    bool d = false;

    cl::Cmdline cl;
    cl.Add("a", "<descr>", cl::Assign(a), cl::Opt::zero_or_more, cl::Arg::no,       cl::MayGroup::yes);
    cl.Add("b", "<descr>", cl::Assign(b), cl::Opt::zero_or_more, cl::Arg::optional, cl::MayGroup::yes);
    cl.Add("c", "<descr>", cl::Assign(c), cl::Opt::zero_or_more, cl::Arg::required, cl::MayGroup::yes);
    cl.Add("d", "<descr>", cl::Assign(d), cl::Opt::zero_or_more, cl::Arg::required, cl::JoinArg::yes);

    char const* command_line
        = "-a --b -b -b=true -b=0 -b=on -c          false -c=0 -c=1 -c=true -c=false -c=on --c=off -c=yes --c=no -ac true -ab -ab=true -dtrue -dno -d1";

    CHECK(true == cl.Parse(cl::argv_begin(command_line, c_str_end{}), cl::argv_end()));
    cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == true);
}

#endif
