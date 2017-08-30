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

#include "cxx_string_view.h"

#include <cassert>
#include <memory>
#include <sstream>
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

// Contains the help text for an option.
struct Descr
{
    char const* s;
    explicit Descr(char const* c_str) : s(c_str) {}
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

struct ParseContext
{
    cxx::string_view name;    // Name of the option being parsed
    cxx::string_view arg;     // Option argument
    int              index;   // Current index in the argv array
    Cmdline*         cmdline; // The command line parser which currently parses the argument list (never null)
};

class OptionBase
{
    friend class Cmdline;

    // The name of the option.
    cxx::string_view const name_;
    // The description of this option
    cxx::string_view descr_;
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

    void Apply(Descr             v) { descr_ = v.s; }
    void Apply(char const*       v) { descr_ = v; }
    void Apply(Opt               v) { num_occurrences_ = v; }
    void Apply(Arg               v) { num_args_ = v; }
    void Apply(JoinArg           v) { join_arg_ = v; }
    void Apply(MayGroup          v) { may_group_ = v; }
    void Apply(Positional        v) { positional_ = v; }
    void Apply(CommaSeparatedArg v) { comma_separated_arg_ = v; }
    void Apply(ConsumeRemaining  v) { consume_remaining_ = v; }

protected:
    template <typename ...Args>
    explicit OptionBase(char const* name, Args&&... args)
        : name_(name)
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
    cxx::string_view name() const { return name_; }

    // Returns the description of this option
    cxx::string_view descr() const { return descr_; }

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
    Option(ParserInit&& parser, char const* name, Args&&... args)
        : OptionBase(name, std::forward<Args>(args)...)
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
};

class Cmdline
{
    struct NameOptionPair
    {
        cxx::string_view name   = {}; // NB: Names are always constructed from C-strings
        OptionBase*      option = nullptr;
    };

    using UniqueOptions = std::vector<std::unique_ptr<OptionBase>>;
    using Options       = std::vector<NameOptionPair>;
    using Diagnostics   = std::vector<Diagnostic>;

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

    void EmitDiag(Diagnostic::Type type, int index, std::string message);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    template <typename Parser, typename ...Args>
    auto Add(Parser&& parser, char const* name, Args&&... args);

    // Reset the parser.
    void Reset();

    // Parse the command line arguments in [first, last).
    // Emits an error for unknown options.
    template <typename It>
    bool Parse(It first, It last, CheckMissing check_missing = CheckMissing::yes);

    // Parse the command line arguments in [first, last).
    // Calls sink() for unknown options.
    //
    // Sink must have signature "bool sink(It, int)" and should return false if
    // the parser should stop or true to continue parsing command line
    // arguments.
    template <typename It, typename Sink>
    bool Parse(It first, It last, CheckMissing check_missing, Sink sink);

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if all required options have been (successfully) parsed.
    bool CheckMissingOptions();

    // Prints error messages to stderr
    void PrintErrors() const;

    // Returns a short help message
    std::string HelpMessage(std::string const& program_name) const;

    // Prints the help message to stderr
    void PrintHelpMessage(std::string const& program_name) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(cxx::string_view name) const;
    OptionBase* FindOption(cxx::string_view name, bool& ambiguous) const;

    void DoAdd(OptionBase* opt);

    template <typename It, typename Sink>
    bool DoParse(It& curr, It last, Sink sink);

    template <typename It>
    Result Handle1(It& curr, It last);

    // <file>
    Result HandlePositional(cxx::string_view optstr);

    // -f
    // -f <file>
    template <typename It>
    Result HandleStandardOption(cxx::string_view optstr, It& curr, It last);

    // -f=<file>
    Result HandleOption(cxx::string_view optstr);

    // -I<dir>
    Result HandlePrefix(cxx::string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(cxx::string_view optstr, std::vector<OptionBase*>& group);

    template <typename It>
    Result HandleGroup(cxx::string_view optstr, It& curr, It last);

    template <typename It>
    Result HandleOccurrence(OptionBase* opt, cxx::string_view name, It& curr, It last);
    Result HandleOccurrence(OptionBase* opt, cxx::string_view name, cxx::string_view arg);

    Result ParseOptionArgument(OptionBase* opt, cxx::string_view name, cxx::string_view arg);
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace impl
{
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)

    template <typename R, typename T, typename ...Args>
    using IsInvocableR = typename std::is_invocable_r<R, T, Args...>::type;

#else // WORKAROUND (incomplete)

    template <typename T>
    using Void_t = void;

    template <typename T, typename Sig, typename = void>
    struct IsInvocableImpl : std::false_type
    {
    };

    template <typename T, typename Ret, typename ...Args>
    struct IsInvocableImpl<T, Ret (Args...), Void_t< std::result_of<T&& (Args&&...)> >>
        : std::is_convertible< std::result_of_t<T&& (Args&&...)>, Ret >
    {
    };

    template <typename R, typename T, typename ...Args>
    using IsInvocableR = typename IsInvocableImpl<T, R(Args...)>::type;

#endif
}

template <typename Parser, typename ...Args>
auto Cmdline::Add(Parser&& parser, char const* name, Args&&... args)
{
    using DecayedParser = std::decay_t<Parser>;

    static_assert(
        impl::IsInvocableR<bool, DecayedParser, ParseContext&>::value ||
        impl::IsInvocableR<void, DecayedParser, ParseContext&>::value,
        "The parser must be invocable with an argument of type "
        "'ParseContext&' and the return type should be convertible "
        "to 'bool'");

    auto opt = std::make_unique<Option<DecayedParser>>( std::forward<Parser>(parser), name, std::forward<Args>(args)... );

    const auto p = opt.get();
    DoAdd(p);
    unique_options_.push_back(std::move(opt)); // commit

    return p;
}

template <typename It>
bool Cmdline::Parse(It first, It last, CheckMissing check_missing)
{
    auto sink = [&](It curr, int index)
    {
        cxx::string_view optstr {*curr};
        EmitDiag(Diagnostic::error, index, "Unknown option '" + std::string(*curr) + "'");
        return false;
    };

    return Parse(first, last, check_missing, sink);
}

template <typename It, typename Sink>
bool Cmdline::Parse(It first, It last, CheckMissing check_missing, Sink sink)
{
    if (!DoParse(first, last, sink))
        return false;

    if (check_missing == CheckMissing::yes)
        return CheckMissingOptions();

    return true;
}

template <typename It, typename Sink>
bool Cmdline::DoParse(It& curr, It last, Sink sink)
{
    assert(curr_positional_ >= 0);
    assert(curr_index_ >= 0);

    while (curr != last)
    {
        Result const res = Handle1(curr, last);

        if (res == Result::error)
            return false;

        if (res == Result::ignored && !sink(curr, curr_index_))
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

template <typename It>
Cmdline::Result Cmdline::Handle1(It& curr, It last)
{
    assert(curr != last);

    cxx::string_view optstr(*curr);

    // This cannot happen if we're parsing the argv[] arrray, but it might
    // happen if we're parsing a user-supplied array of command line arguments.
    if (optstr.empty())
        return Result::success;

    // Stop parsing if "--" has been found
    if (optstr == "--" && !dashdash_)
    {
        dashdash_ = true;
        return Result::success;
    }

    // This argument is considered to be positional if it doesn't start with
    // '-', if it is "-" itself, or if we have seen "--" already.
    if (optstr[0] != '-' || optstr == "-" || dashdash_)
        return HandlePositional(optstr);

    // Starts with a dash, must be an option.

    optstr.remove_prefix(1); // Remove the first dash.

    // If the name starts with a single dash, this is a short option and might
    // actually be an option group.
    bool const is_short = (optstr[0] != '-');

    if (!is_short)
        optstr.remove_prefix(1); // Remove the second dash.

    Result res = HandleStandardOption(optstr, curr, last);

    if (res == Result::ignored)
        res = HandleOption(optstr);

    if (res == Result::ignored)
        res = HandlePrefix(optstr);

    if (res == Result::ignored && is_short /* && arg.size() >= 2 */)
        res = HandleGroup(optstr, curr, last);

    return res;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It>
Cmdline::Result Cmdline::HandleStandardOption(cxx::string_view optstr, It& curr, It last)
{
    bool ambiguous = false;
    if (auto const opt = FindOption(optstr, ambiguous))
    {
        if (ambiguous)
        {
            EmitDiag(Diagnostic::error, curr_index_, "Option '" + std::string(optstr) + "' is ambiguous");
            return Result::error;
        }

        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::ignored;
}

template <typename It>
Cmdline::Result Cmdline::HandleGroup(cxx::string_view optstr, It& curr, It last)
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
            ++arg_start;

        auto const arg = optstr.substr(arg_start);
        return HandleOccurrence(opt, name, arg);
    }

    return Result::success;
}

template <typename It>
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, cxx::string_view name, It& curr, It last)
{
    assert(curr != last);

    cxx::string_view arg;

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    bool err = opt->join_arg_ == JoinArg::yes;

    if (!err && opt->num_args_ == Arg::required)
    {
        ++curr;
        ++curr_index_;

        if (curr == last)
            err = true;
        else
            arg = cxx::string_view(*curr);
    }

    if (err)
    {
        EmitDiag(Diagnostic::error, curr_index_, "Option '" + std::string(name) + "' requires an argument");
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
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

template <typename T = void, typename /*Enable*/ = void>
struct ParseValue
{
    bool operator()(ParseContext const& ctx, T& value) const
    {
        std::stringstream stream { std::string(ctx.arg) };

        stream.setf(std::ios_base::fmtflags(0), std::ios::basefield);
        stream >> value;

        return !stream.fail() && stream.eof();
    }
};

template <>
struct ParseValue<bool>
{
    bool operator()(ParseContext const& ctx, bool& value) const;
};

template <>
struct ParseValue<std::string>
{
    bool operator()(ParseContext const& ctx, std::string& value) const;
};

template <>
struct ParseValue<void>
{
    template <typename T>
    bool operator()(ParseContext& ctx, T& value) const
    {
        return ParseValue<T>{}(ctx, value);
    }
};

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

//
// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
//
template <typename T, typename ...Predicates>
auto Value(T& value, Predicates&&... preds)
{
    return [=, &value](ParseContext& ctx)
    {
#if __cplusplus >= 201703
        return ParseValue<>{}(ctx, value) && (... && preds(ctx, value));
#else
        bool res = true;
        int unused[] = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
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
auto List(T& container, Predicates&&... preds)
{
    return [=, &container](ParseContext& ctx)
    {
        typename T::value_type value;

#if __cplusplus >= 201703
        if (ParseValue<>{}(ctx, value) && (... && preds(ctx, value)))
#else
        bool res = true;
        int unused[] = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
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
template <typename T>
auto Map(T& target, std::vector<std::pair<char const*, T>> map_)
{
    return [&target, map = std::move(map_)](ParseContext& ctx)
    {
        for (auto const& p : map)
        {
            if (p.first == ctx.arg)
            {
                target = p.second;
                return true;
            }
        }

        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "Invalid argument '" + std::string(ctx.arg) + "' for  option '" + std::string(ctx.name) + "'");
        for (auto const& p : map)
        {
            ctx.cmdline->EmitDiag(Diagnostic::note, ctx.index, std::string("Could be '") + p.first + "'");
        }

        return false;
    };
}

} // namespace cl

#endif // CL_CMDLINE_H
