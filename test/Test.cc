#define CL_WINDOWS_CONSOLE_COLORS 1
#define CL_ANSI_CONSOLE_COLORS 1
#include "Cmdline.h"

#include <cstdint>
#include <climits>

#define CATCH_CONFIG_MAIN
#undef min
#undef max
#include "catch.hpp"

template <typename CharT>
struct fancy_iterator
{
    using It                = typename std::initializer_list<CharT const*>::iterator;
    using iterator_category = std::input_iterator_tag;
//  using iterator_category = typename std::iterator_traits<It>::iterator_category;
    using reference         = typename std::iterator_traits<It>::reference;
    using pointer           = typename std::iterator_traits<It>::pointer;
    using value_type        = typename std::iterator_traits<It>::value_type;
    using difference_type   = typename std::iterator_traits<It>::difference_type;

    It it;

    fancy_iterator() = default;
    explicit fancy_iterator(It it_) : it(it_) {}

#if 0
    CharT const* operator*() { return *it; }
    fancy_iterator& operator++() {
        ++it;
        return *this;
    }
#endif
#if 1
    std::basic_string<CharT> operator*() const
    {
        std::basic_string<CharT> s(*it);
        size_t k = s.size();
        s.append(200, CharT('~'));
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
template <typename CharT = char>
static bool ParseArgs(cl::Cmdline& cl, std::initializer_list<CharT const*> args)
{
    fancy_iterator<CharT> first{args.begin()};
    fancy_iterator<CharT> last {args.end()};

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", {}, cl::Var(a)); // 'Required::no' is the default

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Required::yes, cl::Var(a));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes, cl::Var(a));

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

        cl::Cmdline cl("test", "test");
#if CL_HAS_DEDUCTION_GUIDES
        cl::Option opt_a("a", "", cl::Required::yes | cl::Multiple::yes, cl::Var(a));
        cl.Add(&opt_a);
#else
        cl.Add("a", "", cl::Required::yes | cl::Multiple::yes, cl::Var(a));
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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes, cl::Var(a)); // 'Arg::no' is the default

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::optional, cl::Var(a));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::yes, cl::Var(a));

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

    cl::Cmdline cl("test", "test");

    cl.Add("a", "", cl::Multiple::yes | cl::MayGroup::yes | cl::Arg::no,       cl::Var(a));
    cl.Add("b", "", cl::Multiple::yes | cl::MayGroup::yes | cl::Arg::no,       cl::Var(b));
    cl.Add("c", "", cl::Multiple::yes | cl::MayGroup::yes | cl::Arg::required, cl::Var(c));

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
    CHECK(false == ParseArgs(cl, {"-cba"})); // c must not join its argument
    CHECK(a == false);
    CHECK(b == false);
    CHECK(c == false);
    //cl.Reset();
    CHECK(false == ParseArgs(cl, {"-abcab=true"})); // c must not join its argument
    //cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == false);

    cl.Add("ab", "", cl::Multiple::yes | cl::Arg::no, cl::Var(ab));

    a = false;
    b = false;
    CHECK(true == ParseArgs(cl, {"-ab", "--ab"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ab == true);
    ab = false;

    cl.Add("ac", "", cl::Multiple::yes | cl::Arg::required, cl::Var(ac));

    CHECK(true == ParseArgs(cl, {"-ac=true"}));
    CHECK(a == false);
    CHECK(b == false);
    CHECK(ac == true);
    ac = false;

    cl.Add("abc", "", cl::Multiple::yes | cl::Arg::required, cl::Var(abc));

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

    //cl.Reset();
    CHECK(false == ParseArgs(cl, {"-aa=true"})); // a does not accept an argument
    CHECK(a == true); // The first a has been parsed
    //cl.PrintDiag();
    a = false;

    CHECK(false == ParseArgs(cl, {"-="}));
}

TEST_CASE("Positional")
{
    SECTION("strings")
    {
        std::vector<std::string> strings;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Positional::yes | cl::Multiple::yes | cl::Arg::required, cl::Var(strings));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Positional::yes | cl::Required::yes | cl::Multiple::yes | cl::Arg::required, cl::Var(ints));

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

    cl::Cmdline cl("test", "test");
    cl.Add("a", "", cl::Multiple::yes | cl::Arg::required | cl::CommaSeparated::yes, cl::Var(ints));

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

    cl::Cmdline cli("test", "test");
    cli.Add("a", "", cl::Arg::optional, cl::Var(a));
    cli.Add("command", "", cl::Positional::yes | cl::Arg::yes | cl::StopParsing::yes | cl::Required::yes, cl::Var(command));

    SECTION("1")
    {
        a = -1;

        cli.Reset();
        auto const args = {"-a=123", "command", "-b", "-c"};
        auto const res = cli.Parse(args.begin(), args.end(), cl::CheckMissingOptions::yes);
        CHECK(res.success);
        CHECK(res.next == args.begin() + 2);
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
        CHECK(res.next == args.begin() + 1);
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

    cl::Cmdline cl("test", "test");
    cl.Add("s", "", cl::Multiple::yes | cl::Arg::required, cl::Var(str));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "127"}));
        CHECK(a == 127);
        CHECK(true == ParseArgs(cl, {"-a", "-128"}));
        CHECK(a == -128);
        CHECK(false == ParseArgs(cl, {"-a", "128"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "-129"})); // overflow
    }

    SECTION("decimal unsigned 8-bit")
    {
        uint8_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "127"}));
        CHECK(a == 127);
        CHECK(false == ParseArgs(cl, {"-a", "-127"}));
        CHECK(a == 127);
        CHECK(false == ParseArgs(cl, {"-a", "-128"}));
        CHECK(a == 127);
        CHECK(false == ParseArgs(cl, {"-a", "-255"}));
        CHECK(a == 127);
        a = 0;
        CHECK(true == ParseArgs(cl, {"-a", "128"}));
        CHECK(a == 128);
        CHECK(true == ParseArgs(cl, {"-a", "255"}));
        CHECK(a == 255);
        a = 0;
        CHECK(false == ParseArgs(cl, {"-a", "256"}));
        CHECK(a == 0);
    }

    SECTION("decimal signed 32-bit")
    {
        int32_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    +0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    -0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +  0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -  0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +   "}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -   "}));
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
        CHECK(false == ParseArgs(cl, {"-a", "214748364F"})); //invalid digit
        CHECK(false == ParseArgs(cl, {"-a", "F147483647"})); //invalid digit
    }

    SECTION("decimal unsigned 32-bit")
    {
        uint32_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    +0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "    -0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +  0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -  0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +   "}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  -   "}));
        CHECK(a == 0);
        //CHECK(true == ParseArgs(cl, {"-a", "123   "}));
        //CHECK(a == 123);
        //CHECK(true == ParseArgs(cl, {"-a", "321 "}));
        //CHECK(a == 321);
        a = 0;
        CHECK(false == ParseArgs(cl, {"-a", "-123"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "          -2147483648"}));
        CHECK(a == INT32_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "          +2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "2147483648"}));
        CHECK(a == 2147483648);
        a = 0;
        CHECK(true == ParseArgs(cl, {"-a", "+2147483648"}));
        CHECK(a == 2147483648);
        CHECK(true == ParseArgs(cl, {"-a", "+4294967295"}));
        CHECK(a == 4294967295);
        CHECK(false == ParseArgs(cl, {"-a", "4294967296"})); // overflow
        CHECK(a == 4294967295);
        CHECK(false == ParseArgs(cl, {"-a", "true"})); // invalid argument
        CHECK(false == ParseArgs(cl, {"-a", "hello"})); // invalid argument
    }

    SECTION("decimal signed 64-bit")
    {
        int64_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    0"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "    +0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +  0"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +"}));
        CHECK(a == 0);
        CHECK(false == ParseArgs(cl, {"-a", "  +   "}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "9223372036854775807"}));
        CHECK(a == INT64_MAX);
        CHECK(true == ParseArgs(cl, {"-a", "-9223372036854775808"}));
        CHECK(a == INT64_MIN);
        CHECK(true == ParseArgs(cl, {"-a", "+9223372036854775807"}));
        CHECK(a == INT64_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "-9223372036854775809"})); // overflow
        CHECK(false == ParseArgs(cl, {"-a", "9223372036854775808"})); // overflow
    }

    SECTION("decimal unsigned 64-bit")
    {
        uint64_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

        CHECK(true == ParseArgs(cl, {"-a", "9223372036854775807"}));
        CHECK(a == INT64_MAX);
        a = 0;
        CHECK(false == ParseArgs(cl, {"-a", "-9223372036854775808"}));
        CHECK(a == 0);
        CHECK(true == ParseArgs(cl, {"-a", "+9223372036854775807"}));
        CHECK(a == INT64_MAX);
        CHECK(false == ParseArgs(cl, {"-a", "-9223372036854775809"}));
        CHECK(true == ParseArgs(cl, {"-a", "9223372036854775808"}));
        CHECK(a == 9223372036854775808ull);
        CHECK(true == ParseArgs(cl, {"-a", "18446744073709551615"}));
        CHECK(a == 18446744073709551615ull);
        a = 0;
        CHECK(false == ParseArgs(cl, {"-a", "18446744073709551616"})); // overflow
        CHECK(a == 0);
    }

    //
    // TODO:
    // Implement two's complement parser for integers?!?!
    //

    SECTION("hexadecimal signed 32-bit")
    {
        int32_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

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
        CHECK(false == ParseArgs(cl, {"-a", "+0x7FFFFFFZ"})); // invalid digit
        CHECK(false == ParseArgs(cl, {"-a", "+0x7ZFFFFFF"})); // invalid digit
    }

    SECTION("octal signed 8-bit")
    {
        int8_t a = -1;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a));

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
        CHECK(false == ParseArgs(cl, {"-a", "0178"})); // invalid digit
        CHECK(false == ParseArgs(cl, {"-a", "0877"})); // invalid digit
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

    cl::Cmdline cl("test", "test");
    cl.Add("simpson", "<descr>",
        cl::Multiple::yes | cl::Arg::required,
        cl::Map(simpson, {{ "Homer",    Simpson::Homer  },
                          { "Marge",    Simpson::Marge  },
                          { "Bart",     Simpson::Bart   },
                          { "El Barto", Simpson::Bart   },
                          { "Lisa",     Simpson::Lisa   },
                          { "Maggie",   Simpson::Maggie }}));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a, cl::check::InRange(-3, 3)));
        cl.Add("b", "", cl::Multiple::yes | cl::Arg::required, cl::Var(b, cl::check::InRange(INT32_MIN, INT32_MAX)));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a, cl::check::GreaterThan(-3)));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a, cl::check::GreaterEqual(7)));

        CHECK(true == ParseArgs(cl, {"-a=7"}));
        CHECK(a == 7);
        CHECK(true == ParseArgs(cl, {"-a=2147483647"}));
        CHECK(a == INT32_MAX);
        CHECK(false == ParseArgs(cl, {"-a=6"}));
    }

    SECTION("LessThan")
    {
        int32_t a = 0;

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a, cl::check::LessThan(-3)));

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

        cl::Cmdline cl("test", "test");
        cl.Add("a", "", cl::Multiple::yes | cl::Arg::required, cl::Var(a, cl::check::LessEqual(INT32_MIN)));

        CHECK(true == ParseArgs(cl, {"-a=-2147483648"}));
        CHECK(a == INT32_MIN);
        CHECK(false == ParseArgs(cl, {"-a=-2147483647"}));
        CHECK(false == ParseArgs(cl, {"-a=+2147483647"}));
    }
}

TEST_CASE("Unicode")
{
    std::string str;

    cl::Cmdline cl("test", "test");
    cl.Add(u8"😃-😜", "", cl::Multiple::yes | cl::Arg::required, cl::Var(str));

    CHECK(true == ParseArgs(cl, {u8"-😃-😜=hello😍😎world"}));
    CHECK(str == u8"hello😍😎world");
}

TEST_CASE("Wide strings")
{
    std::wstring str;

    cl::Cmdline cl("test", "test");
    cl.Add(u8"😃-😜", "", cl::Multiple::yes | cl::Arg::required, cl::Var(str));

    CHECK(true == ParseArgs(cl, {u8"-😃-😜=hello😍😎world"}));
    CHECK(str == L"hello😍😎world");
    CHECK(true == ParseArgs(cl, {u8"-😃-😜=-😃-😜"}));
    CHECK(str == L"-😃-😜");
}

TEST_CASE("Unicode up-down")
{
    auto up_down_bool_parser = [](bool& value)
    {
        return [&value](cl::ParseContext const& ctx) {
            if (ctx.arg.empty() || ctx.arg == u8"👍")
                value = true;
            else if (ctx.arg == u8"👎")
                value = false;
            else
                return false;
            return true;
        };
    };

    bool value;

    cl::Cmdline cli("test", "test");
    cli.Add("b", "up-down", cl::Multiple::yes | cl::Arg::optional, up_down_bool_parser(value));

    CHECK(true == ParseArgs(cli, {u8"-b"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {u8"-b=👍"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {u8"-b=👎"}));
    CHECK(false == value);
    CHECK(true == ParseArgs(cli, {L"-b"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {L"-b=👍"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {L"-b=👎"}));
    CHECK(false == value);
    CHECK(true == ParseArgs(cli, {u"-b"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {u"-b=👍"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {u"-b=👎"}));
    CHECK(false == value);
    CHECK(true == ParseArgs(cli, {U"-b"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {U"-b=👍"}));
    CHECK(true == value);
    CHECK(true == ParseArgs(cli, {U"-b=👎"}));
    CHECK(false == value);
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

    cl::Cmdline cl("test", "test");
    cl.Add("a", "<descr>", cl::Multiple::yes | cl::Arg::no |       cl::MayGroup::yes, cl::Var(a));
    cl.Add("b", "<descr>", cl::Multiple::yes | cl::Arg::optional | cl::MayGroup::yes, cl::Var(b));
    cl.Add("c", "<descr>", cl::Multiple::yes | cl::Arg::required | cl::MayGroup::yes, cl::Var(c));

    char const* command_line
        = "-a --b -b -b=true -b=0 -b=on -c          false -c=0 -c=1 -c=true -c=false -c=on --c=off -c=yes --c=no -ac true -ab -ab=true";

    CHECK(true == cl.ParseArgs(cl::TokenizeWindows(command_line, cl::ParseProgramName::no)));
    cl.PrintDiag();
    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
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

    cl::Cmdline cl("test", "test");
    cl.Add("a|no-a", "bool", cl::Multiple::yes | cl::Arg::no, Flag(a));
    cl.Add("b|no-b", "bool", cl::Multiple::yes | cl::Arg::no, Flag(b));

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

#if 0
TEST_CASE("Ex 1")
{
    // TODO:
    // Customize PrintHelp with some formatting... tabs, paragraphs etc...?!?!

    enum class OptimizationLevel { O0, O1, O2, O3, Os };

    OptimizationLevel optlevel = OptimizationLevel::O0;

    cl::Cmdline cl("compiler", "test");
    cl.Add(
        "O0|O1|O2|O3|Os",
        "Optimization level\r\n"
        "  -O0  No optimization (the default); generates unoptimized code but has the fastest compilation time.\r\n"
        "  -O1  Moderate optimization; optimizes reasonably well but does not degrade compilation time significantly.\r\n"
        "  -O2  \tFull optimization; generates highly optimized code and has the slowest compilation time.\r\n"
        "  -O3  \tFull optimization as in -O2; also uses more aggressive automatic inlining of subprograms within a unit (Inlining of Subprograms) and attempts to vectorize loops.\r\n"
        "  -Os  \tOptimize space usage (code and data) of resulting program.",
        cl::Arg::no | cl::Required::yes,
        cl::Map(optlevel, {{"O0", OptimizationLevel::O0},
                           {"O1", OptimizationLevel::O1},
                           {"O2", OptimizationLevel::O2},
                           {"O3", OptimizationLevel::O3},
                           {"Os", OptimizationLevel::Os}})
        );

    CHECK(false == ParseArgs(cl, {}));
    cl.PrintDiag();

    cl::Cmdline::HelpFormat fmt;
    fmt.indent = 2;
    fmt.descr_indent = 8;
    fmt.line_length = 56;

    cl.PrintHelp(fmt);
}

TEST_CASE("Ex 2")
{
    // TODO:
    // Customize PrintHelp with some formatting... tabs, paragraphs etc...?!?!

    enum class OptimizationLevel { O0, O1, O2, O3, Os };

    OptimizationLevel optlevel = OptimizationLevel::O0;
    std::string input = "-";

    cl::Cmdline cl("compiler", "test");
    cl.Add(
        "O",
        "Optimization level\n"
        " -OptimzationLevel0000000\tNo optimization (the default); generates unoptimized code but has the fastest compilation time.\n"
        "\t  -O1  Moderate optimization; optimizes reasonably well but does not degrade compilation time significantly.\n"
        "  -O2  \tFull optimization; generates highly optimized code and has the slowest compilation time.\n"
        "  -O3  \tFull optimization as in -O2; also uses more aggressive automatic inlining of subprograms within a unit (Inlining of Subprograms) and attempts to vectorize loops.\n"
        "  -Os  \tOptimize space usage (code and data) of resulting program.",
        cl::MayJoin::yes | cl::Arg::yes | cl::Required::yes,
        [&](cl::ParseContext const& ctx) {
            if (ctx.arg == "0") {
                optlevel = OptimizationLevel::O0;
            } else if (ctx.arg == "1") {
                optlevel = OptimizationLevel::O1;
            } else if (ctx.arg == "2") {
                optlevel = OptimizationLevel::O2;
            } else if (ctx.arg == "3") {
                optlevel = OptimizationLevel::O3;
            } else if (ctx.arg == "s") {
                optlevel = OptimizationLevel::Os;
            } else {
                ctx.cmdline->EmitDiag(cl::Diagnostic::error, ctx.index, "invalid argument '", ctx.arg, "' for option -O<optlevel>");
                ctx.cmdline->EmitDiag(cl::Diagnostic::note, ctx.index, "<optlevel> must be 0,1,2,3, or s");
                return false;
            }

            return true;
        }
        );
    cl.Add("input", "input file", cl::Required::no | cl::Positional::yes, cl::Var(input));

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

    cl.PrintHelp(fmt);
}
#endif

#if 0
TEST_CASE("custom check")
{
    // Returns a function object which checks whether a given value is in the range [lower, upper].
    auto Clamp = [](auto const& lower, auto const& upper)
    {
        using namespace cl;

        return [=](cl::ParseContext const& ctx, auto& value) {
            if (value < lower)
            {
                ctx.cmdline->EmitDiag(cl::Diagnostic::warning, ctx.index,
                    "argument '", std::to_string(value), "' is out of range. using lower bound '", std::to_string(lower), "'");
                value = lower;
            }
            else if (upper < value)
            {
                ctx.cmdline->EmitDiag(cl::Diagnostic::warning, ctx.index,
                    "argument '", std::to_string(value), "' is out of range. using upper bound '", std::to_string(upper), "'");
                value = upper;
            }
            return true;
        };
    };

    int i = 0;

    cl::Cmdline cli("test", "test");
    cli.Add("i", "int", cl::Required::yes | cl::Arg::yes, cl::Var(i, Clamp(-3,3)));

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "1"}));
    CHECK(i == 1);

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "1000"}));
    CHECK(i == 3);
    CHECK(!cli.Diag().empty());
    cli.PrintDiag();

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "-1000"}));
    CHECK(i == -3);
    CHECK(!cli.Diag().empty());
    cli.PrintDiag();

    cli.Reset();
    CHECK(true == ParseArgs(cli, {"-i", "3"}));
    CHECK(i == 3);
    CHECK(cli.Diag().empty());
}
#endif

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
        {"\x80", 1},                          // 3.1.1  First continuation byte 0x80
        {"\xBF", 1},                          // 3.1.2  Last continuation byte 0xbf
        {"\x80\xBF", 2},                      // 3.1.3  2 continuation bytes
        {"\x80\xBF\x80", 3},                  // 3.1.4  3 continuation bytes
        {"\x80\xBF\x80\xBF", 4},              // 3.1.5  4 continuation bytes
        {"\x80\xBF\x80\xBF\x80", 5},          // 3.1.6  5 continuation bytes
        {"\x80\xBF\x80\xBF\x80\xBF", 6},      // 3.1.7  6 continuation bytes
        {"\x80\xBF\x80\xBF\x80\xBF\x80", 7},  // 3.1.8  7 continuation bytes
        // 3.1.9  Sequence of all 64 possible continuation bytes (0x80-0xbf)
        {"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
         "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
         "\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
         "\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF", 64},

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
        // character in each of the next tests.
        {"\xC0", 1},                 // 2-byte sequence with last byte missing (U+0000)
        {"\xE0\x80", 2},             // 3-byte sequence with last byte missing (U+0000)
        {"\xF0\x80\x80", 3},         // 4-byte sequence with last byte missing (U+0000)

        {"\xE0\xA0", 1},             // 3-byte sequence with last byte missing (U+0000)
        {"\xF0\x90\x80", 1},         // 4-byte sequence with last byte missing (U+0000)

        {"\xDF", 1},                 // 2-byte sequence with last byte missing (U-000007FF)
        {"\xEF\xBF", 1},             // 3-byte sequence with last byte missing (U-0000FFFF)
        {"\xF7\xBF\xBF", 3},         // 4-byte sequence with last byte missing (U-001FFFFF)

        {"\xF4\x8F\xBF", 1},         // 4-byte sequence with last byte missing (U-001FFFFF)

        // 3.4  Concatenation of incomplete sequences
        // All the 9 sequences of 3.3 concatenated, you should see 14 malformed
        // sequences being signalled
        {"\xC0"
         "\xE0\x80"
         "\xF0\x80\x80"
         "\xDF"
         "\xEF\xBF"
         "\xF7\xBF\xBF"
         "\xE0\xA0"
         "\xF0\x90\x80"
         "\xF4\x8F\xBF", 14},

        // 3.5  Impossible bytes
        {"\xFE", 1},
        {"\xFF", 1},
        {"\xFE\xFE\xFF\xFF", 4},

        // 4.1  Examples of an overlong ASCII character
        {"\xC0\xAF", 2},                 // U+002F
        {"\xE0\x80\xAF", 3},             // U+002F
        {"\xF0\x80\x80\xAF", 4},         // U+002F
#if 1
        {"\xF8\x80\x80\x80\xAF", 5},     // U+002F
        {"\xFC\x80\x80\x80\x80\xAF", 6}, // U+002F
#endif

        // 4.2  Maximum overlong sequences
        {"\xC1\xBF", 2},                 // U-0000007F
        {"\xE0\x9F\xBF", 3},             // U-000007FF
        {"\xF0\x8F\xBF\xBF", 4},         // U-0000FFFF
#if 1
        {"\xF8\x87\xBF\xBF\xBF", 5},     // U-001FFFFF
        {"\xFC\x83\xBF\xBF\xBF\xBF", 6}, // U-03FFFFFF
#endif

        // 4.3  Overlong representation of the NUL character
        {"\xC0\x80", 2},                 // U+0000
        {"\xE0\x80\x80", 3},             // U+0000
        {"\xF0\x80\x80\x80", 4},         // U+0000
#if 1
        {"\xF8\x80\x80\x80\x80", 5},     // U+0000
        {"\xFC\x80\x80\x80\x80\x80", 6}, // U+0000
#endif

        // 5.1 Single UTF-16 surrogates
        {"\xED\xA0\x80", 3}, // U+D800
        {"\xED\xAD\xBF", 3}, // U+DB7F
        {"\xED\xAE\x80", 3}, // U+DB80
        {"\xED\xAF\xBF", 3}, // U+DBFF
        {"\xED\xB0\x80", 3}, // U+DC00
        {"\xED\xBE\x80", 3}, // U+DF80
        {"\xED\xBF\xBF", 3}, // U+DFFF

        // 5.2 Paired UTF-16 surrogates
        {"\xED\xA0\x80\xED\xB0\x80", 6}, // U+D800 U+DC00
        {"\xED\xA0\x80\xED\xBF\xBF", 6}, // U+D800 U+DFFF
        {"\xED\xAD\xBF\xED\xB0\x80", 6}, // U+DB7F U+DC00
        {"\xED\xAD\xBF\xED\xBF\xBF", 6}, // U+DB7F U+DFFF
        {"\xED\xAE\x80\xED\xB0\x80", 6}, // U+DB80 U+DC00
        {"\xED\xAE\x80\xED\xBF\xBF", 6}, // U+DB80 U+DFFF
        {"\xED\xAF\xBF\xED\xB0\x80", 6}, // U+DBFF U+DC00
        {"\xED\xAF\xBF\xED\xBF\xBF", 6}, // U+DBFF U+DFFF
    };

    auto CountInvalidUTF8Sequences = [](std::string const& in) {
        int count = 0;
        cl::impl::ForEachUTF8EncodedCodepoint(in.begin(), in.end(), [&](char32_t U) {
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

TEST_CASE("UTF-16")
{
    struct Test {
        std::u16string input;
        int num_replacements; // 0: ignore
    };

    static const Test tests[] = {
        {{u'X', (char16_t)0xD800}, 1},
        {{u'X', (char16_t)0xD900}, 1},
        {{u'X', (char16_t)0xDA00}, 1},
        {{u'X', (char16_t)0xDBFF}, 1},
        {{u'X', (char16_t)0xDC00}, 1},
        {{u'X', (char16_t)0xDD00}, 1},
        {{u'X', (char16_t)0xDE00}, 1},
        {{u'X', (char16_t)0xDFFF}, 1},

        {{u'X', (char16_t)0xD800, u'Y'}, 1},
        {{u'X', (char16_t)0xD900, u'Y'}, 1},
        {{u'X', (char16_t)0xDA00, u'Y'}, 1},
        {{u'X', (char16_t)0xDBFF, u'Y'}, 1},
        {{u'X', (char16_t)0xDC00, u'Y'}, 1},
        {{u'X', (char16_t)0xDD00, u'Y'}, 1},
        {{u'X', (char16_t)0xDE00, u'Y'}, 1},
        {{u'X', (char16_t)0xDFFF, u'Y'}, 1},

        {{u'X', (char16_t)0xD800, (char16_t)0xDBFF, u'Y'}, 2},
        {{u'X', (char16_t)0xD900, (char16_t)0xDA00, u'Y'}, 2},
        {{u'X', (char16_t)0xDA00, (char16_t)0xD900, u'Y'}, 2},
        {{u'X', (char16_t)0xDBFF, (char16_t)0xD800, u'Y'}, 2},
        {{u'X', (char16_t)0xDC00, (char16_t)0xDFFF, u'Y'}, 2},
        {{u'X', (char16_t)0xDD00, (char16_t)0xDE00, u'Y'}, 2},
        {{u'X', (char16_t)0xDE00, (char16_t)0xDD00, u'Y'}, 2},
        {{u'X', (char16_t)0xDFFF, (char16_t)0xDC00, u'Y'}, 2},

        {{u'X', (char16_t)0xD800, (char16_t)0xDC00, u'Y'}, 0},
        {{u'X', (char16_t)0xD800, (char16_t)0xDFFF, u'Y'}, 0},
        {{u'X', (char16_t)0xDBFF, (char16_t)0xDC00, u'Y'}, 0},
        {{u'X', (char16_t)0xDBFF, (char16_t)0xDFFF, u'Y'}, 0},
    };

    auto CountInvalidUTF16Sequences = [](std::u16string const& in) {
        int count = 0;
        cl::impl::ForEachUTF16EncodedCodepoint(in.begin(), in.end(), [&](char32_t U) {
            if (U == cl::impl::kInvalidCodepoint)
                ++count;
            return true;
        });
        return count;
    };

    int i = 0;
    for (auto const& test : tests)
    {
        int const num_invalid = CountInvalidUTF16Sequences(test.input);
        CAPTURE(i);
        CAPTURE(test.input);
        CHECK(test.num_replacements == num_invalid);
        ++i;
    }
}

#if 0
#include <filesystem>

TEST_CASE("std::path 1")
{
    std::filesystem::path p;

    cl::Cmdline cli("test", "test");
    cli.Add("p", "path", cl::Multiple::yes | cl::Arg::required, cl::Var(p));

    p = "";
    CHECK(true == ParseArgs(cli, {"-p", "path_without_spaces"}));
    cli.PrintDiag();
    CHECK(p.generic_string() == "path_without_spaces");
    p = "";
    CHECK(true == ParseArgs(cli, {"-p", "\"quoted path with spaces\""}));
    cli.PrintDiag();
    CHECK(p.generic_string() == "quoted path with spaces");
    //p = "";
    //CHECK(true == ParseArgs(cli, {"-p", "'quoted path with spaces'"}));
    //cli.PrintDiag();
    //CHECK(p.generic_string() == "quoted path with spaces");
    //p = "";
    //CHECK(true == ParseArgs(cli, {"-p", "path with spaces"}));
    //cli.PrintDiag();
    //CHECK(p.generic_string() == "path with spaces");
}

TEST_CASE("std::path 2")
{
    std::filesystem::path p;

    cl::Cmdline cli("test", "test");
    cli.Add("p", "path", cl::Multiple::yes | cl::Arg::required,
        [&](cl::ParseContext const& ctx) { p.assign(ctx.arg.begin(), ctx.arg.end()); });

    p = "";
    CHECK(true == ParseArgs(cli, {"-p", "path_without_spaces"}));
    cli.PrintDiag();
    CHECK(p.generic_string() == "path_without_spaces");
    //p = "";
    //CHECK(true == ParseArgs(cli, {"-p", "\"quoted path with spaces\""}));
    //cli.PrintDiag();
    //CHECK(p.generic_string() == "quoted path with spaces");
    //p = "";
    //CHECK(true == ParseArgs(cli, {"-p", "'quoted path with spaces'"}));
    //cli.PrintDiag();
    //CHECK(p.generic_string() == "quoted path with spaces");
    p = "";
    CHECK(true == ParseArgs(cli, {"-p", "path with spaces"}));
    cli.PrintDiag();
    CHECK(p.generic_string() == "path with spaces");
}
#endif

TEST_CASE("MayJoin")
{
    std::string value;

    cl::Cmdline cli("test", "test");
    cli.Add("n", "", cl::Multiple::yes | cl::Arg::required | cl::MayJoin::no, cl::Var(value));
    cli.Add("y", "", cl::Multiple::yes | cl::Arg::required | cl::MayJoin::yes, cl::Var(value));

    value = "?";
    CHECK(false == ParseArgs(cli, {"-n"}));
    CHECK(value == "?");
    value = "?";
    CHECK(true == ParseArgs(cli, {"-n", "dir"}));
    CHECK(value == "dir");
    value = "?";
    CHECK(true == ParseArgs(cli, {"-n=dir"}));
    CHECK(value == "dir");
    value = "?";
    CHECK(false == ParseArgs(cli, {"-ndir"}));
    CHECK(value == "?");

    value = "?";
    CHECK(false == ParseArgs(cli, {"-y"}));
    CHECK(value == "?");
    value = "?";
    CHECK(true == ParseArgs(cli, {"-y", "dir"}));
    CHECK(value == "dir");
    value = "?";
    CHECK(true == ParseArgs(cli, {"-y=dir"}));
    CHECK(value == "=dir");
    value = "?";
    CHECK(true == ParseArgs(cli, {"-ydir"}));
    CHECK(value == "dir");
}
