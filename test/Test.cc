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
#endif
#if 0
    std::string operator*() const { return std::string(*it); }
#endif

    fancy_iterator& operator++() {
        ++it;
        return *this;
    }

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
    if (cl.Parse(first, last))
        return true;

    cl.PrintDiag();
    return false;
#else
    return cl.Parse(first, last);
#endif
}

TEST_CASE("Opt")
{
    SECTION("optional")
    {
        bool a = false;

        cl::Cmdline cl;
        cl.Add("a", "", cl::Assign(a)); // 'NumOpts::optional' is the default

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::one_or_more);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more); // 'arg_disallowed' is the default

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::optional);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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

    cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::no);
    cl.Add("b", "", cl::Assign(b), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::no);
    cl.Add("c", "", cl::Assign(c), cl::NumOpts::zero_or_more, cl::MayGroup::yes, cl::HasArg::required);

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

    cl.Add("ab", "", cl::Assign(ab), cl::NumOpts::zero_or_more, cl::HasArg::no);

    CHECK(true == ParseArgs(cl, {"-ab", "--ab"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ab == true);
    ab = false;

    cl.Add("ac", "", cl::Assign(ac), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {"-ac=true"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ac == true);
    ac = false;

    cl.Add("abc", "", cl::Assign(abc), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::PushBack(strings), cl::Positional::yes, cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::PushBack(ints), cl::Positional::yes, cl::NumOpts::one_or_more, cl::HasArg::required);

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
    cl.Add("a", "", cl::PushBack(ints), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::CommaSeparated::yes);

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

TEST_CASE("EndsOptions")
{
    std::vector<std::string> sink;
    bool a = false;

    cl::Cmdline cl;
    cl.Add("a", "", cl::Assign(a), cl::EndsOptions::yes, cl::HasArg::required);
    cl.Add("!", "", cl::PushBack(sink), cl::Positional::yes, cl::NumOpts::zero_or_more);

    CHECK(true == ParseArgs(cl, {"eins", "zwei"}));
    CHECK(sink.size() == 2);
    CHECK(sink[0] == "eins");
    CHECK(sink[1] == "zwei");
    CHECK(false == ParseArgs(cl, {"-123"})); // unknown option
    CHECK(true == ParseArgs(cl, {"drei", "-a", "true", "-123"}));
    CHECK(a == true);
    CHECK(sink.size() == 4);
    CHECK(sink[0] == "eins");
    CHECK(sink[1] == "zwei");
    CHECK(sink[2] == "drei");
    CHECK(sink[3] == "-123");
    CHECK(true == ParseArgs(cl, {"-a", "false"})); // option parsing disabled now...
    CHECK(sink.size() == 6);
    CHECK(sink[4] == "-a");
    CHECK(sink[5] == "false");
}

TEST_CASE("Strings")
{
    std::string str;

    cl::Cmdline cl;
    cl.Add("s", "", cl::Assign(str), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a, cl::check::InRange(-3, 3)), cl::NumOpts::zero_or_more, cl::HasArg::required);
        cl.Add("b", "", cl::Assign(b, cl::check::InRange(INT32_MIN, INT32_MAX)), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a, cl::check::GreaterThan(-3)), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a, cl::check::GreaterEqual(7)), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a, cl::check::LessThan(-3)), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
        cl.Add("a", "", cl::Assign(a, cl::check::LessEqual(INT32_MIN)), cl::NumOpts::zero_or_more, cl::HasArg::required);

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
    cl.Add(u8"😃-😜", "", cl::Assign(str), cl::NumOpts::zero_or_more, cl::HasArg::required);

    CHECK(true == ParseArgs(cl, {u8"-😃-😜=hello😍😎world"}));
    CHECK(str == u8"hello😍😎world");
}

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
        auto args = cl::TokenizeWindows(t.inp, /*parse_program_name*/ true);
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
    cl.Add("a", "<descr>", cl::Assign(a), cl::NumOpts::zero_or_more, cl::HasArg::no,       cl::MayGroup::yes);
    cl.Add("b", "<descr>", cl::Assign(b), cl::NumOpts::zero_or_more, cl::HasArg::optional, cl::MayGroup::yes);
    cl.Add("c", "<descr>", cl::Assign(c), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::MayGroup::yes);
    cl.Add("d", "<descr>", cl::Assign(d), cl::NumOpts::zero_or_more, cl::HasArg::required, cl::JoinArg::yes);

    char const* command_line
        = "-a --b -b -b=true -b=0 -b=on -c          false -c=0 -c=1 -c=true -c=false -c=on --c=off -c=yes --c=no -ac true -ab -ab=true -dtrue -dno -d1";

    CHECK(true == cl.ParseArgs(cl::TokenizeWindows(command_line, /*parse_program_name*/ false)));
    cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == true);
}
