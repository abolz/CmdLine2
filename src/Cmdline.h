// Copyright (c) 2017 Alexander Bolz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef CL_CMDLINE_H
#define CL_CMDLINE_H 1

#include <cassert>
#include <cstring>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace cl {

class Cmdline;

class string_view // Minimal std::string_view replacement for C++14 compatibility
{
public:
    using value_type      = char;
    using const_pointer   = char const*;
    using const_reference = char const&;
    using const_iterator  = char const*;

private:
    const_pointer data_ = nullptr;
    size_t        size_ = 0;

    static size_t Min(size_t x, size_t y) { return y < x ? y : x; }

public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    constexpr string_view() noexcept = default;
    constexpr string_view(string_view const&) noexcept = default;

    /*constexpr*/ string_view(const_pointer ptr, size_t len) noexcept
        : data_(ptr)
        , size_(len)
    {
        assert(size_ == 0 || data_ != nullptr);
    }

    /*constexpr*/ /*implicit*/ string_view(const_pointer c_str) noexcept
        : data_(c_str)
        , size_(c_str ? ::strlen(c_str) : 0u)
    {
    }

    /*constexpr*/ /*implicit*/ string_view(std::string const& str) noexcept
        : data_(str.data())
        , size_(str.size())
    {
    }

    explicit operator std::string() const {
        // Construct from iterators!
        // Constructing from data/size requires data != null...
        return std::string(begin(), end());
    }

    // Returns a pointer to the start of the string.
    constexpr const_pointer data() const noexcept { return data_; }

    // Returns the length of the string.
    constexpr size_t size() const noexcept { return size_; }

    // Returns whether the string is empty.
    constexpr bool empty() const noexcept { return size_ == 0; }

    // Returns a reference to the N-th character of the string.
    /*constexpr*/ const_reference operator[](size_t n) const noexcept {
        assert(n < size_);
        return data_[n];
    }

    // Returns an iterator pointing to the start of the string.
    constexpr const_iterator begin() const noexcept { return data_; }

    // Returns an iterator pointing past the end of the string.
    constexpr const_iterator end() const noexcept { return data_ + size_; }

    bool _cmp_eq(string_view other) const noexcept;
    bool _cmp_lt(string_view other) const noexcept;

    // Lexicographically compare this string with another string OTHER.
    int compare(string_view other) const noexcept;

    // Removes the first N characters from the string.
    void remove_prefix(size_t n) noexcept {
        assert(n <= size_);
        data_ += n;
        size_ -= n;
    }

    // Removes the last N characters from the string.
    void remove_suffix(size_t n) noexcept {
        assert(n <= size_);
        size_ -= n;
    }

    // Returns the substring [first, +count)
    /*constexpr*/ string_view substr(size_t first = 0, size_t count = npos) const noexcept {
        assert(first <= size_);
        return { data_ + first, Min(count, size_ - first) };
    }

    // Search for the first character ch in the sub-string [from, end)
    size_t find(char ch, size_t from = 0) const noexcept;

    // Search for the last character ch in the sub-string [0, from)
    size_t rfind(char ch, size_t from = npos) const noexcept;

    // Search for the first character in the sub-string [from, end)
    // which matches any of the characters in chars.
    size_t find_first_of(string_view chars, size_t from = 0) const noexcept;

    // Search for the last character in the sub-string [0, from)
    // which matches any of the characters in chars.
    size_t find_last_of(string_view chars, size_t from = npos) const noexcept;
};

inline bool operator==(string_view s1, string_view s2) noexcept {
    return s1._cmp_eq(s2);
}

inline bool operator!=(string_view s1, string_view s2) noexcept {
    return !(s1 == s2);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Controls how often an option may/must be specified.
enum class NumOpts : unsigned char {
    // The option may appear at most once.
    // This is the default.
    optional,
    // The option must appear exactly once.
    required,
    // The option may appear multiple times.
    zero_or_more,
    // The option must appear at least once.
    one_or_more,
};

// Controls the number of arguments the option accepts.
enum class HasArg : unsigned char {
    // An argument is not allowed.
    // This is the default.
    no,
    // An argument is optional.
    optional,
    // An argument is required.
    required,
};

// Controls whether the option may/must join its argument.
enum class JoinArg : unsigned char {
    // The option must not join its argument: "-I dir" and "-I=dir" are
    // possible. If the option is specified with an equals sign ("-I=dir") the
    // '=' will NOT be part of the option argument.
    // This is the default.
    no,
    // The option may join its argument: "-I dir" and "-Idir" are possible. If
    // the option is specified with an equals sign ("-I=dir") the '=' will be
    // part of the option argument.
    optional,
    // The option must join its argument: "-Idir" is the only possible format.
    // If the option is specified with an equals sign ("-I=dir") the '=' will be
    // part of the option argument.
    yes,
};

// May this option group with other options?
enum class MayGroup : unsigned char {
    // The option may not be grouped with other options (even if the option name
    // consists only of a single letter).
    // This is the default.
    no,
    // The option may be grouped with other options.
    // This flag is ignored if the names of the options are not a single letter
    // and option groups must be prefixed with a single '-', e.g. "-xvf=file".
    yes,
};

// Positional option?
enum class Positional : unsigned char {
    // The option is not a positional option, i.e. requires '-' or '--' as a
    // prefix when specified.
    // This is the default.
    no,
    // Positional option, no '-' required.
    yes,
};

// Split the argument between commas?
enum class CommaSeparated : unsigned char {
    // Do not split the argument between commas.
    // This is the default.
    no,
    // If this flag is set, the option's argument is split between commas, e.g.
    // "-i=1,2,,3" will be parsed as ["-i=1", "-i=2", "-i=", "-i=3"].
    // Note that each comma-separated argument counts as an option occurrence.
    yes,
};

// Parse all following options as positional options?
enum class ConsumeRemaining : unsigned char {
    // Nothing special.
    // This is the default.
    no,
    // If an option with this flag is (successfully) parsed, all the remaining
    // options are parsed as positional options.
    yes,
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Provides information about the argument and the command line parser which
// is currently parsing the arguments.
// The members are only valid inside the callback (parser).
struct ParseContext
{
    string_view name;    // Name of the option being parsed    (only valid in callback!)
    string_view arg;     // Option argument                    (only valid in callback!)
    int         index;   // Current index in the argv array
    Cmdline*    cmdline; // The command line parser which currently parses the argument list (never null)
};

class OptionBase
{
    friend class Cmdline;

    // The name of the option.
    std::string const name_;
    // The description of this option
    std::string const descr_;
    // Flags controlling how the option may/must be specified.
    NumOpts          num_opts_          = NumOpts::optional;
    HasArg           has_arg_           = HasArg::no;
    JoinArg          join_arg_          = JoinArg::no;
    MayGroup         may_group_         = MayGroup::no;
    Positional       positional_        = Positional::no;
    CommaSeparated   comma_separated_   = CommaSeparated::no;
    ConsumeRemaining consume_remaining_ = ConsumeRemaining::no;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    template <typename T> void Apply(T) = delete; // For slightly more useful error messages...

    void Apply(NumOpts          v) { num_opts_ = v; }
    void Apply(HasArg           v) { has_arg_ = v; }
    void Apply(JoinArg          v) { join_arg_ = v; }
    void Apply(MayGroup         v) { may_group_ = v; }
    void Apply(Positional       v) { positional_ = v; }
    void Apply(CommaSeparated   v) { comma_separated_ = v; }
    void Apply(ConsumeRemaining v) { consume_remaining_ = v; }

protected:
    template <typename ...Args>
    explicit OptionBase(std::string name, std::string descr, Args&&... args)
        : name_(std::move(name))
        , descr_(std::move(descr))
    {
#if __cplusplus >= 201703
        (Apply(args), ...);
#else
        int unused[] = {(Apply(args), 0)..., 0};
        static_cast<void>(unused);
#endif
    }

public:
    virtual ~OptionBase();

public:
    // Returns the name of this option
    std::string const& name() const { return name_; }

    // Returns the description of this option
    std::string const& descr() const { return descr_; }

    // Returns the number of times this option was specified on the command line
    int count() const { return count_; }

private:
    bool IsOccurrenceAllowed() const;
    bool IsOccurrenceRequired() const;

    // Parse the given value from NAME and/or ARG and store the result.
    // Return true on success, false otherwise.
    virtual bool Parse(ParseContext& ctx) = 0;
};

template <typename Parser>
class Option : public OptionBase
{
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
    static_assert(
        std::is_invocable_r<bool, Parser, ParseContext&>::value ||
        std::is_invocable_r<void, Parser, ParseContext&>::value,
        "The parser must be invocable with an argument of type "
        "'ParseContext&' and the return type should be convertible "
        "to 'bool'");
#endif

    Parser /*const*/ parser_;

public:
    template <typename ParserInit, typename ...Args>
    Option(std::string name, std::string descr, ParserInit&& parser, Args&&... args)
        : OptionBase(std::move(name), std::move(descr), std::forward<Args>(args)...)
        , parser_(std::forward<ParserInit>(parser))
    {
    }

    Parser const& parser() const { return parser_; }

private:
    bool Parse(ParseContext& ctx) override {
        return DoParse(ctx, std::is_convertible<decltype(parser_(ctx)), bool>{});
    }

    bool DoParse(ParseContext& ctx, /*parser_ returns bool*/ std::true_type) {
        return parser_(ctx);
    }

    bool DoParse(ParseContext& ctx, /*parser_ returns bool*/ std::false_type) {
        parser_(ctx);
        return true;
    }
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

struct Diagnostic
{
    enum /*class*/ Type { error, warning, note };

    Type        type    = Type::error;
    int         index   = -1;
    std::string message = {};

    // For VS2015
    Diagnostic() = default;
    Diagnostic(Type type_, int index_, std::string message_)
        : type(type_)
        , index(index_)
        , message(std::move(message_))
    {
    }
};

class Cmdline
{
    struct NameOptionPair
    {
        string_view name   = {}; // Points into option->name_
        OptionBase* option = nullptr;

        // For VS2015
        NameOptionPair() = default;
        NameOptionPair(string_view name_, OptionBase* option_)
            : name(name_)
            , option(option_)
        {
        }
    };

    using Diagnostics   = std::vector<Diagnostic>;
    using UniqueOptions = std::vector<std::unique_ptr<OptionBase>>;
    using Options       = std::vector<NameOptionPair>;

    Diagnostics     diag_            = {}; // List of diagnostic messages
    UniqueOptions   unique_options_  = {}; // Option storage.
    Options         options_         = {}; // List of options. Includes the positional options (in order).
    int             max_prefix_len_  = 0;  // Maximum length of the names of all prefix options
    int             curr_positional_ = 0;  // The current positional argument - if any
    int             curr_index_      = 0;  // Index of the current argument
    bool            dashdash_        = false; // "--" seen?

public:
    // Controls whether the Parse() methods should check for missing options.
    enum class CheckMissing : unsigned char {
        // Do not check for missing required options in Cmdline::Parse().
        no,
        // Check for missing required options in Cmdline::Parse().
        // Any missing option will be reported as an error.
        // This is the default.
        yes,
    };

public:
    Cmdline();
    ~Cmdline();

    Cmdline(Cmdline const&) = delete;
    Cmdline& operator =(Cmdline const&) = delete;

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& diag() const { return diag_; }

    // Adds a diagnostic message
    void EmitDiag(Diagnostic::Type type, int index, std::string message);

#ifdef __GNUC__
    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...) __attribute__((format(printf, 4, 5)));
#else
    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...);
#endif

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    // NAME and DESCR must be valid while this command line parse is still alive.
    template <typename Parser, typename ...Args>
    auto Add(std::string name, std::string descr, Parser&& parser, Args&&... args);

    // Resets the parser. Sets the COUNT members of all registered options to 0.
    void Reset();

    // Parse the command line arguments in [first, last).
    // Emits an error for unknown options.
    template <typename It, typename EndIt>
    bool Parse(It first, EndIt last, CheckMissing check_missing = CheckMissing::yes);

    // Parse the command line arguments in [first, last).
    // Calls sink() for unknown options.
    //
    // Sink must have signature "bool sink(It, int)" and should return false if
    // the parser should stop, or return true to continue parsing command line
    // arguments.
    template <typename It, typename EndIt, typename Sink>
    bool Parse(It first, EndIt last, CheckMissing check_missing, Sink sink);

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if all required options have been (successfully) parsed.
    bool CheckMissingOptions();

    // Prints error messages to stderr.
    void PrintDiag() const;

    // Returns a short help message listing all registered options.
    std::string FormatHelp(string_view program_name) const;

    // Prints the help message to stderr
    void PrintHelp(string_view program_name) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(string_view name) const;

    void DoAdd(std::unique_ptr<OptionBase> opt);

    template <typename It, typename EndIt, typename Sink>
    bool DoParse(It curr, EndIt last, Sink sink);

    template <typename It, typename EndIt>
    Result Handle1(string_view optstr, It& curr, EndIt last);

    // <file>
    Result HandlePositional(string_view optstr);

    // -f
    // -f <file>
    template <typename It, typename EndIt>
    Result HandleStandardOption(string_view optstr, It& curr, EndIt last);

    // -f=<file>
    Result HandleOption(string_view optstr);

    // -I<dir>
    Result HandlePrefix(string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(string_view optstr, std::vector<OptionBase*>& group);

    template <typename It, typename EndIt>
    Result HandleGroup(string_view optstr, It& curr, EndIt last);

    template <typename It, typename EndIt>
    Result HandleOccurrence(OptionBase* opt, string_view name, It& curr, EndIt last);
    Result HandleOccurrence(OptionBase* opt, string_view name, string_view arg);

    Result ParseOptionArgument(OptionBase* opt, string_view name, string_view arg);
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename Parser, typename ...Args>
auto Cmdline::Add(std::string name, std::string descr, Parser&& parser, Args&&... args)
{
    using DecayedParser = std::decay_t<Parser>;

    auto opt = std::make_unique<Option<DecayedParser>>(
        std::move(name), std::move(descr), std::forward<Parser>(parser), std::forward<Args>(args)...);
    auto const p = opt.get();

    DoAdd(std::move(opt));

    return p;
}

template <typename It, typename EndIt>
bool Cmdline::Parse(It first, EndIt last, CheckMissing check_missing)
{
    auto sink = [&](string_view curr, int index)
    {
        FormatDiag(Diagnostic::error, index, "Unknown option '%.*s'", static_cast<int>(curr.size()), curr.data());
        return false;
    };

    return Parse(first, last, check_missing, sink);
}

template <typename It, typename EndIt, typename Sink>
bool Cmdline::Parse(It first, EndIt last, CheckMissing check_missing, Sink sink)
{
    if (!DoParse(first, last, sink))
        return false;

    if (check_missing == CheckMissing::yes)
        return CheckMissingOptions();

    return true;
}

template <typename It, typename EndIt, typename Sink>
bool Cmdline::DoParse(It curr, EndIt last, Sink sink)
{
    assert(curr_positional_ >= 0);
    assert(curr_index_ >= 0);

    while (curr != last)
    {
        // Make a copy of the current argument.
        // This is required in case
        //  - It models InputIterator.
        //      In this case *CURR might be invalid after the call to Handle1,
        //      since this function might increment CURR.
        //  - *CURR does not actually return a lvalue-reference.
        //      This problem might be solved by wrapping the loop-body into
        //      another function, though.
        //
        // TODO:
        // Use some meta-progamming to check if the copy is required...
        std::string optstr(*curr);

        Result const res = Handle1(optstr, curr, last);
        if (res == Result::error)
            return false;
        if (res == Result::ignored && !sink(optstr, curr_index_))
            return false;

        // Handle1 might have changed CURR.
        // Need to recheck if we're done.
        if (curr == last)
            break;
        ++curr;
        ++curr_index_;
    }

    return true;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::Handle1(string_view optstr, It& curr, EndIt last)
{
    assert(curr != last);

    // This cannot happen if we're parsing the main's argv[] array, but it might
    // happen if we're parsing a user-supplied array of command line arguments.
    if (optstr.empty())
    {
        return Result::success;
    }

    // Stop parsing if "--" has been found
    if (optstr == "--" && !dashdash_)
    {
        dashdash_ = true;
        return Result::success;
    }

    // This argument is considered to be positional if it doesn't start with
    // '-', if it is "-" itself, or if we have seen "--" already.
    if (optstr[0] != '-' || optstr == "-" || dashdash_)
    {
        return HandlePositional(optstr);
    }

    // Starts with a dash, must be an option.

    optstr.remove_prefix(1); // Remove the first dash.

    // If the name starts with a single dash, this is a short option and might
    // actually be an option group.
    bool const is_short = (optstr[0] != '-');
    if (!is_short)
    {
        optstr.remove_prefix(1); // Remove the second dash.
    }

    // 1. Try to handle options like "-f" and "-f file"
    Result res = HandleStandardOption(optstr, curr, last);

    // 2. Try to handle options like "-f=file"
    if (res == Result::ignored)
    {
        res = HandleOption(optstr);
    }

    // 3. Try to handle options like "-Idir"
    if (res == Result::ignored)
    {
        res = HandlePrefix(optstr);
    }

    // 4. Try to handle options like "-xvf=file" and "-xvf file"
    if (res == Result::ignored && is_short)
    {
        res = HandleGroup(optstr, curr, last);
    }

    // Otherwise this is an unknown option.

    return res;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleStandardOption(string_view optstr, It& curr, EndIt last)
{
    if (auto const opt = FindOption(optstr))
    {
        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::ignored;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleGroup(string_view optstr, It& curr, EndIt last)
{
    std::vector<OptionBase*> group;

    // First determine if this is a valid option group.
    auto const res = DecomposeGroup(optstr, group);

    if (res != Result::success)
        return res;

    // Then process all options.
    for (size_t n = 0; n < group.size(); ++n)
    {
        auto const opt = group[n];
        auto const name = optstr.substr(n, 1);

        if (opt->has_arg_ == HasArg::no || n + 1 == optstr.size())
        {
            if (Result::success != HandleOccurrence(opt, name, curr, last))
                return Result::error;
            continue;
        }

        // Almost done. Process the last option which accepts an argument.

        size_t arg_start = n + 1;

        // If the next character is '=' and the option may not join its
        // argument, discard the equals sign.
        if (optstr[arg_start] == '=' && opt->join_arg_ == JoinArg::no)
        {
            ++arg_start;
        }

        return HandleOccurrence(opt, name, optstr.substr(arg_start));
    }

    return Result::success;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, string_view name, It& curr, EndIt last)
{
    assert(curr != last);

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    if (opt->join_arg_ != JoinArg::yes)
    {
        if (opt->has_arg_ != HasArg::required)
        {
            return ParseOptionArgument(opt, name, {});
        }

        // If the option requires an argument, steal one from the command line.
        ++curr;
        ++curr_index_;

        if (curr != last)
        {
            return ParseOptionArgument(opt, name, *curr);
        }
    }

    FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' requires an argument",
        static_cast<int>(name.size()), name.data());
    return Result::error;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Convert the string representation in STR into an object of type T.
template <typename T = void, typename /*Enable*/ = void>
struct ConvertValue
{
    template <typename Stream = std::stringstream>
    bool operator()(string_view str, T& value) const
    {
        Stream stream{std::string(str)};
        stream >> value;
        return !stream.fail() && stream.eof();
    }
};

template <> struct ConvertValue< bool               > { bool operator()(string_view str, bool&               value) const; };
template <> struct ConvertValue< signed char        > { bool operator()(string_view str, signed char&        value) const; };
template <> struct ConvertValue< signed short       > { bool operator()(string_view str, signed short&       value) const; };
template <> struct ConvertValue< signed int         > { bool operator()(string_view str, signed int&         value) const; };
template <> struct ConvertValue< signed long        > { bool operator()(string_view str, signed long&        value) const; };
template <> struct ConvertValue< signed long long   > { bool operator()(string_view str, signed long long&   value) const; };
template <> struct ConvertValue< unsigned char      > { bool operator()(string_view str, unsigned char&      value) const; };
template <> struct ConvertValue< unsigned short     > { bool operator()(string_view str, unsigned short&     value) const; };
template <> struct ConvertValue< unsigned int       > { bool operator()(string_view str, unsigned int&       value) const; };
template <> struct ConvertValue< unsigned long      > { bool operator()(string_view str, unsigned long&      value) const; };
template <> struct ConvertValue< unsigned long long > { bool operator()(string_view str, unsigned long long& value) const; };
template <> struct ConvertValue< float              > { bool operator()(string_view str, float&              value) const; };
template <> struct ConvertValue< double             > { bool operator()(string_view str, double&             value) const; };
template <> struct ConvertValue< long double        > { bool operator()(string_view str, long double&        value) const; };
template <> struct ConvertValue< std::string        > { bool operator()(string_view str, std::string&        value) const; };

template <typename Key, typename Value>
struct ConvertValue<std::pair<Key, Value>>
{
    bool operator()(string_view str, std::pair<Key, Value>& value) const
    {
        auto const p = str.find(':');

        if (p == string_view::npos)
            return false;

        return ConvertValue<Key>{}(str.substr(0, p), value.first)
            && ConvertValue<Value>{}(str.substr(p + 1), value.second);
    }
};

template <>
struct ConvertValue<void>
{
    template <typename T>
    bool operator()(string_view str, T& value) const
    {
        return ConvertValue<T>{}(str, value);
    }
};

// Convert the string representation in CTX.ARG into an object of type T.
// Possibly emits diagnostics on error.
template <typename T = void, typename /*Enable*/ = void>
struct ParseValue
{
    bool operator()(ParseContext const& ctx, T& value) const
    {
        return ConvertValue<T>{}(ctx.arg, value);
    }
};

template <>
struct ParseValue<void>
{
    template <typename T>
    bool operator()(ParseContext /*const*/& ctx, T& value) const
    {
        return ParseValue<T>{}(ctx, value);
    }
};

namespace check {

// Returns a function object which checks whether a given value is in the range [lower, upper].
// Emits a diagnostic on error.
template <typename T, typename U>
auto InRange(T lower, U upper)
{
    return [=](ParseContext& ctx, auto const& value)
    {
        if (!(value < lower) && !(upper < value))
            return true;
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index,
            "Argument must be in [" + std::to_string(lower) + ", " + std::to_string(upper) + "]");
        return false;
    };
}

// Returns a function object which checks whether a given value is > lower.
// Emits a diagnostic on error.
template <typename T>
auto GreaterThan(T lower)
{
    return [=](ParseContext& ctx, auto const& value)
    {
        if (lower < value)
            return true;
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be > " + std::to_string(lower));
        return false;
    };
}

// Returns a function object which checks whether a given value is >= lower.
// Emits a diagnostic on error.
template <typename T>
auto GreaterEqual(T lower)
{
    return [=](ParseContext& ctx, auto const& value)
    {
        if (!(value < lower)) // value >= lower
            return true;
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be >= " + std::to_string(lower));
        return false;
    };
}

// Returns a function object which checks whether a given value is < upper.
// Emits a diagnostic on error.
template <typename T>
auto LessThan(T upper)
{
    return [=](ParseContext& ctx, auto const& value)
    {
        if (value < upper)
            return true;
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be < " + std::to_string(upper));
        return false;
    };
}

// Returns a function object which checks whether a given value is <= upper.
// Emits a diagnostic on error.
template <typename T>
auto LessEqual(T upper)
{
    return [=](ParseContext& ctx, auto const& value)
    {
        if (!(upper < value)) // upper >= value
            return true;
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be <= " + std::to_string(upper));
        return false;
    };
}

} // namespace check

namespace impl
{
    template <typename T>
    struct RemoveCVRec
    {
        using type = std::remove_cv_t<T>;
    };

    template <template <typename...> class T, typename ...Args>
    struct RemoveCVRec<T<Args...>>
    {
        using type = T<typename RemoveCVRec<Args>::type...>;
    };

    // Calls f(CTX, VALUE) for all f in FUNCS (in order) until the first f returns false.
    // Returns true iff all f return true.
    template <typename T, typename ...Funcs>
    bool ApplyFuncs(ParseContext& ctx, T& value, Funcs&&... funcs)
    {
        static_cast<void>(ctx);   // may be unused if funcs is empty
        static_cast<void>(value); // may be unused if funcs is empty

#if __cplusplus >= 201703
        return (... && funcs(ctx, value));
#else
        bool res = true;
        bool unused[] = { (res = res && funcs(ctx, value))..., false };
        static_cast<void>(unused);
        return res;
#endif
    }
}

// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
template <typename T, typename ...Predicates>
auto Assign(T& target, Predicates&&... preds)
{
    return [=, &target](ParseContext& ctx)
    {
        // XXX:
        // Writes to VALUE even if any predicate returns false...

        return impl::ApplyFuncs(ctx, target, ParseValue<>{}, preds...);
    };
}

// Default parser for list types.
// Uses an instance of Parser<> to convert the string and then inserts the
// converted value into the container.
// Predicates apply to the currently parsed value, not the whole list.
template <typename T, typename ...Predicates>
auto PushBack(T& container, Predicates&&... preds)
{
    return [=, &container](ParseContext& ctx)
    {
        using V = typename impl::RemoveCVRec<typename T::value_type>::type;

        V value;
        if (impl::ApplyFuncs(ctx, value, ParseValue<>{}, preds...))
        {
            container.insert(container.end(), std::move(value));
            return true;
        }
        return false;
    };
}

// Default parser for enum types.
// Look up the key in the map and if it exists, returns the mapped value.
template <typename T, typename ...Predicates>
auto Map(T& value, std::initializer_list<std::pair<char const*, T>> ilist, Predicates&&... preds)
{
    using MapType = std::vector<std::pair<char const*, T>>;

    return [=, &value, map = MapType(ilist)](ParseContext& ctx)
    {
        // XXX:
        // Writes to VALUE even if any predicate returns false...

        for (auto const& p : map)
        {
            if (p.first == ctx.arg)
            {
                value = p.second;
                if (impl::ApplyFuncs(ctx, value, preds...))
                    return true;
            }
        }

        ctx.cmdline->FormatDiag(Diagnostic::error, ctx.index, "Invalid argument '%.*s' for option '%.*s'",
            static_cast<int>(ctx.arg.size()), ctx.arg.data(),
            static_cast<int>(ctx.name.size()), ctx.name.data());
        for (auto const& p : map)
        {
            ctx.cmdline->FormatDiag(Diagnostic::note, ctx.index, "Could be '%s'", p.first);
        }

        return false;
    };
}

} // namespace cl

#endif // CL_CMDLINE_H
