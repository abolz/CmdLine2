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
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace cl {

class Cmdline;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Controls how often an option may/must be specified.
enum class Opt : unsigned char {
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
enum class Arg : unsigned char {
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
enum class CommaSeparatedArg : unsigned char {
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

// Controls whether the Parse() methods should check for missing options.
enum class CheckMissing : unsigned char {
    // Do not check for missing required options in Cmdline::Parse().
    no,
    // Check for missing required options in Cmdline::Parse().
    // Any missing option will be reported as an error.
    // This is the default.
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
    std::string name;    // Name of the option being parsed
    std::string arg;     // Option argument
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
    Opt               num_occurrences_     = Opt::optional;
    Arg               num_args_            = Arg::no;
    JoinArg           join_arg_            = JoinArg::no;
    MayGroup          may_group_           = MayGroup::no;
    Positional        positional_          = Positional::no;
    CommaSeparatedArg comma_separated_arg_ = CommaSeparatedArg::no;
    ConsumeRemaining  consume_remaining_   = ConsumeRemaining::no;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    template <typename T> void Apply(T) = delete; // For slightly more useful error messages...

    void Apply(Opt               v) { num_occurrences_ = v; }
    void Apply(Arg               v) { num_args_ = v; }
    void Apply(JoinArg           v) { join_arg_ = v; }
    void Apply(MayGroup          v) { may_group_ = v; }
    void Apply(Positional        v) { positional_ = v; }
    void Apply(CommaSeparatedArg v) { comma_separated_arg_ = v; }
    void Apply(ConsumeRemaining  v) { consume_remaining_ = v; }

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
        std::string const name   = {};
        OptionBase* const option = nullptr;

        // For VS2015
        NameOptionPair() = default;
        NameOptionPair(std::string name_, OptionBase* option_)
            : name(std::move(name_))
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
    Cmdline();
    ~Cmdline();

    Cmdline(Cmdline const&) = delete;
    Cmdline& operator =(Cmdline const&) = delete;

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& diag() const { return diag_; }

    // Adds a diagnostic message
    void EmitDiag(Diagnostic::Type type, int index, std::string message);

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
    std::string GetHelp(std::string const& program_name) const;

    // Prints the help message to stderr
    void PrintHelp(std::string const& program_name) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(std::string const& name) const;
    OptionBase* FindOption(std::string const& name, bool& ambiguous) const;

    void DoAdd(OptionBase* opt);

    template <typename It, typename EndIt, typename Sink>
    bool DoParse(It& curr, EndIt last, Sink sink);

    template <typename It, typename EndIt>
    Result Handle1(It& curr, EndIt last, std::string optstr);

    // <file>
    Result HandlePositional(std::string const& optstr);

    // -f
    // -f <file>
    template <typename It, typename EndIt>
    Result HandleStandardOption(std::string const& optstr, It& curr, EndIt last);

    // -f=<file>
    Result HandleOption(std::string const& optstr);

    // -I<dir>
    Result HandlePrefix(std::string const& optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(std::string const& optstr, std::vector<OptionBase*>& group);

    template <typename It, typename EndIt>
    Result HandleGroup(std::string const& optstr, It& curr, EndIt last);

    template <typename It, typename EndIt>
    Result HandleOccurrence(OptionBase* opt, std::string const& name, It& curr, EndIt last);
    Result HandleOccurrence(OptionBase* opt, std::string const& name, std::string const& arg);

    Result ParseOptionArgument(OptionBase* opt, std::string const& name, std::string const& arg);
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename Parser, typename ...Args>
auto Cmdline::Add(std::string name, std::string descr, Parser&& parser, Args&&... args)
{
    using DecayedParser = std::decay_t<Parser>;

#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
    static_assert(
        std::is_invocable_r<bool, DecayedParser, ParseContext&>::value ||
        std::is_invocable_r<void, DecayedParser, ParseContext&>::value,
        "The parser must be invocable with an argument of type "
        "'ParseContext&' and the return type should be convertible "
        "to 'bool'");
#endif

    auto opt = std::make_unique<Option<DecayedParser>>( std::move(name), std::move(descr), std::forward<Parser>(parser), std::forward<Args>(args)... );
    auto const p = opt.get();

    unique_options_.push_back(std::move(opt)); // commit

    DoAdd(p);

    return p;
}

template <typename It, typename EndIt>
bool Cmdline::Parse(It first, EndIt last, CheckMissing check_missing)
{
    auto sink = [&](std::string const& curr, int index)
    {
        EmitDiag(Diagnostic::error, index, "Unknown option '" + curr + "'");
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
bool Cmdline::DoParse(It& curr, EndIt last, Sink sink)
{
    assert(curr_positional_ >= 0);
    assert(curr_index_ >= 0);

    while (curr != last)
    {
        // Make a copy of the current argument.
        // This is required in case It is an InputIterator.
        std::string optstr(*curr);

        Result const res = Handle1(curr, last, optstr);

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
Cmdline::Result Cmdline::Handle1(It& curr, EndIt last, std::string optstr)
{
    assert(curr != last);

    // This cannot happen if we're parsing the argv[] arrray, but it might
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

    optstr.erase(0, 1); // Remove the first dash.

    // If the name starts with a single dash, this is a short option and might
    // actually be an option group.
    bool const is_short = (optstr[0] != '-');
    if (!is_short)
    {
        optstr.erase(0, 1); // Remove the second dash.
    }

    Result res = HandleStandardOption(optstr, curr, last);

    if (res == Result::ignored)
    {
        res = HandleOption(optstr);
    }

    if (res == Result::ignored)
    {
        res = HandlePrefix(optstr);
    }

    if (res == Result::ignored && is_short)
    {
        res = HandleGroup(optstr, curr, last);
    }

    return res;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleStandardOption(std::string const& optstr, It& curr, EndIt last)
{
    bool ambiguous = false;
    if (auto const opt = FindOption(optstr, ambiguous))
    {
        if (ambiguous)
        {
            EmitDiag(Diagnostic::error, curr_index_, "Option '" + optstr + "' is ambiguous");
            return Result::error;
        }

        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::ignored;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleGroup(std::string const& optstr, It& curr, EndIt last)
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

        if (opt->num_args_ == Arg::no || n + 1 == optstr.size())
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

        auto const arg = optstr.substr(arg_start);
        return HandleOccurrence(opt, name, arg);
    }

    return Result::success;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std::string const& name, It& curr, EndIt last)
{
    assert(curr != last);

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    if (opt->join_arg_ != JoinArg::yes)
    {
        if (opt->num_args_ != Arg::required)
        {
            return ParseOptionArgument(opt, name, {});
        }

        // If the option requires an argument, steal one from the command line.
        ++curr;
        ++curr_index_;

        if (curr != last)
        {
            // Make a copy of the current argument.
            // Just in case *curr is only explicitly convertible to std::string.
            std::string optstr(*curr);

            return ParseOptionArgument(opt, name, optstr);
        }
    }

    EmitDiag(Diagnostic::error, curr_index_, "Option '" + name + "' requires an argument");
    return Result::error;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

//
// XXX
//
// ParseValue<>::operator() and predicates have the same signature...
// Combine?!?!
//

// Convert the string representation in STR into an object of type T.
template <typename T = void, typename /*Enable*/ = void>
struct ConvertValue
{
    template <typename Stream = std::stringstream>
    bool operator()(std::string const& str, T& value) const
    {
        Stream stream {str};
        stream >> value;
        return !stream.fail() && stream.eof();
    }
};

template <> struct ConvertValue< bool               > { bool operator()(std::string const& str, bool&               value) const; };
template <> struct ConvertValue< signed char        > { bool operator()(std::string const& str, signed char&        value) const; };
template <> struct ConvertValue< signed short       > { bool operator()(std::string const& str, signed short&       value) const; };
template <> struct ConvertValue< signed int         > { bool operator()(std::string const& str, signed int&         value) const; };
template <> struct ConvertValue< signed long        > { bool operator()(std::string const& str, signed long&        value) const; };
template <> struct ConvertValue< signed long long   > { bool operator()(std::string const& str, signed long long&   value) const; };
template <> struct ConvertValue< unsigned char      > { bool operator()(std::string const& str, unsigned char&      value) const; };
template <> struct ConvertValue< unsigned short     > { bool operator()(std::string const& str, unsigned short&     value) const; };
template <> struct ConvertValue< unsigned int       > { bool operator()(std::string const& str, unsigned int&       value) const; };
template <> struct ConvertValue< unsigned long      > { bool operator()(std::string const& str, unsigned long&      value) const; };
template <> struct ConvertValue< unsigned long long > { bool operator()(std::string const& str, unsigned long long& value) const; };
template <> struct ConvertValue< float              > { bool operator()(std::string const& str, float&              value) const; };
template <> struct ConvertValue< double             > { bool operator()(std::string const& str, double&             value) const; };
template <> struct ConvertValue< long double        > { bool operator()(std::string const& str, long double&        value) const; };
template <> struct ConvertValue< std::string        > { bool operator()(std::string const& str, std::string&        value) const; };

template <typename Key, typename Value>
struct ConvertValue<std::pair<Key, Value>>
{
    bool operator()(std::string const& str, std::pair<Key, Value>& value) const
    {
        auto const p = str.find(':');

        if (p == std::string::npos)
            return false;

        return ConvertValue<Key>{}(str.substr(0, p), value.first)
            && ConvertValue<Value>{}(str.substr(p + 1), value.second);
    }
};

template <>
struct ConvertValue<void>
{
    template <typename T>
    bool operator()(std::string const& str, T& value) const
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
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be in the range [" + std::to_string(lower) + ", " + std::to_string(upper) + "]");
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
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be greater than " + std::to_string(lower));
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
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be greater than or equal to " + std::to_string(lower));
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
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be less than " + std::to_string(upper));
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
        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Argument must be less than or equal to " + std::to_string(upper));
        return false;
    };
}

} // namespace check

namespace impl
{
    template <typename T>
    struct RemoveCVRecImpl
    {
        using type = std::remove_cv_t<T>;
    };

    template <template <typename...> class T, typename ...Args>
    struct RemoveCVRecImpl<T<Args...>>
    {
        using type = T<typename RemoveCVRecImpl<Args>::type...>;
    };

    template <typename T>
    using RemoveCVRec = typename RemoveCVRecImpl<T>::type;
}

//
// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
//
template <typename T, typename ...Predicates>
auto Assign(T& value, Predicates&&... preds)
{
    return [=, &value](ParseContext& ctx)
    {
#if __cplusplus >= 201703
        return ParseValue<>{}(ctx, value) && (... && preds(ctx, value));
#else
        bool res = true;
        bool unused[] = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
        static_cast<void>(unused);
        return res;
#endif
    };
}

//
// Default parser for list types.
// Uses an instance of Parser<> to convert the string and then inserts the
// converted value into the container.
//
// Predicates apply to the currently parsed value, not the whole list.
//
template <typename T, typename ...Predicates>
auto PushBack(T& container, Predicates&&... preds)
{
    return [=, &container](ParseContext& ctx)
    {
        impl::RemoveCVRec<typename T::value_type> value;

#if __cplusplus >= 201703
        if (ParseValue<>{}(ctx, value) && (... && preds(ctx, value)))
#else
        bool res = true;
        bool unused[] = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
        static_cast<void>(unused);
        if (res)
#endif
        {
            container.insert(container.end(), std::move(value));
            return true;
        }

        return false;
    };
}

//
// Default parser for enum types.
// Look up the key in the map and if it exists, returns the mapped value.
//
template <typename T, typename ...Predicates>
auto Map(T& value, std::initializer_list<std::pair<std::string, T>> ilist, Predicates&&... preds)
{
    using MapType = std::vector<std::pair<std::string, T>>;

    return [=, &value, map = MapType(ilist)](ParseContext& ctx)
    {
        for (auto const& p : map)
        {
            if (p.first == ctx.arg)
            {
                value = p.second;
#if __cplusplus >= 201703
                if ((... && preds(ctx, value)))
#else
                bool res = true;
                bool unused[] = { (res = res && preds(ctx, value))..., false };
                static_cast<void>(unused);
                if (res)
#endif
                {
                    return true;
                }
            }
        }

        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Invalid argument '" + ctx.arg + "' for  option '" + ctx.name + "'");
        for (auto const& p : map)
        {
            ctx.cmdline->EmitDiag(Diagnostic::note, ctx.index, "Could be '" + p.first + "'");
        }

        return false;
    };
}

} // namespace cl

#endif // CL_CMDLINE_H
