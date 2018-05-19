#define CL_WINDOWS_CONSOLE_COLORS 1
#define CL_ANSI_CONSOLE_COLORS 1
#include "Cmdline.h"

#include <cstdint>
#include <climits>

#define CATCH_CONFIG_MAIN
#undef min
#undef max
#include "catch.hpp"

struct fancy_iterator
{
    using It                = std::initializer_list<char const*>::iterator;
    using iterator_category = std::input_iterator_tag; //std::iterator_traits<It>::iterator_category;
    using reference         = std::iterator_traits<It>::reference;
    using pointer           = std::iterator_traits<It>::pointer;
    using value_type        = std::iterator_traits<It>::value_type;
    using difference_type   = std::iterator_traits<It>::difference_type;

    It it;

    fancy_iterator() = default;
    explicit fancy_iterator(It it_) : it(it_) {}

#if 1
    char const* operator*() { return *it; }
    fancy_iterator& operator++() {
        ++it;
        return *this;
    }
#endif
#if 0
    std::string operator*() const
    {
        std::string s(*it);
        size_t k = s.size();
        s.append(200, '~');
        s.resize(k);
        return s;
    }
    fancy_iterator& operator++() {
        ++it;
        return *this;
    }
#endif
#if 0
    std::string s_;
    cl::string_view operator*()
    {
        s_ = *it;
        size_t k = s_.size();
        s_.append(200, '~');
        return cl::string_view(s_.c_str(), k);
    }
    fancy_iterator& operator++() {
        ++it;
        return *this;
    }
#endif

    friend bool operator==(fancy_iterator lhs, fancy_iterator rhs) { return lhs.it == rhs.it; }
    friend bool operator!=(fancy_iterator lhs, fancy_iterator rhs) { return lhs.it != rhs.it; }
};

//
// XXX:
// Cmdline needs a method like this, which takes a range...
//
static bool ParseArgs(cl::Cmdline& cl, std::initializer_list<char const*> args)
{
    fancy_iterator first{args.begin()};
    fancy_iterator last {args.end()};

#if 0
    if (cl.Parse(first, last).success)
        return true;

    cl.PrintDiag();
    return false;
#else
    return cl.Parse(first, last).success;
#endif
}

TEST_CASE("Opt")
{
    SECTION("optional")
    {
        bool a = false;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a)); // 'NumOpts::optional' is the default

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == false);
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        cl.Reset();
        CHECK(false == ParseArgs(cl, {"-a", "-a"}));
    }

    SECTION("required")
    {
        bool a = false;

        cl::Cmdline cl;
        auto opt_a = cl::MakeOption("a", "", cl::Var(a), cl::NumOpts::required);
        cl.Add(&opt_a);

        CHECK(false == ParseArgs(cl, {}));
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        cl.Reset();
        CHECK(false == ParseArgs(cl, {"-a", "-a"}));
    }

    SECTION("zero_or_more")
    {
        bool a = false;

        cl::Cmdline cl;
        auto opt_a = cl::MakeUniqueOption("a", "", cl::Var(a), cl::NumOpts::zero_or_more);
        cl.Add(std::move(opt_a));

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == false);
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a", "-a"}));
        CHECK(a == true);
    }

    SECTION("one_or_more")
    {
        bool a = false;

        cl::Cmdline cl;
#if CL_HAS_DEDUCTION_GUIDES
        cl::Option opt_a("a", "", cl::Var(a), cl::NumOpts::one_or_more);
        cl.Add(&opt_a);
#else
        cl.Add("a", "", cl::Var(a), cl::NumOpts::one_or_more);
#endif

        CHECK(false == ParseArgs(cl, {}));
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        cl.Reset();
        CHECK(true == ParseArgs(cl, {"-a", "-a"}));
        CHECK(a == true);
    }
}

TEST_CASE("Arg")
{
    SECTION("no")
    {
        bool a = false;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more); // 'arg_disallowed' is the default

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        CHECK(false == ParseArgs(cl, {"-a=true"}));
        CHECK(false == ParseArgs(cl, {"-a", "true"})); // unknown positional argument 'true'
    }

    SECTION("optional")
    {
        bool a = false;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::optional);

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=true"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=false"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=1"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=0"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=yes"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=no"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=on"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=off"}));
        CHECK(a == false);
        CHECK(false == ParseArgs(cl, {"-a", "true"})); // unknown positional argument 'true'
        CHECK(a == true); // should have been assigned to before generating an error
        CHECK(false == ParseArgs(cl, {"-a=123"}));
        CHECK(false == ParseArgs(cl, {"-a=hello"}));
    }

    SECTION("required")
    {
        bool a = false;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::yes);

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == false);
        CHECK(false == ParseArgs(cl, {"-a"}));
        CHECK(true == ParseArgs(cl, {"-a=true"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=false"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=1"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=0"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=yes"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=no"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a=on"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a=off"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a", "true"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a", "false"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a", "1"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a", "yes"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a", "no"}));
        CHECK(a == false);
        CHECK(true == ParseArgs(cl, {"-a", "on"}));
        CHECK(a == true);
        CHECK(true == ParseArgs(cl, {"-a", "off"}));
        CHECK(a == false);
        CHECK(false == ParseArgs(cl, {"-a=123"})); // invalid argument
        CHECK(false == ParseArgs(cl, {"-a=hello"})); // invalid argument
        CHECK(false == ParseArgs(cl, {"-a", "123"})); // invalid argument
        CHECK(false == ParseArgs(cl, {"-a", "hello"})); // invalid argument
    }
}

TEST_CASE("JoinArg")
{
}

TEST_CASE("MayGroup")
{
    bool a = false;
    bool b = false;
    bool c = false;
    bool ab = false;
    bool ac = false;
    bool abc = false;

    cl::Cmdline cl;

    cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::no);
    cl.Add("b", "", cl::Var(b), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::no);
    cl.Add("c", "", cl::Var(c), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {}));
    CHECK(true == ParseArgs(cl, {"-a", "-b", "-c=true"}));
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    a = false;
    b = false;
    c = false;
    CHECK(true == ParseArgs(cl, {"-abc=true"}));
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    a = false;
    b = false;
    c = false;
    CHECK(false == ParseArgs(cl, {"--abc=true"})); // "--" => not an option group
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);
    CHECK(true == ParseArgs(cl, {"-ababab", "-c", "true"}));
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    a = false;
    b = false;
    c = false;
    CHECK(true == ParseArgs(cl, {"-abbac", "true"}));
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    a = false;
    b = false;
    c = false;
    CHECK(false == ParseArgs(cl, {"-cba"})); // c must be the last option
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);
    CHECK(false == ParseArgs(cl, {"-abcab=true"})); // c must be the last option
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);

    cl.Add("ab", "", cl::Var(ab), cl::NumOpts::zero_or_more, cl::HasArg::no);

    CHECK(true == ParseArgs(cl, {"-ab", "--ab"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ab == true);
    ab = false;

    cl.Add("ac", "", cl::Var(ac), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {"-ac=true"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ac == true);
    ac = false;

    cl.Add("abc", "", cl::Var(abc), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {"-abc=true"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);
    CHECK(abc == true);
    abc = false;
    CHECK(true == ParseArgs(cl, {"-abc", "true"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);
    CHECK(abc == true);
    abc = false;
}

TEST_CASE("Positional")
{
    SECTION("strings")
    {
        std::vector<std::string> strings;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(strings), cl::Positional::yes, cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {}));
        CHECK(strings.empty());
        CHECK(true == ParseArgs(cl, {"eins"}));
        CHECK(strings.size() == 1);
        CHECK(strings[0] == "eins");
        CHECK(true == ParseArgs(cl, {"zwei", "drei"}));
        CHECK(strings.size() == 3);
        CHECK(strings[0] == "eins");
        CHECK(strings[1] == "zwei");
        CHECK(strings[2] == "drei");
    }

    SECTION("ints")
    {
        std::vector<int> ints;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(ints), cl::Positional::yes, cl::NumOpts::one_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"1"}));
        CHECK(true == ParseArgs(cl, {})); // succeeds because the count member is not automatically reset!
        CHECK(true == ParseArgs(cl, {"2"}));
        CHECK(true == ParseArgs(cl, {"3", "4"}));
        CHECK(ints.size() == 4);
        CHECK(ints[0] == 1);
        CHECK(ints[1] == 2);
        CHECK(ints[2] == 3);
        CHECK(ints[3] == 4);
    }
}

TEST_CASE("CommaSeparatedArg")
{
    std::vector<int> ints;

    cl::Cmdline cl;
    cl.Add("a", "", cl::Var(ints), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::CommaSeparated::yes);

    CHECK(true == ParseArgs(cl, {}));
    CHECK(ints.empty());
    CHECK(true == ParseArgs(cl, {"-a", "1"}));
    CHECK(ints.size() == 1);
    CHECK(ints[0] == 1);
    CHECK(false == ParseArgs(cl, {"-a", "hello"}));
    CHECK(ints.size() == 1);
    CHECK(ints[0] == 1);
    CHECK(true == ParseArgs(cl, {"-a", "2,3"}));
    CHECK(ints.size() == 3);
    CHECK(ints[0] == 1);
    CHECK(ints[1] == 2);
    CHECK(ints[2] == 3);
    CHECK(false == ParseArgs(cl, {"-a", "4,funf,6"})); // "funf" is not an int, but "4" should have been added!
    CHECK(ints.size() == 4);
    CHECK(ints[0] == 1);
    CHECK(ints[1] == 2);
    CHECK(ints[2] == 3);
    CHECK(ints[3] == 4);
}

TEST_CASE("StopParsing")
{
    std::string command;
    int a = 0;

    cl::Cmdline cli;
    cli.Add("a", "", cl::Var(a), cl::HasArg::optional);
    cli.Add("command", "", cl::Var(command), cl::Positional::yes, cl::HasArg::yes, cl::StopParsing::yes, cl::NumOpts::required);

    SECTION("1")
    {
        a = -1;

        cli.Reset();
        auto const args = {"-a=123", "command", "-b", "-c"};
        auto const res = cli.Parse(args.begin(), args.end(), cl::CheckMissingOptions::yes);
        CHECK(res.success);
        CHECK(res.next == args.begin() + 1);
        CHECK(a == 123);
        CHECK(command == "command");
    }
    SECTION("2")
    {
        a = -1;

        cli.Reset();
        auto const args = {"command", "-a=hello", "-b", "-c"};
        auto const res = cli.Parse(args.begin(), args.end(), cl::CheckMissingOptions::yes);
        CHECK(res.success);
        CHECK(res.next == args.begin() + 0);
        CHECK(a == -1);
        CHECK(command == "command");
    }
}

TEST_CASE("Subcommands")
{
    // ...
    // ...
    // ...
}

TEST_CASE("Strings")
{
    std::string str;

    cl::Cmdline cl;
    cl.Add("s", "", cl::Var(str), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {"-s=hello hello hello hello hello hello hello hello hello hello hello hello",
                                 "-s=world world world world world world world world world world world world"}));
    CHECK(str == "world world world world world world world world world world world world");
}

static_assert(INT_MIN == -INT_MAX - 1, "Tests assume two's complement");

TEST_CASE("Ints")
{
    SECTION("decimal signed 8-bit")
    {
        int8_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "127"}));
        CHECK(a == 127);
        CHECK(true == ParseArgs(cl, {"-a", "-128"}));
        CHECK(a == -128);
        CHECK(false == ParseArgs(cl, {"-a", "128"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "-129"})); // overflow
    }

    SECTION("decimal signed 32-bit")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "123"}));
        CHECK(a == 123);
        CHECK(true == ParseArgs(cl, {"-a", "-123"}));
        CHECK(a == -123);
        CHECK(true == ParseArgs(cl, {"-a", "2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "-2147483648"}));
        CHECK(a == INT32_MIN);
        CHECK(true == ParseArgs(cl, {"-a", "+2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "-2147483649"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "2147483648"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "+2147483648"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "true"})); // invalid argument
        CHECK(false == ParseArgs(cl, {"-a", "hello"})); // invalid argument
    }

    SECTION("decimal signed 64-bit")
    {
        int64_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a", "9223372036854775807"}));
        CHECK(a == INT64_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "-9223372036854775808"}));
        CHECK(a == INT64_MIN);
        CHECK(true == ParseArgs(cl, {"-a", "+9223372036854775807"}));
        CHECK(a == INT64_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "-9223372036854775809"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "9223372036854775808"})); // overflow
    }

    //
    // TODO:
    // Implement two's complement parser for integers?!?!
    //

    SECTION("hexadecimal signed 32-bit")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0x0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "-0x01"}));
        CHECK(a == -1);
        CHECK(true == ParseArgs(cl, {"-a", "-0x80000000"}));
        CHECK(a == INT32_MIN);
        CHECK(true == ParseArgs(cl, {"-a", "0x7FFFFFFF"}));
        CHECK(a == INT32_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "+0x7FFFFFFF"}));
        CHECK(a == INT32_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "-0x80000001"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "0x80000000"})); // overflow
    }

    SECTION("octal signed 8-bit")
    {
        int8_t a = -1;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "000"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "001"}));
        CHECK(a == 001);
        CHECK(true == ParseArgs(cl, {"-a", "077"}));
        CHECK(a == 077);
        CHECK(true == ParseArgs(cl, {"-a", "0177"}));
        CHECK(a == 0177);
        CHECK(true == ParseArgs(cl, {"-a", "-0177"}));
        CHECK(a == -0177);
        CHECK(true == ParseArgs(cl, {"-a", "-0200"}));
        CHECK(a == -0200);
        CHECK(false == ParseArgs(cl, {"-a", "-0201"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "0200"})); // overflow
    }
}

TEST_CASE("Floats")
{
    SECTION("fixed")
    {
    }

    SECTION("exponential")
    {
    }

    SECTION("hexadecimal")
    {
    }

    SECTION("inf")
    {
    }

    SECTION("nan")
    {
    }
}

TEST_CASE("Map")
{
    enum class Simpson { Homer, Marge, Bart, Lisa, Maggie };

    Simpson simpson = Simpson::Maggie;

    cl::Cmdline cl;
    cl.Add("simpson", "<descr>",
        cl::Map(simpson, {{ "Homer",    Simpson::Homer  },
                          { "Marge",    Simpson::Marge  },
                          { "Bart",     Simpson::Bart   },
                          { "El Barto", Simpson::Bart   },
                          { "Lisa",     Simpson::Lisa   },
                          { "Maggie",   Simpson::Maggie }}),
        cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {}));
    CHECK(simpson == Simpson::Maggie); // unchanged!
    CHECK(false == ParseArgs(cl, {"-simpson"})); // argument yes
    CHECK(simpson == Simpson::Maggie); // unchanged!
    CHECK(true == ParseArgs(cl, {"-simpson", "Homer"}));
    CHECK(simpson == Simpson::Homer);
    CHECK(true == ParseArgs(cl, {"-simpson=Marge"}));
    CHECK(simpson == Simpson::Marge);
    CHECK(true == ParseArgs(cl, {"-simpson", "Bart"}));
    CHECK(simpson == Simpson::Bart);
    CHECK(true == ParseArgs(cl, {"-simpson", "El Barto"}));
    CHECK(simpson == Simpson::Bart);
    CHECK(true == ParseArgs(cl, {"-simpson", "Lisa"}));
    CHECK(simpson == Simpson::Lisa);
    CHECK(true == ParseArgs(cl, {"-simpson=El Barto"})); // (quotes around "El Barto" removed by shell...)
    CHECK(simpson == Simpson::Bart);
    CHECK(true == ParseArgs(cl, {"-simpson", "Maggie"}));
    CHECK(simpson == Simpson::Maggie);
    CHECK(false == ParseArgs(cl, {"-simpson", "Granpa"})); // invalid argument
    CHECK(false == ParseArgs(cl, {"-simpson", "homer"})); // case-sensitive
}

TEST_CASE("Checks")
{
    SECTION("InRange")
    {
        int32_t a = 0;
        int32_t b = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a, cl::check::InRange(-3, 3)), cl::NumOpts::zero_or_more, cl::HasArg::required);
        cl.Add("b", "", cl::Var(b, cl::check::InRange(INT32_MIN, INT32_MAX)), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a=0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a=-3"}));
        CHECK(a == -3);
        CHECK(true == ParseArgs(cl, {"-a=3"}));
        CHECK(a == 3);
        CHECK(true == ParseArgs(cl, {"-a=+3"}));
        CHECK(a == 3);
        CHECK(false == ParseArgs(cl, {"-a=-4"}));
        CHECK(false == ParseArgs(cl, {"-a=+4"}));

        CHECK(true == ParseArgs(cl, {"-b=2147483647"}));
        CHECK(b == INT32_MAX);
        CHECK(true == ParseArgs(cl, {"-b=-2147483648"}));
        CHECK(b == INT32_MIN);
    }

    SECTION("GreaterThan")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a, cl::check::GreaterThan(-3)), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a=0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a=-2"}));
        CHECK(a == -2);
        CHECK(true == ParseArgs(cl, {"-a=3"}));
        CHECK(a == 3);
        CHECK(false == ParseArgs(cl, {"-a=-3"}));
        CHECK(false == ParseArgs(cl, {"-a=-4"}));
    }

    SECTION("GreaterEqual")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a, cl::check::GreaterEqual(7)), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a=7"}));
        CHECK(a == 7);
        CHECK(true == ParseArgs(cl, {"-a=2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(false == ParseArgs(cl, {"-a=6"}));
    }

    SECTION("LessThan")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a, cl::check::LessThan(-3)), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a=-4"}));
        CHECK(a == -4);
        CHECK(true == ParseArgs(cl, {"-a=-2147483648"}));
        CHECK(a == INT32_MIN);
        CHECK(false == ParseArgs(cl, {"-a=-3"}));
        CHECK(false == ParseArgs(cl, {"-a=-2"}));
    }

    SECTION("LessEqual")
    {
        int32_t a = 0;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Var(a, cl::check::LessEqual(INT32_MIN)), cl::NumOpts::zero_or_more, cl::HasArg::required);

        CHECK(true == ParseArgs(cl, {"-a=-2147483648"}));
        CHECK(a == INT32_MIN);
        CHECK(false == ParseArgs(cl, {"-a=-2147483647"}));
        CHECK(false == ParseArgs(cl, {"-a=+2147483647"}));
    }
}

TEST_CASE("Unicode")
{
    std::string str;

    cl::Cmdline cl;
    cl.Add(u8"😃-😜", "", cl::Var(str), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {u8"-😃-😜=hello😍😎world"}));
    CHECK(str == u8"hello😍😎world");
}

TEST_CASE("Wide strings")
{
    std::wstring str;

    cl::Cmdline cl;
    cl.Add(u8"😃-😜", "", cl::Var(str), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {u8"-😃-😜=hello😍😎world"}));
    CHECK(str == L"hello😍😎world");
    CHECK(true == ParseArgs(cl, {u8"-😃-😜=-😃-😜"}));
    CHECK(str == L"-😃-😜");
}

#if _WIN32
TEST_CASE("CommandLineToArgvUTF8")
{
}
#endif

TEST_CASE("Tokenize Windows 1")
{
    struct Test {
        std::string inp;
        std::vector<std::string> out;
    };

    const Test tests[] = {
    // with program name
        { R"(test )",                      { R"(test)" }                                            },
        { R"(test  )",                     { R"(test)" }                                            },
        { R"(test  ")",                    { R"(test)", R"()" }                                     },
        { R"(test  " )",                   { R"(test)", R"( )" }                                    },
        { R"(test foo""""""""""""bar)",    { R"(test)", R"(foo""""bar)" }                           },
        { R"(test foo"X""X""X""X"bar)",    { R"(test)", R"(fooX"XXXbar)" }                          },
        { R"(test    "this is a string")", { R"(test)", R"(this is a string)" }                     },
        { R"(test "  "this is a string")", { R"(test)", R"(  this)", R"(is)", R"(a)", R"(string)" } },
        { R"(test  " "this is a string")", { R"(test)", R"( this)", R"(is)", R"(a)", R"(string)" }  },
        { R"(test "this is a string")",    { R"(test)", R"(this is a string)" }                     },
        { R"(test "this is a string)",     { R"(test)", R"(this is a string)" }                     },
        { R"(test """this" is a string)",  { R"(test)", R"("this is a string)" }                    },
        { R"(test "hello\"there")",        { R"(test)", R"(hello"there)" }                          },
        { R"(test "hello\\")",             { R"(test)", R"(hello\)" }                               },
        { R"(test abc)",                   { R"(test)", R"(abc)" }                                  },
        { R"(test "a b c")",               { R"(test)", R"(a b c)" }                                },
        { R"(test a"b c d"e)",             { R"(test)", R"(ab c de)" }                              },
        { R"(test a\"b c)",                { R"(test)", R"(a"b)", R"(c)" }                          },
        { R"(test "a\"b c")",              { R"(test)", R"(a"b c)" }                                },
        { R"(test "a b c\\")",             { R"(test)", R"(a b c\)" }                               },
        { R"(test "a\\\"b")",              { R"(test)", R"(a\"b)" }                                 },
        { R"(test a\\\b)",                 { R"(test)", R"(a\\\b)" }                                },
        { R"(test "a\\\b")",               { R"(test)", R"(a\\\b)" }                                },
        { R"(test "\"a b c\"")",           { R"(test)", R"("a b c")" }                              },
        { R"(test "a b c" d e)",           { R"(test)", R"(a b c)", R"(d)", R"(e)" }                },
        { R"(test "ab\"c" "\\" d)",        { R"(test)", R"(ab"c)", R"(\)", R"(d)" }                 },
        { R"(test a\\\b d"e f"g h)",       { R"(test)", R"(a\\\b)", R"(de fg)", R"(h)" }            },
        { R"(test a\\\"b c d)",            { R"(test)", R"(a\"b)", R"(c)", R"(d)" }                 },
        { R"(test a\\\\"b c" d e)",        { R"(test)", R"(a\\b c)", R"(d)", R"(e)" }               },
        { R"(test "a b c""   )",           { R"(test)", R"(a b c")" }                               },
        { R"(test """a""" b c)",           { R"(test)", R"("a")", R"(b)", R"(c)" }                  },
        { R"(test """a b c""")",           { R"(test)", R"("a)", R"(b)", R"(c")" }                  },
        { R"(test """"a b c"" d e)",       { R"(test)", R"("a b c")", R"(d)", R"(e)" }              },
        { R"(test "c:\file x.txt")",       { R"(test)", R"(c:\file x.txt)" }                        },
        { R"(test "c:\dir x\\")",          { R"(test)", R"(c:\dir x\)" }                            },
        { R"(test "\"c:\dir x\\\"")",      { R"(test)", R"("c:\dir x\")" }                          },
        { R"(test "a b c")",               { R"(test)", R"(a b c)" }                                },
        { R"(test "a b c"")",              { R"(test)", R"(a b c")" }                               },
        { R"(test "a b c""")",             { R"(test)", R"(a b c")" }                               },
        { R"(test a b c)",                 { R"(test)", R"(a)", R"(b)", R"(c)" }                    },
        { R"(test a\tb c)",                { R"(test)", R"(a\tb)", R"(c)" }                         },
        { R"(test a\nb c)",                { R"(test)", R"(a\nb)", R"(c)" }                         },
        { R"(test a\vb c)",                { R"(test)", R"(a\vb)", R"(c)" }                         },
        { R"(test a\fb c)",                { R"(test)", R"(a\fb)", R"(c)" }                         },
        { R"(test a\rb c)",                { R"(test)", R"(a\rb)", R"(c)" }                         },

    // without program name
        { R"( )",                          { R"()" }                                                },
        { R"( ")",                         { R"()", R"()" }                                         },
        { R"( " )",                        { R"()", R"( )" }                                        },
        { R"(foo""""""""""""bar)",         { R"(foo""""""""""""bar)" }                              },
        { R"(foo"X""X""X""X"bar)",         { R"(foo"X""X""X""X"bar)" }                              },
        { R"(   "this is a string")",      { R"()", R"(this is a string)" }                         },
        { R"("  "this is a string")",      { R"(  )", R"(this)", R"(is)", R"(a)", R"(string)" }     },
        { R"( " "this is a string")",      { R"()", R"( this)", R"(is)", R"(a)", R"(string)" }      },
        { R"("this is a string")",         { R"(this is a string)" }                                },
        { R"("this is a string)",          { R"(this is a string)" }                                },
        { R"("""this" is a string)",       { R"()", R"(this)", R"(is)", R"(a)", R"(string)" }       },
        { R"("hello\"there")",             { R"(hello\)", R"(there)" }                              },
        { R"("hello\\")",                  { R"(hello\\)" }                                         },
        { R"(abc)",                        { R"(abc)" }                                             },
        { R"("a b c")",                    { R"(a b c)" }                                           },
        { R"(a"b c d"e)",                  { R"(a"b)", R"(c)", R"(de)" }                            },
        { R"(a\"b c)",                     { R"(a\"b)", R"(c)" }                                    },
        { R"("a\"b c")",                   { R"(a\)", R"(b)", R"(c)" }                              },
        { R"("a b c\\")",                  { R"(a b c\\)" }                                         },
        { R"("a\\\"b")",                   { R"(a\\\)", R"(b)" }                                    },
        { R"(a\\\b)",                      { R"(a\\\b)" }                                           },
        { R"("a\\\b")",                    { R"(a\\\b)" }                                           },
        { R"("\"a b c\"")",                { R"(\)", R"(a)", R"(b)", R"(c")" }                      },
        { R"("a b c" d e)",                { R"(a b c)", R"(d)", R"(e)" }                           },
        { R"("ab\"c" "\\" d)",             { R"(ab\)", R"(c \ d)" }                                 },
        { R"(a\\\b d"e f"g h)",            { R"(a\\\b)", R"(de fg)", R"(h)" }                       },
        { R"(a\\\"b c d)",                 { R"(a\\\"b)", R"(c)", R"(d)" }                          },
        { R"(a\\\\"b c" d e)",             { R"(a\\\\"b)", R"(c d e)" }                             },
        { R"("a b c""   )",                { R"(a b c)", R"(   )" }                                 },
        { R"("""a""" b c)",                { R"()", R"(a" b c)" }                                   },
        { R"("""a b c""")",                { R"()", R"(a b c")" }                                   },
        { R"(""""a b c"" d e)",            { R"()", R"(a)", R"(b)", R"(c)", R"(d)", R"(e)" }        },
        { R"("c:\file x.txt")",            { R"(c:\file x.txt)" }                                   },
        { R"("c:\dir x\\")",               { R"(c:\dir x\\)" }                                      },
        { R"("\"c:\dir x\\\"")",           { R"(\)", R"(c:\dir)", R"(x\")" }                        },
        { R"("a b c")",                    { R"(a b c)" }                                           },
        { R"("a b c"")",                   { R"(a b c)", R"()" }                                    },
        { R"("a b c""")",                  { R"(a b c)", R"()" }                                    },
        { R"(a b c)",                      { R"(a)", R"(b)", R"(c)" }                               },
        { R"(a\tb c)",                     { R"(a\tb)", R"(c)" }                                    },
        { R"(a\nb c)",                     { R"(a\nb)", R"(c)" }                                    },
        { R"(a\vb c)",                     { R"(a\vb)", R"(c)" }                                    },
        { R"(a\fb c)",                     { R"(a\fb)", R"(c)" }                                    },
        { R"(a\rb c)",                     { R"(a\rb)", R"(c)" }                                    },
    };

    for (auto const& t : tests)
    {
        CAPTURE(t.inp);
        auto args = cl::TokenizeWindows(t.inp, cl::ParseProgramName::yes);
        CHECK(args == t.out);
    }
}

TEST_CASE("Tokenize Windows 2")
{
    bool a = false;
    bool b = false;
    bool c = false;
    bool d = false;

    cl::Cmdline cl;
    cl.Add("a", "<descr>", cl::Var(a), cl::NumOpts::zero_or_more, cl::HasArg::no,       cl::MayGroup::yes);
    cl.Add("b", "<descr>", cl::Var(b), cl::NumOpts::zero_or_more, cl::HasArg::optional, cl::MayGroup::yes);
    cl.Add("c", "<descr>", cl::Var(c), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::MayGroup::yes);
    cl.Add("d", "<descr>", cl::Var(d), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::JoinArg::yes);

    char const* command_line
        = "-a --b -b -b=true -b=0 -b=on -c          false -c=0 -c=1 -c=true -c=false -c=on --c=off -c=yes --c=no -ac true -ab -ab=true -dtrue -dno -d1";

    CHECK(true == cl.ParseArgs(cl::TokenizeWindows(command_line, cl::ParseProgramName::yes)));
    cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == true);
}

static auto Flag(bool& target)
{
    return [&](cl::ParseContext const& ctx) {
        target = ctx.name.substr(0, 3) != "no-";
        return true;
    };
}

TEST_CASE("Invertible flag")
{
    bool a = false;
    bool b = false;

    cl::Cmdline cl;
    cl.Add("a|no-a", "bool", Flag(a), cl::NumOpts::zero_or_more, cl::HasArg::no);
    cl.Add("b|no-b", "bool", Flag(b), cl::NumOpts::zero_or_more, cl::HasArg::no);

    CHECK(true == ParseArgs(cl, {}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(true == ParseArgs(cl, {"-a", "-b"}));
    CHECK(a == true);
    CHECK(b == true);
    CHECK(true == ParseArgs(cl, {"-no-a", "-no-b"}));
    CHECK(a == false);
    CHECK(b == false);
}

TEST_CASE("Ex 1")
{
    // TODO:
    // Customize PrintHelp with some formatting... tabs, paragraphs etc...?!?!

    enum class OptimizationLevel { O0, O1, O2, O3, Os };

    OptimizationLevel optlevel = OptimizationLevel::O0;

    cl::Cmdline cl;
    cl.Add(
        "O0|O1|O2|O3|Os",
"Optimization level\r\n"
"  -O0  No optimization (the default); generates unoptimized code but has the fastest compilation time.\r\n"
"  -O1  Moderate optimization; optimizes reasonably well but does not degrade compilation time significantly.\r\n"
"  -O2  \tFull optimization; generates highly optimized code and has the slowest compilation time.\r\n"
"  -O3  \tFull optimization as in -O2; also uses more aggressive automatic inlining of subprograms within a unit (Inlining of Subprograms) and attempts to vectorize loops.\r\n"
"  -Os  \tOptimize space usage (code and data) of resulting program.",
        cl::Map(optlevel, {{"O0", OptimizationLevel::O0},
                           {"O1", OptimizationLevel::O1},
                           {"O2", OptimizationLevel::O2},
                           {"O3", OptimizationLevel::O3},
                           {"Os", OptimizationLevel::Os}}),
        cl::HasArg::no,
        cl::NumOpts::required
        );

    CHECK(false == ParseArgs(cl, {}));
    cl.PrintDiag();

    cl::Cmdline::HelpFormat fmt;
    fmt.indent = 2;
    fmt.descr_indent = 8;
    fmt.line_length = 56;

    cl.PrintHelp("compiler", fmt);
}

TEST_CASE("Ex 2")
{
    // TODO:
    // Customize PrintHelp with some formatting... tabs, paragraphs etc...?!?!

    enum class OptimizationLevel { O0, O1, O2, O3, Os };

    OptimizationLevel optlevel = OptimizationLevel::O0;
    std::string input = "-";

    cl::Cmdline cl;
    cl.Add(
        "O",
"Optimization level\n"
" -OptimzationLevel0000000\tNo optimization (the default); generates unoptimized code but has the fastest compilation time.\n"
"\t  -O1  Moderate optimization; optimizes reasonably well but does not degrade compilation time significantly.\n"
"  -O2  \tFull optimization; generates highly optimized code and has the slowest compilation time.\n"
"  -O3  \tFull optimization as in -O2; also uses more aggressive automatic inlining of subprograms within a unit (Inlining of Subprograms) and attempts to vectorize loops.\n"
"  -Os  \tOptimize space usage (code and data) of resulting program.",
        [&](cl::ParseContext const& ctx) {
            if (ctx.arg == "0")
                optlevel = OptimizationLevel::O0;
            else if (ctx.arg == "1")
                optlevel = OptimizationLevel::O1;
            else if (ctx.arg == "2")
                optlevel = OptimizationLevel::O2;
            else if (ctx.arg == "3")
                optlevel = OptimizationLevel::O3;
            else if (ctx.arg == "s")
                optlevel = OptimizationLevel::Os;
            else {
                ctx.cmdline->FormatDiag(cl::Diagnostic::error, ctx.index, "invalid argument '%.*s' for option -O<optlevel>", static_cast<int>(ctx.arg.size()), ctx.arg.data());
                ctx.cmdline->FormatDiag(cl::Diagnostic::note, ctx.index, "%s", "<optlevel> must be 0,1,2,3, or s");
                return false;
            }

            return true;
        },
        cl::JoinArg::yes,
        cl::HasArg::yes,
        cl::NumOpts::required
        );
    cl.Add("input", "input file", cl::Var(input), cl::NumOpts::optional, cl::Positional::yes);

    CHECK(false == ParseArgs(cl, {"-Oinf"}));
    cl.PrintDiag();
    cl.Reset();
    CHECK(false == ParseArgs(cl, {"-O", "1"}));
    cl.PrintDiag();
    cl.Reset();
    CHECK(true == ParseArgs(cl, {"-Os"}));

    cl::Cmdline::HelpFormat fmt;
    fmt.indent = 2;
    fmt.descr_indent = 8;
    fmt.line_length = 20;

    cl.PrintHelp("compiler", fmt);
}

TEST_CASE("custom check")
{
    // Returns a function object which checks whether a given value is in the range [lower, upper].
    auto Clamp = [](auto const& lower, auto const& upper)
    {
        using namespace cl;

        return [=](ParseContext const& ctx, auto& value) {
            if (value < lower)
            {
                ctx.cmdline->EmitDiag(Diagnostic::warning, ctx.index,
                    "argument '" + std::to_string(value) + "' is out of range. using lower bound '" + std::to_string(lower) + "'");
                value = lower;
            }
            else if (upper < value)
            {
                ctx.cmdline->EmitDiag(Diagnostic::warning, ctx.index,
                    "argument '" + std::to_string(value) + "' is out of range. using upper bound '" + std::to_string(upper) + "'");
                value = upper;
            }
            return true;
        };
    };

    int i = 0;

    cl::Cmdline cli;
    cli.Add("i", "int", cl::Var(i, Clamp(-3,3)), cl::NumOpts::required, cl::HasArg::yes);

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "1"}));
    CHECK(i == 1);

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "1000"}));
    CHECK(i == 3);
    CHECK(!cli.diag().empty());
    cli.PrintDiag();

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "-1000"}));
    CHECK(i == -3);
    CHECK(!cli.diag().empty());
    cli.PrintDiag();

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "3"}));
    CHECK(i == 3);
    CHECK(cli.diag().empty());
}

TEST_CASE("Invalid UTF-8")
{
    // Test cases from:
    // https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

    struct Test {
        std::string input;
        int num_replacements; // 0: ignore
    };

    static const Test tests[] = {
// 3.1  Unexpected continuation bytes
// Each unexpected continuation byte should be separately signalled as a
// malformed sequence of its own.
//
// XXX: The UTF decoder in namespace cl handles all contiguous sequences of trail bytes as a
//      single invalid UTF-8 sequence
//
        {"\x80", 1},                          // 3.1.1  First continuation byte 0x80
        {"\xBF", 1},                          // 3.1.2  Last continuation byte 0xbf
        {"\x80\xBF", 1},                      // 3.1.3  2 continuation bytes
        {"\x80\xBF\x80", 1},                  // 3.1.4  3 continuation bytes
        {"\x80\xBF\x80\xBF", 1},              // 3.1.5  4 continuation bytes
        {"\x80\xBF\x80\xBF\x80", 1},          // 3.1.6  5 continuation bytes
        {"\x80\xBF\x80\xBF\x80\xBF", 1},      // 3.1.7  6 continuation bytes
        {"\x80\xBF\x80\xBF\x80\xBF\x80", 1},  // 3.1.8  7 continuation bytes
        // 3.1.9  Sequence of all 64 possible continuation bytes (0x80-0xbf)
        {"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
         "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
         "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
         "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF", 1},

// 3.2  Lonely start characters
        // 3.2.1  All 32 first bytes of 2-byte sequences (0xc0-0xdf)
        {"\xC0", 1},
        {"\xC1", 1},
        {"\xC2", 1},
        {"\xC3", 1},
        {"\xC4", 1},
        {"\xC5", 1},
        {"\xC6", 1},
        {"\xC7", 1},
        {"\xC8", 1},
        {"\xC9", 1},
        {"\xCA", 1},
        {"\xCB", 1},
        {"\xCC", 1},
        {"\xCD", 1},
        {"\xCE", 1},
        {"\xCF", 1},
        {"\xD0", 1},
        {"\xD1", 1},
        {"\xD2", 1},
        {"\xD3", 1},
        {"\xD4", 1},
        {"\xD5", 1},
        {"\xD6", 1},
        {"\xD7", 1},
        {"\xD8", 1},
        {"\xD9", 1},
        {"\xDA", 1},
        {"\xDB", 1},
        {"\xDC", 1},
        {"\xDD", 1},
        {"\xDE", 1},
        {"\xDF", 1},
        // 3.2.2  All 16 first bytes of 3-byte sequences (0xe0-0xef)
        {"\xE0", 1},
        {"\xE1", 1},
        {"\xE2", 1},
        {"\xE3", 1},
        {"\xE4", 1},
        {"\xE5", 1},
        {"\xE6", 1},
        {"\xE7", 1},
        {"\xE8", 1},
        {"\xE9", 1},
        {"\xEA", 1},
        {"\xEB", 1},
        {"\xEC", 1},
        {"\xED", 1},
        {"\xEE", 1},
        {"\xEF", 1},
        // 3.2.3  All 8 first bytes of 4-byte sequences (0xf0-0xf7)
        {"\xF0", 1},
        {"\xF1", 1},
        {"\xF2", 1},
        {"\xF3", 1},
        {"\xF4", 1},
        {"\xF5", 1},
        {"\xF6", 1},
        {"\xF7", 1},
        // 3.2.4  All 4 first bytes of 5-byte sequences (0xf8-0xfb)
        {"\xF8", 1},
        {"\xF9", 1},
        {"\xFA", 1},
        {"\xFB", 1},
        // 3.2.5  All 2 first bytes of 6-byte sequences (0xfc-0xfd)
        {"\xFC", 1},
        {"\xFD", 1},

// 3.3  Sequences with last continuation byte missing
// All bytes of an incomplete sequence should be signalled as a single
// malformed sequence, i.e., you should see only a single replacement
// character in each of the next 10 tests.
        {"\xC0", 1},                 // 2-byte sequence with last byte missing (U+0000)
        {"\xE0\x80", 1},             // 3-byte sequence with last byte missing (U+0000)
        {"\xF0\x80\x80", 1},         // 4-byte sequence with last byte missing (U+0000)
        {"\xF8\x80\x80\x80", 1},     // 5-byte sequence with last byte missing (U+0000)
        {"\xFC\x80\x80\x80\x80", 1}, // 6-byte sequence with last byte missing (U+0000)
        {"\xDF", 1},                 // 2-byte sequence with last byte missing (U-000007FF)
        {"\xEF\xBF", 1},             // 3-byte sequence with last byte missing (U-0000FFFF)
        {"\xF7\xBF\xBF", 1},         // 4-byte sequence with last byte missing (U-001FFFFF)
        {"\xFB\xBF\xBF\xBF", 1},     // 5-byte sequence with last byte missing (U-03FFFFFF)
        {"\xFD\xBF\xBF\xBF\xBF", 1}, // 6-byte sequence with last byte missing (U-7FFFFFFF)

// 3.4  Concatenation of incomplete sequences
// All the 10 sequences of 3.3 concatenated, you should see 10 malformed
// sequences being signalled
#if 0
        {"\xC0"
         "\xE0\x80"
         "\xF0\x80\x80"
         "\xDF"
         "\xEF\xBF"
         "\xF7\xBF\xBF", 6},
        {"\xC0"
         "\xE0\x80"
         "\xF0\x80\x80"
         "\xF8\x80\x80\x80"
         "\xFC\x80\x80\x80\x80"
         "\xDF"
         "\xEF\xBF"
         "\xF7\xBF\xBF"
         "\xFB\xBF\xBF\xBF"
         "\xFD\xBF\xBF\xBF\xBF", 10},
#endif

// 3.5  Impossible bytes
        {"\xFE", 1},
        {"\xFF", 1},
        {"\xFE\xFE\xFF\xFF", 4},

// 4.1  Examples of an overlong ASCII character
        {"\xC0\xAF", 1},                 // U+002F
        {"\xE0\x80\xAF", 1},             // U+002F
        {"\xF0\x80\x80\xAF", 1},         // U+002F
        {"\xF8\x80\x80\x80\xAF", 1},     // U+002F
        {"\xFC\x80\x80\x80\x80\xAF", 1}, // U+002F

// 4.2  Maximum overlong sequences
        {"\xC1\xBF", 1},                 // U-0000007F
        {"\xE0\x9F\xBF", 1},             // U-000007FF
        {"\xF0\x8F\xBF\xBF", 1},         // U-0000FFFF
        {"\xF8\x87\xBF\xBF\xBF", 1},     // U-001FFFFF
        {"\xFC\x83\xBF\xBF\xBF\xBF", 1}, // U-03FFFFFF

// 4.3  Overlong representation of the NUL character
        {"\xC0\x80", 1},                 // U+0000
        {"\xE0\x80\x80", 1},             // U+0000
        {"\xF0\x80\x80\x80", 1},         // U+0000
        {"\xF8\x80\x80\x80\x80", 1},     // U+0000
        {"\xFC\x80\x80\x80\x80\x80", 1}, // U+0000

// 5.1 Single UTF-16 surrogates
        {"\xED\xA0\x80", 1}, // U+D800
        {"\xED\xAD\xBF", 1}, // U+DB7F
        {"\xED\xAE\x80", 1}, // U+DB80
        {"\xED\xAF\xBF", 1}, // U+DBFF
        {"\xED\xB0\x80", 1}, // U+DC00
        {"\xED\xBE\x80", 1}, // U+DF80
        {"\xED\xBF\xBF", 1}, // U+DFFF

// 5.2 Paired UTF-16 surrogates
        {"\xED\xA0\x80\xED\xB0\x80", 2}, // U+D800 U+DC00
        {"\xED\xA0\x80\xED\xBF\xBF", 2}, // U+D800 U+DFFF
        {"\xED\xAD\xBF\xED\xB0\x80", 2}, // U+DB7F U+DC00
        {"\xED\xAD\xBF\xED\xBF\xBF", 2}, // U+DB7F U+DFFF
        {"\xED\xAE\x80\xED\xB0\x80", 2}, // U+DB80 U+DC00
        {"\xED\xAE\x80\xED\xBF\xBF", 2}, // U+DB80 U+DFFF
        {"\xED\xAF\xBF\xED\xB0\x80", 2}, // U+DBFF U+DC00
        {"\xED\xAF\xBF\xED\xBF\xBF", 2}, // U+DBFF U+DFFF
    };

    auto CountInvalidUTF8Sequences = [](std::string const& in) {
        int count = 0;
        cl::impl::ForEachUTF8EncodedCodepoint(in.begin(), in.end(), [&](uint32_t U) {
            if (U == cl::impl::kInvalidCodepoint)
                ++count;
            return true;
        });
        return count;
    };

    int i = 0;
    for (auto const& test : tests)
    {
        int const num_invalid = CountInvalidUTF8Sequences(test.input);
        CAPTURE(i);
        CAPTURE(test.input);
        CHECK(test.num_replacements == num_invalid);
        ++i;
    }
}
