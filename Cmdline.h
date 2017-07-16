#pragma once

#include "__string_view.h"

#include <cassert>
#include <memory> // unique_ptr
#include <sstream>
#include <string>
#include <vector>

namespace cl {
inline namespace v1 {

// Controls how often an option may/must be specified.
enum class Opt : unsigned char {
    // The option may appear at most once
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
    // An argument is not allowed
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
    no,
    // Positional option, no '-' required.
    yes,
};

// Split the argument between commas?
enum class CommaSeparatedArg : unsigned char {
    // Do not split the argument between commas.
    no,
    // If this flag is set, the option's argument is split between commas, e.g.
    // "-i=1,2,,3" will be parsed as ["-i=1", "-i=2", "-i=", "-i=3"].
    // Note that each comma-separated argument counts as an option occurrence.
    yes,
};

// Parse all following options as positional options?
enum class ConsumeRemaining : unsigned char {
    // Nothing special.
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
    yes,
};

// Contains the help text for an option.
struct Descr
{
    char const* s;
    explicit Descr(char const* s) : s(s) {}
};

struct ParseContext
{
    std__string_view name;  // in:  Name of the option being parsed
    std__string_view arg;   // in:  Option argument
    int              index; // in:  Current index in the argv array
    std::string      diag;  // out: Optional error message
};

template <typename T = void>
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
    bool operator()(ParseContext const& ctx, bool& value) const
    {
        if (ctx.arg.empty() || ctx.arg == "1" ||  ctx.arg == "y" || ctx.arg == "true" || ctx.arg == "on" || ctx.arg == "yes")
            value = true;
        else if (ctx.arg == "0" || ctx.arg == "n" || ctx.arg == "false" || ctx.arg == "off" || ctx.arg == "no")
            value = false;
        else
            return false;

        return true;
    }
};

template <>
struct ParseValue<std::string>
{
    bool operator()(ParseContext const& ctx, std::string& value) const
    {
        value.assign(ctx.arg.data(), ctx.arg.size());
        return true;
    }
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
        ctx.diag = "argument must be in the range [" + std::to_string(lower) + ", " + std::to_string(upper) + "]";
        return false;
    };
}

//
// XXX
//
// ParseValue<>::operator() and predicates have the same signature...
// Combine?!?!
//

// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
template <typename T, typename ...Predicates>
auto Value(T& value, Predicates... preds)
{
    return [=, &value](ParseContext& ctx)
    {
#if 1 // WORKAROUND: folding expressions
        auto res = true;
        auto lst = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
        static_cast<void>(lst);
        return res;
#else
        return ParseValue<>{}(ctx, value) && (... && preds(ctx, value));
#endif
    };
}

// Default parser for list types.
// Uses an instance of Parser<> to convert the string and then inserts the
// converted value into the container.
template <typename T, typename ...Predicates>
auto List(T& container, Predicates... preds)
{
    return [=, &container](ParseContext& ctx)
    {
        typename T::value_type value;

#if 1 // WORKAROUND: folding expressions
        auto res = true;
        auto lst = { res = ParseValue<>{}(ctx, value), (res = res && preds(ctx, value))... };
        static_cast<void>(lst);
        if (res)
#else
        if (ParseValue<>{}(ctx, value) && (... && preds(ctx, value)))
#endif
        {
            container.insert(container.end(), std::move(value));
            return true;
        }

        return false;
    };
}

class OptionBase
{
    friend class Cmdline;

    // The name of the option.
    std__string_view name_;
    // The description of this option
    std__string_view descr_;
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
    template <typename T> void Apply(T) = delete;

    void Apply(Opt               v) { num_occurrences_ = v; }
    void Apply(Arg               v) { num_args_ = v; }
    void Apply(JoinArg           v) { join_arg_ = v; }
    void Apply(MayGroup          v) { may_group_ = v; }
    void Apply(Positional        v) { positional_ = v; }
    void Apply(CommaSeparatedArg v) { comma_separated_arg_ = v; }
    void Apply(ConsumeRemaining  v) { consume_remaining_ = v; }
    void Apply(Descr             v) { descr_ = v.s; }
    void Apply(char const*       v) { descr_ = v; }

protected:
    template <typename ...Args>
    explicit OptionBase(char const* name, Args&&... args)
        : name_(name)
    {
#if 1 // WORKAROUND: folding expressions
        auto lst = {(Apply(args), 0)..., 0};
        static_cast<void>(lst);
#else
        (Apply(args), ...);
#endif
    }

public:
    virtual ~OptionBase();

public:
    // Returns the name of this option
    std__string_view name() const { return name_; }

    // Returns the description of this option
    std__string_view descr() const { return descr_; }

    // Returns the number of times this option was specified on the command line
    int count() const { return count_; }

private:
    bool IsOccurrenceAllowed() const;
    bool IsOccurrenceRequired() const;

    // Parse the given value from NAME and/or ARG and store the result.
    // Return true on success, false otherwise.
    virtual bool Parse(ParseContext& ctx) = 0;
};

inline OptionBase::~OptionBase()
{
}

inline bool OptionBase::IsOccurrenceAllowed() const
{
    if (num_occurrences_ == Opt::required || num_occurrences_ == Opt::optional)
        return count_ == 0;
    return true;
}

inline bool OptionBase::IsOccurrenceRequired() const
{
    if (num_occurrences_ == Opt::required || num_occurrences_ == Opt::one_or_more)
        return count_ == 0;
    return false;
}

template <typename Parser>
class Option : public OptionBase
{
    Parser const parser_;

public:
    template <typename ParserInit, typename ...Args>
    Option(ParserInit&& parser, char const* name, Args&&... args)
        : OptionBase(name, std::forward<Args>(args)...)
        , parser_(std::forward<ParserInit>(parser))
    {
    }

    Parser const& parser() const { return parser_; }

private:
    bool Parse(ParseContext& ctx) override
    {
        return DoParse(ctx, std::is_convertible<decltype(parser_(ctx)), bool>{});
    }

    bool DoParse(ParseContext& ctx, /*parser_ returns bool*/ std::true_type)
    {
        return parser_(ctx);
    }

    bool DoParse(ParseContext& ctx, /*parser_ returns bool*/ std::false_type)
    {
        parser_(ctx);
        return true;
    }
};

class Cmdline
{
public:
    struct Diagnostic
    {
        enum class Type { error, note };

        Type type = Type::error;
        int index = -1;
        std::string message = {};

        // Returns a string representation of this diagnostic message.
        std::string str() const;
    };

private:
    struct NameOptionPair
    {
        std__string_view name;
        OptionBase* option;
    };

    using UniqueOptions = std::vector<std::unique_ptr<OptionBase>>;
    using Options = std::vector<NameOptionPair>;

    // List of diagnostic messages
    std::vector<Diagnostic> diag_;
    // Option storage.
    UniqueOptions unique_options_ {};
    // List of options. Includes the positional options (in order).
    Options options_ {};
    // Maximum length of the names of all prefix options
    int max_prefix_len_ = 0;
    // The current positional argument - if any
    int curr_positional_ = 0;
    // Index of the current argument
    int curr_index_ = 0;
    // "--" seen?
    bool dashdash_ = false;

public:
    Cmdline();
    ~Cmdline();

    Cmdline(Cmdline const&) = delete;
    Cmdline& operator =(Cmdline const&) = delete;

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& diag() const { return diag_; }

    void EmitError(int index, std::string message);
    void EmitNote(int index, std::string message);

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

    // Returns a short help message
    std::string HelpMessage(std::string const& program_name) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(std__string_view name) const;
    OptionBase* FindOption(std__string_view name, bool& ambiguous) const;

    void DoAdd(OptionBase* opt);

    template <typename It, typename Sink>
    bool DoParse(It& curr, It last, Sink sink);

    template <typename It>
    Result Handle1(It& curr, It last);

    // <file>
    Result HandlePositional(std__string_view optstr);

    // -f
    // -f <file>
    template <typename It>
    Result HandleStandardOption(std__string_view optstr, It& curr, It last);

    // -f=<file>
    Result HandleOption(std__string_view optstr);

    // -I<dir>
    Result HandlePrefix(std__string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(
        std__string_view optstr, std::vector<OptionBase*>& group);

    template <typename It>
    Result HandleGroup(std__string_view optstr, It& curr, It last);

    template <typename It>
    Result HandleOccurrence(OptionBase* opt, std__string_view name, It& curr, It last);
    Result HandleOccurrence(OptionBase* opt, std__string_view name, std__string_view arg);

    Result ParseOptionArgument(OptionBase* opt, std__string_view name, std__string_view arg);

    // Does not skip empty tokens.
    template <typename Func>
    static bool SplitString(std__string_view str, char sep, Func func);
};

inline std::string Cmdline::Diagnostic::str() const
{
    std::string stype = (type == Diagnostic::Type::error)
        ? "error"
        : "note";
    std::string sindex = (index >= 0)
        ? "(" + std::to_string(index) + ")"
        : "";

    return stype + sindex + ": " + message;
}

inline Cmdline::Cmdline()
{
}

inline Cmdline::~Cmdline()
{
}

inline void Cmdline::EmitError(int index, std::string message)
{
    diag_.push_back({Diagnostic::Type::error, index, std::move(message)});
}

inline void Cmdline::EmitNote(int index, std::string message)
{
    diag_.push_back({Diagnostic::Type::note, index, std::move(message)});
}

namespace impl
{
#if 1 // WORKAROUND (incomplete)
    template <typename ...Args>
    struct IsInvocableImpl
    {
        struct Any { template <typename T> Any(T&&) {} };

        template <typename F>
        static auto test(F&&) -> decltype(std::declval<F>()(std::declval<Args>()...), std::true_type{});
        static auto test(Any) -> std::false_type;
    };

    template <typename F, typename ...Args>
    struct IsInvocable : decltype(IsInvocableImpl<Args...>::test(std::declval<F>()))
    {
    };
#else
    template <typename F, typename ...Args>
    using IsInvocable = std::is_invocable<F, Args...>;
#endif

#if 1 // WORKAROUND (incomplete)
    template <typename R, typename ...Args>
    struct IsInvocableRImpl
    {
        struct Any { template <typename T> Any(T&&) {} };

        template <typename F>
        static auto test(F&&) -> std::is_convertible< decltype(std::declval<F>()(std::declval<Args>()...)), R >;
        static auto test(Any) -> std::false_type;
    };

    template <typename R, typename F, typename ...Args>
    struct IsInvocableR : decltype(IsInvocableRImpl<R, Args...>::test(std::declval<F>()))
    {
    };
#else
    template <typename R, typename F, typename ...Args>
    using IsInvocableR = std::is_invocable_r<R, F, Args...>;
#endif
}

template <typename Parser, typename ...Args>
auto Cmdline::Add(Parser&& parser, char const* name, Args&&... args)
{
    using DecayedParser = std::decay_t<Parser>;

    static_assert(
        impl::IsInvocableR<bool, DecayedParser, ParseContext&>::value ||
        impl::IsInvocable<DecayedParser, ParseContext&>::value,
        "The parser must be invocable with an argument of type "
        "'ParseContext&' and the return type should be convertible "
        "to 'bool'");

    auto opt = std::make_unique<Option<DecayedParser>>( std::forward<Parser>(parser), name, std::forward<Args>(args)... );

    const auto p = opt.get();
    DoAdd(p);
    unique_options_.push_back(std::move(opt)); // commit

    return p;
}

inline void Cmdline::Reset()
{
    curr_positional_ = 0;
    curr_index_      = 0;
    dashdash_        = false;

    for (auto& opt : unique_options_)
        opt->count_ = 0;
}

template <typename It>
bool Cmdline::Parse(It first, It last, CheckMissing check_missing)
{
    auto sink = [&](It curr, int index)
    {
        EmitError(index, "unkown option '" + std::string(*curr) + "'");
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

// Emit errors for ALL missing options.
inline bool Cmdline::CheckMissingOptions()
{
    bool res = true;

    for (auto const& opt : unique_options_)
    {
        if (opt->IsOccurrenceRequired())
        {
            EmitError(-1, "option '" + std::string(opt->name_) + "' missing");
            res = false;
        }
    }

    return res;
}

inline std::string Cmdline::HelpMessage(std::string const& program_name) const
{
    static constexpr size_t kIndent = 2;
    static constexpr size_t kDescrIndent = 16;

    std::string spos;
    std::string sopt;

    for (auto const& opt : unique_options_)
    {
        if (opt->positional_ == Positional::yes)
        {
            spos += ' ';
            spos.append(opt->name_.data(), opt->name_.size());
        }
        else
        {
            auto const curr_size = sopt.size();

            sopt.append(kIndent, ' ');

            sopt += '-';
            sopt.append(opt->name_.data(), opt->name_.size());
            switch (opt->num_args_)
            {
            case Arg::no:
                break;
            case Arg::optional:
                sopt += "=<arg>";
                break;
            case Arg::required:
                sopt += " <arg>";
                break;
            }

            // Length of: indent + option usage
            auto const len = sopt.size() - curr_size;

            sopt.append(len >= kDescrIndent ? 1u : kDescrIndent - len, ' ');
            sopt.append(opt->descr_.data(), opt->descr_.size());

            sopt += '\n'; // One option per line
        }
    }

    if (sopt.empty())
        return "Usage: " + program_name + spos + '\n';
    else
        return "Usage: " + program_name + " [options]" + spos + "\nOptions:\n";
}

inline OptionBase* Cmdline::FindOption(std__string_view name) const
{
    for (auto&& p : options_)
    {
        if (p.name == name)
            return p.option;
    }

    return nullptr;
}

inline OptionBase* Cmdline::FindOption(std__string_view name, bool& ambiguous) const
{
    ambiguous = false;

#if 0
    OptionBase* opt = nullptr;

    for (auto&& p : options_)
    {
        if (p.name == name) // exact match
        {
            ambiguous = false;
            return p.option;
        }

        if (p.name.size() > name.size() && p.name.substr(0, name.size()) == name)
        {
            if (opt == nullptr)
                opt = p.option;
            else
                ambiguous = true;
        }
    }

    return opt;
#else
    return FindOption(name);
#endif
}

inline void Cmdline::DoAdd(OptionBase* opt)
{
    assert(!opt->name_.empty());

    SplitString(opt->name_, '|', [&](std__string_view name)
    {
        assert(!name.empty());
        assert(FindOption(name) == nullptr); // option already exists?!

        if (opt->join_arg_ != JoinArg::no)
        {
            int const n = static_cast<int>(name.size());
            if (max_prefix_len_ < n)
                max_prefix_len_ = n;
        }

        options_.push_back({name, opt});
        return true;
    });
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

    std__string_view optstr(*curr);

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

inline Cmdline::Result Cmdline::HandlePositional(std__string_view optstr)
{
    int const E = static_cast<int>(options_.size());
    assert(curr_positional_ <= E);

    for ( ; curr_positional_ != E; ++curr_positional_)
    {
        auto&& opt = options_[curr_positional_].option;

        if (opt->positional_ == Positional::yes && opt->IsOccurrenceAllowed())
        {
            // The "argument" of a positional option, is the option name itself.
            return HandleOccurrence(opt, opt->name_, optstr);
        }
    }

    return Result::ignored;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It>
Cmdline::Result Cmdline::HandleStandardOption(std__string_view optstr, It& curr, It last)
{
    bool ambiguous = false;
    if (auto const opt = FindOption(optstr, ambiguous))
    {
        if (ambiguous)
        {
            EmitError(curr_index_, "option '" + std::string(optstr) + "' is ambiguous");
            return Result::error;
        }

        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::ignored;
}

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
inline Cmdline::Result Cmdline::HandleOption(std__string_view optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != std__string_view::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        bool ambiguous = false;
        if (auto const opt = FindOption(name, ambiguous))
        {
            if (ambiguous)
            {
                EmitError(curr_index_, "option '" + std::string(optstr) + "' is ambiguous");
                return Result::error;
            }

            // Ok, something like "-f=file".

            // Discard the equals sign if this option may NOT join its value.
            if (opt->join_arg_ == JoinArg::no)
                ++arg_start;

            auto const arg = optstr.substr(arg_start);
            return HandleOccurrence(opt, name, arg);
        }
    }

    return Result::ignored;
}

inline Cmdline::Result Cmdline::HandlePrefix(std__string_view optstr)
{
    // Scan over all known prefix lengths.
    // Start with the longest to allow different prefixes like e.g. "-with" and
    // "-without".

    size_t n = static_cast<size_t>(max_prefix_len_);
    if (n > optstr.size())
        n = optstr.size();

    for ( ; n != 0; --n)
    {
        auto const name = optstr.substr(0, n);
        auto const opt = FindOption(name);

        if (opt && opt->join_arg_ != JoinArg::no)
        {
            auto const arg = optstr.substr(n);
            return HandleOccurrence(opt, name, arg);
        }
    }

    return Result::ignored;
}

// Check if OPTSTR is actually a group of single letter options and store the
// options in GROUP.
inline Cmdline::Result Cmdline::DecomposeGroup(std__string_view optstr, std::vector<OptionBase*>& group)
{
    group.reserve(optstr.size());

    for (size_t n = 0; n < optstr.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->may_group_ == MayGroup::no)
            return Result::ignored;

        if (opt->num_args_ == Arg::no || n + 1 == optstr.size())
        {
            group.push_back(opt);
            continue;
        }

        // The option accepts an argument. This terminates the option group.
        // It is a valid option if the next character is an equal sign, or if
        // the option may join its argument.
        if (optstr[n + 1] == '=' || opt->join_arg_ != JoinArg::no)
        {
            group.push_back(opt);
            break;
        }

        // The option accepts an argument, but may not join its argument.
        EmitError(curr_index_, std::string("option '") + optstr[n] + "' must be last in a group");
        return Result::error;
    }

    return Result::success;
}

template <typename It>
Cmdline::Result Cmdline::HandleGroup(std__string_view optstr, It& curr, It last)
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
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std__string_view name, It& curr, It last)
{
    assert(curr != last);

    std__string_view arg;

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
            arg = std__string_view(*curr);
    }

    if (err)
    {
        EmitError(curr_index_, "option '" + std::string(name) + "' requires an argument");
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std__string_view name, std__string_view arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->num_args_ == Arg::no)
    {
        EmitError(curr_index_, "option '" + std::string(name) + "' does not accept an argument");
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, std__string_view name, std__string_view arg)
{
    auto Parse1 = [&](std__string_view arg1)
    {
        if (!opt->IsOccurrenceAllowed())
        {
            EmitError(curr_index_, "option '" + std::string(name) + "' already specified");
            return Result::error;
        }

        ParseContext ctx;

        ctx.name  = name;
        ctx.arg   = arg1;
        ctx.index = curr_index_;

        if (!opt->Parse(ctx))
        {
            EmitError(curr_index_, "invalid argument '" + std::string(arg1) + "' for option '" + std::string(name) + "'");
            if (!ctx.diag.empty())
                EmitNote(-1, ctx.diag);
            return Result::error;
        }

        ++opt->count_;
        return Result::success;
    };

    Result res = Result::success;

    if (opt->comma_separated_arg_ == CommaSeparatedArg::yes)
    {
        SplitString(arg, ',', [&](std__string_view s)
        {
            res = Parse1(s);
            return res == Result::success;
        });
    }
    else
    {
        res = Parse1(arg);
    }

    if (res == Result::success)
    {
        // If the current option has the ConsumeRemaining flag set, parse all
        // following options as positional options.
        if (opt->consume_remaining_ == ConsumeRemaining::yes)
            dashdash_ = true;
    }

    return res;
}

template <typename Func>
bool Cmdline::SplitString(std__string_view str, char sep, Func func)
{
    for (;;)
    {
        auto const p = str.find(sep);
        if (!func(str.substr(0, p))) // p == npos is ok here
            return false;
        if (p == std__string_view::npos)
            return true;
        str.remove_prefix(p + 1);
    }
}

} // namespace v1
} // namespace cl
