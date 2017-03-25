// Distributed under the MIT license. See the end of the file for details.

#pragma once

#include <cassert>
#include <memory>
#include <sstream>
#include <string>
#if _MSC_VER
#  include <string_view>
#else
#  if __has_include(<string_view>)
#    include <string_view>
#  else
#    include <experimental/string_view>
     namespace std { using std::experimental::string_view; }
#  endif
#endif
#include <vector>

namespace cl {

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Flags controling how often an option may/must be specified.
enum class Opt : unsigned char {
    // The option may appear at most once
    Optional,
    // The option must appear exactly once.
    Required,
    // The option may appear multiple times.
    ZeroOrMore,
    // The option must appear at least once.
    OneOrMore,
};

// Flags controling the number of arguments the option accepts.
enum class Arg : unsigned char {
    // An argument is not allowed
    Disallowed,
    // An argument is optional.
    Optional,
    // An argument is required.
    Required,
};

// Controls whether the option may/must join its argument.
enum class JoinArg : unsigned char {
    // The option must not join its argument: "-I dir" and "-I=dir" are
    // possible.
    // The equals sign (if any) will not be a part of the option argument.
    No,
    // The option may join its argument: "-I dir" and "-Idir" are possible.
    // If the option is specified with an equals sign ("-I=dir") the '=' will
    // be part of the option argument.
    Optional,
    // The option must join its argument: "-Idir" is the only possible format.
    // If the option is specified with an equals sign ("-I=dir") the '=' will
    // be part of the option argument.
    Yes,
};

// May this option group with other options?
enum class MayGroup : unsigned char {
    // The option may not be grouped with other options (even if the option
    // name consists only of a single letter).
    No,
    // The option may be grouped with other options.
    // This flag is ignored if the name of the options is not a single letter
    // and option groups must be prefixed with a single '-', e.g. "-xvf=file".
    Yes,
};

// Positional option?
enum class Positional : unsigned char {
    // The option is not a positional option, i.e. requires '-' or '--' as a
    // prefix when specified.
    No,
    // Positional option, no '-' required.
    Yes,
};

// Split the argument between commas?
enum class CommaSeparatedArg : unsigned char {
    // Do not split the argument between commas.
    No,
    // If this flag is set, the option's argument is split between commas,
    // e.g. "-i=1,2,,3" will be parsed as ["-i=1", "-i=2", "-i=", "-i=3"].
    // Note that each comma-separated argument counts as an option occurrence.
    Yes,
};

// Parse all following options as positional options?
enum class ConsumeRemaining : unsigned char {
    // Nothing special.
    No,
    // If an option with this flag is (successfully) parsed, all the remaining
    // options are parsed as positional options.
    Yes,
};

// Steal arguments from the command line?
enum class SeparateArg : unsigned char {
    // Do not steal arguments from the command line.
    Disallowed,
    // If the option requires an argument and is not specified as "-i=1",
    // the parser may steal the next argument from the command line.
    // This allows options to be specified as "-i 1" instead of "-i=1".
    Optional,
};

// Controls whether the Parse() methods should check for missing options.
enum class CheckMissing : unsigned char {
    // Do not check for missing required options in Cmdline::Parse().
    No,
    // Check for missing required options in Cmdline::Parse().
    // Any missing option will be reported as an error.
    Yes,
};

// Controls whether the Parse() methods should ignore unknown options.
enum class IgnoreUnknown : unsigned char {
    // Don't ignore unknown options (default).
    No,
    // Ignore unknown options.
    Yes,

    //
    // XXX:
    //
    // There should be a way to collect ignored options...
    // Or rotate the input range...
    //
};

template <typename T = void>
struct ParseValue
{
    bool operator()(std::string_view /*name*/, std::string_view arg, int /*index*/, T& value) const
    {
        std::stringstream stream;

        stream.write(arg.data(), static_cast<std::streamsize>(arg.size()));
        stream.setf(std::ios_base::fmtflags(0), std::ios::basefield);
        stream >> value;

        return stream && stream.eof();
    }
};

template <>
struct ParseValue<bool>
{
    bool operator()(std::string_view /*name*/, std::string_view arg, int /*index*/, bool& value) const
    {
        if (arg.empty() || arg == "1" || arg == "true" || arg == "on" || arg == "yes")
            value = true;
        else if (arg == "0" || arg == "false" || arg == "off" || arg == "no")
            value = false;
        else
            return false;

        return true;
    }
};

template <>
struct ParseValue<std::string>
{
    bool operator()(std::string_view /*name*/, std::string_view arg, int /*index*/, std::string& value) const
    {
        value.assign(arg.data(), arg.size());
        return true;
    }
};

template <>
struct ParseValue<void>
{
    template <typename T>
    bool operator()(std::string_view name, std::string_view arg, int index, T& value) const {
        return ParseValue<T>{}(name, arg, index, value);
    }
};

// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
template <typename T>
auto Value(T& value)
{
    return [&](std::string_view name, std::string_view arg, int index) {
        return ParseValue<>{}(name, arg, index, value);
    };
}

// Default parser for list types.
// Uses an instance of Parser<> to convert the string and then inserts the
// converted value into the container.
template <typename T>
auto List(T& container)
{
    return [&](std::string_view name, std::string_view arg, int index)
    {
        typename T::value_type value;

        if (!ParseValue<>{}(name, arg, index, value))
            return false;

        container.insert(container.end(), std::move(value));
        return true;
    };
}

class OptionBase
{
    friend class Cmdline;

    //
    // XXX:
    //
    // Everything but count_ should be const...
    //

    // The name of the option.
    std::string_view const name_;
    // Flags controlling how the option may/must be specified.
    Opt               num_occurrences_     = Opt::Optional;
    Arg               num_args_            = Arg::Disallowed;
    JoinArg           join_arg_            = JoinArg::No;
    MayGroup          may_group_           = MayGroup::No;
    Positional        positional_          = Positional::No;
    CommaSeparatedArg comma_separated_arg_ = CommaSeparatedArg::No;
    ConsumeRemaining  consume_remaining_   = ConsumeRemaining::No;
    SeparateArg       separate_arg_        = SeparateArg::Optional;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    void Apply(Opt               v) { num_occurrences_ = v; }
    void Apply(Arg               v) { num_args_ = v; }
    void Apply(JoinArg           v) { join_arg_ = v; }
    void Apply(MayGroup          v) { may_group_ = v; }
    void Apply(Positional        v) { positional_ = v; }
    void Apply(CommaSeparatedArg v) { comma_separated_arg_ = v; }
    void Apply(ConsumeRemaining  v) { consume_remaining_ = v; }
    void Apply(SeparateArg       v) { separate_arg_ = v; }

protected:
    template <typename ...Flags>
    explicit OptionBase(char const* name, Flags&&... flags)
        : name_(name)
    {
        int const unused[] = { (Apply(flags), 0)..., 0 };
        static_cast<void>(unused);
    }

public:
    virtual ~OptionBase();

public:
    // Returns the name of this option
    std::string_view name() const { return name_; }

    // Returns the number of times this option was specified on the command line
    int count() const { return count_; }

private:
    bool IsOccurrenceAllowed() const;
    bool IsOccurrenceRequired() const;

    // Parse the given value from NAME and/or ARG and store the result.
    // Return true on success, false otherwise.
    virtual bool Parse(std::string_view name, std::string_view arg, int index) = 0;
};

template <typename ParserT>
class Option : public OptionBase
{
    ParserT const parser_;

public:
    template <typename P, typename ...Flags>
    Option(P&& parser, char const* name, Flags&&... flags)
        : OptionBase(name, std::forward<Flags>(flags)...)
        , parser_(std::forward<P>(parser))
    {
    }

    ParserT const& parser() const { return parser_; }

private:
    bool Parse(std::string_view name, std::string_view arg, int index) override {
        return parser_(name, arg, index);
    }
};

class Cmdline
{
    using Options = std::vector<std::unique_ptr<OptionBase>>;

    // Output stream for error messages
    std::ostream* const diag_ = nullptr;
    // List of options. Includes the positional options (in order).
    Options options_;
    // Maximum length of the names of all prefix options
    int max_prefix_len_ = 0;
    // The current positional argument - if any
    Options::iterator curr_positional_;
    // Index of the current argument
    int curr_index_ = -1;
    // "--" seen?
    bool dashdash_ = false;

public:
    explicit Cmdline(std::ostream* diag = nullptr);
    ~Cmdline();

    Cmdline(Cmdline const&) = delete;
    Cmdline& operator =(Cmdline const&) = delete;

    // List of options. Includes the positional options (in order).
    Options const& options() const { return options_; }

    //
    // XXX:
    //
    //  - Return the actual Option<...>* instead of OptionBase* ???
    //    Would allow accessing the stored parser...
    //  - Some enable_if-magic to check the parser is callable?
    //

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    template <typename ParserT, typename ...Flags>
    OptionBase* Add(ParserT&& parser, char const* name, Flags&&... flags);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    //
    // Same as for Add(Value(target), name, args...)
    template <typename T, typename ...Flags>
    OptionBase* AddValue(T& target, char const* name, Flags&&... flags);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    //
    // Same as for Add(List(target), name, Opt::ZeroOrMore, args...)
    template <typename T, typename ...Flags>
    OptionBase* AddList(T& target, char const* name, Flags&&... flags);

    // Parse the command line arguments in [first, last)
    template <typename It>
    bool Parse(
        It first,
        It last,
        CheckMissing check_missing = CheckMissing::Yes,
        IgnoreUnknown ignore_unknown = IgnoreUnknown::No);

    // Parse the command line arguments in [first, last)
    template <typename It>
    bool ContinueParse(
        It first,
        It last,
        CheckMissing check_missing = CheckMissing::No,
        IgnoreUnknown ignore_unknown = IgnoreUnknown::No);

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if all required options have been (successfully) parsed.
    bool CheckMissingOptions();

private:
    enum class Result { Success, Error, Ignored };

    // O(n)
    OptionBase* FindOption(std::string_view name) const;

    void DoAdd(std::unique_ptr<OptionBase> opt);

    template <typename It>
    bool DoParse(It& curr, It last, IgnoreUnknown ignore_unknown);

    template <typename It>
    Result Handle1(It& curr, It last);

    // <file>
    Result HandlePositional(std::string_view optstr);

    // -f
    // -f <file>
    template <typename It>
    Result HandleStandardOption(std::string_view optstr, It& curr, It last);

    // -f=<file>
    Result HandleOption(std::string_view optstr);

    // -I<dir>
    Result HandlePrefix(std::string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(std::string_view optstr, std::vector<OptionBase*>& group);

    template <typename It>
    Result HandleGroup(std::string_view optstr, It& curr, It last);

    template <typename It>
    Result HandleOccurrence(OptionBase* opt, std::string_view name, It& curr, It last);
    Result HandleOccurrence(OptionBase* opt, std::string_view name, std::string_view arg);

    Result ParseOptionArgument(OptionBase* opt, std::string_view name, std::string_view arg);
};

template <typename ParserT, typename ...Flags>
OptionBase* Cmdline::Add(ParserT&& parser, char const* name, Flags&&... flags)
{
    auto opt = std::make_unique<Option<std::decay_t<ParserT>>>(
        std::forward<ParserT>(parser), name, std::forward<Flags>(flags)...);

    auto const res = opt.get();
    DoAdd(std::move(opt));
    return res;
}

template <typename T, typename ...Flags>
OptionBase* Cmdline::AddValue(T& target, char const* name, Flags&&... flags)
{
    return Add(Value(target), name, std::forward<Flags>(flags)...);
}

template <typename T, typename ...Flags>
OptionBase* Cmdline::AddList(T& target, char const* name, Flags&&... flags)
{
    return Add(List(target), name, Opt::ZeroOrMore, std::forward<Flags>(flags)...);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

inline OptionBase::~OptionBase()
{
}

inline bool OptionBase::IsOccurrenceAllowed() const
{
    if (num_occurrences_ == Opt::Required || num_occurrences_ == Opt::Optional)
        return count_ == 0;
    return true;
}

inline bool OptionBase::IsOccurrenceRequired() const
{
    if (num_occurrences_ == Opt::Required || num_occurrences_ == Opt::OneOrMore)
        return count_ == 0;
    return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

inline Cmdline::Cmdline(std::ostream* diag)
    : diag_(diag)
    , curr_positional_(options_.begin())
{
}

inline Cmdline::~Cmdline()
{
}

template <typename It>
bool Cmdline::Parse(It first, It last, CheckMissing check_missing, IgnoreUnknown ignore_unknown)
{
    curr_positional_ = options_.begin();
    curr_index_      = 0;
    dashdash_        = false;

    return ContinueParse(first, last, check_missing, ignore_unknown);
}

template <typename It>
bool Cmdline::ContinueParse(It first, It last, CheckMissing check_missing, IgnoreUnknown ignore_unknown)
{
    if (!DoParse(first, last, ignore_unknown))
        return false;

    if (check_missing == CheckMissing::Yes)
        return CheckMissingOptions();

    return true;
}

// Emit errors for ALL missing options.
inline bool Cmdline::CheckMissingOptions()
{
    bool res = true;
    for (auto&& opt : options_)
    {
        if (opt->IsOccurrenceRequired())
        {
            if (diag_)
                *diag_ << "error: option '" << opt->name_ << "' missing\n";
            res = false;
        }
    }

    return res;
}

inline OptionBase* Cmdline::FindOption(std::string_view name) const
{
    for (auto&& p : options_)
    {
        if (p->name_ == name)
            return p.get();
    }

    return nullptr;
}

inline void Cmdline::DoAdd(std::unique_ptr<OptionBase> opt)
{
    assert(!opt->name_.empty());

    if (opt->join_arg_ != JoinArg::No)
    {
        int const n = static_cast<int>(opt->name_.size());
        if (max_prefix_len_ < n)
            max_prefix_len_ = n;
    }

    options_.push_back(std::move(opt));

    curr_positional_ = options_.begin();
}

template <typename It>
bool Cmdline::DoParse(It& curr, It last, IgnoreUnknown ignore_unknown)
{
    while (curr != last)
    {
        Result const res = Handle1(curr, last);

        if (res == Result::Error)
            return false;

        if (res == Result::Ignored && ignore_unknown == IgnoreUnknown::No)
        {
            if (diag_)
                *diag_ << "error(" << curr_index_ << "): unkown option '" << *curr << "'\n";
            return false;
        }

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

    std::string_view optstr(*curr);

    // This cannot happen if we're parsing the argv[] arrray, but it might
    // happen if we're parsing a user-supplied array of command line
    // arguments.
    if (optstr.empty())
        return Result::Success;

    // Stop parsing if "--" has been found
    if (optstr == "--" && !dashdash_)
    {
        dashdash_ = true;
        return Result::Success;
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

    if (res == Result::Ignored)
        res = HandleOption(optstr);

    if (res == Result::Ignored)
        res = HandlePrefix(optstr);

    if (res == Result::Ignored && is_short /* && arg.size() >= 2 */)
        res = HandleGroup(optstr, curr, last);

    return res;
}

inline Cmdline::Result Cmdline::HandlePositional(std::string_view optstr)
{
    for (auto E = options_.end(); curr_positional_ != E; ++curr_positional_)
    {
        auto&& opt = *curr_positional_;

        if (opt->positional_ == Positional::Yes && opt->IsOccurrenceAllowed())
        {
            // The "argument" of a positional option, is the option name itself.
            return HandleOccurrence(opt.get(), opt->name_, optstr);
        }
    }

    return Result::Ignored;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It>
Cmdline::Result Cmdline::HandleStandardOption(std::string_view optstr, It& curr, It last)
{
    if (auto const opt = FindOption(optstr))
    {
        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::Ignored;
}

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
inline Cmdline::Result Cmdline::HandleOption(std::string_view optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != std::string_view::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        if (auto opt = FindOption(name))
        {
            // Ok, something like "-f=file".

            // Discard the equals sign if this option may NOT join its value.
            if (opt->join_arg_ == JoinArg::No)
            {
                ++arg_start;
            }

            auto const arg = optstr.substr(arg_start);
            return HandleOccurrence(opt, name, arg);
        }
    }

    return Result::Ignored;
}

inline Cmdline::Result Cmdline::HandlePrefix(std::string_view optstr)
{
    // Scan over all known prefix lengths.
    // Start with the longest to allow different prefixes like e.g. "-with"
    // and "-without".

    size_t n = static_cast<size_t>(max_prefix_len_);
    if (n > optstr.size())
        n = optstr.size();

    for ( ; n != 0; --n)
    {
        auto const name = optstr.substr(0, n);
        auto const opt = FindOption(name);

        if (opt && opt->join_arg_ != JoinArg::No)
        {
            auto const arg = optstr.substr(n);
            return HandleOccurrence(opt, name, arg);
        }
    }

    return Result::Ignored;
}

// Check if OPTSTR is actually a group of single letter options and store
// the options in GROUP.
inline Cmdline::Result Cmdline::DecomposeGroup(std::string_view optstr, std::vector<OptionBase*>& group)
{
    group.reserve(optstr.size());

    for (size_t n = 0; n < optstr.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->may_group_ == MayGroup::No)
            return Result::Ignored;

        // We have a single letter option which does not accept an argument.
        // This is ok.
        if (opt->num_args_ == Arg::Disallowed)
        {
            group.push_back(opt);
            continue;
        }

        assert(opt->num_args_ == Arg::Optional || opt->num_args_ == Arg::Required);

        // The option accepts an argument. This terminates the option group.
        // It is a valid option if it is the last option in the group, or if the
        // next character is an equal sign, or if the option may join its
        // argument.
        if (n + 1 == optstr.size() || optstr[n + 1] == '=' || opt->join_arg_ != JoinArg::No)
        {
            group.push_back(opt);
            break;
        }

        if (diag_)
            *diag_ << "error(" << curr_index_ << "): option '" << optstr[n] << "' must be last in a group (requires an argument)\n";
        return Result::Error;
    }

    return Result::Success;
}

template <typename It>
Cmdline::Result Cmdline::HandleGroup(std::string_view optstr, It& curr, It last)
{
    std::vector<OptionBase*> group;

    // First determine if this is a valid option group.
    auto const res = DecomposeGroup(optstr, group);

    if (res != Result::Success)
        return res;

    // Then process all options.
    for (size_t n = 0; n < group.size(); ++n)
    {
        auto const opt = group[n];
        auto const name = optstr.substr(n, 1);

        if (opt->num_args_ == Arg::Disallowed || n + 1 == optstr.size())
        {
            if (Result::Success != HandleOccurrence(opt, name, curr, last))
                return Result::Error;
            continue;
        }

        // Almost done. Process the last option which accepts an argument.

        size_t arg_start = n + 1;

        // If the next character is '=' and the option may not join its
        // argument, discard the equals sign.
        if (optstr[arg_start] == '=' && opt->join_arg_ == JoinArg::No)
        {
            ++arg_start;
        }

        auto const arg = optstr.substr(arg_start);
        return HandleOccurrence(opt, name, arg);
    }

    return Result::Success;
}

template <typename It>
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std::string_view name, It& curr, It last)
{
    assert(curr != last);

    std::string_view arg;

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    // If the option might not steal its argument from the command line, this
    // is an error.
    bool err = opt->join_arg_ == JoinArg::Yes || opt->separate_arg_ == SeparateArg::Disallowed;

    if (!err && opt->num_args_ == Arg::Required)
    {
        ++curr;
        ++curr_index_;

        if (curr == last)
            err = true;
        else
            arg = std::string_view(*curr);
    }

    if (err)
    {
        if (diag_)
            *diag_ << "error(" << curr_index_ << "): option '" << name << "' requires an argument\n";
        return Result::Error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std::string_view name, std::string_view arg)
{
    // An argument was specified for OPT.

    if (opt->num_args_ == Arg::Disallowed)
    {
        if (diag_)
            *diag_ << "error(" << curr_index_ << ": option '" << name << "' does not accept an argument\n";
        return Result::Error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, std::string_view name, std::string_view arg)
{
    auto Parse1 = [&](std::string_view arg1)
    {
        if (!opt->IsOccurrenceAllowed())
        {
            if (diag_)
                *diag_ << "error(" << curr_index_ << "): option '" << name << "' already specified\n";
            return Result::Error;
        }

        if (!opt->Parse(name, arg1, curr_index_))
        {
            if (diag_)
                *diag_ << "error(" << curr_index_ << "): invalid argument '" << arg1 << "' for option '" << name << "'\n";
            return Result::Error;
        }

        ++opt->count_;
        return Result::Success;
    };

    Result res = Result::Success;

    if (opt->comma_separated_arg_ == CommaSeparatedArg::Yes)
    {
        for (;;)
        {
            // Don't skip empty arguments: "1," => ["1", ""]

            auto const p = arg.find(',');
            if (p == std::string_view::npos)
            {
                // Last argument. Result checked below.
                res = Parse1(arg);
                break;
            }

            res = Parse1(arg.substr(0, p));
            if (res != Result::Success)
                break;

            arg.remove_prefix(p + 1); // +1 = comma
        }
    }
    else
    {
        res = Parse1(arg);
    }

    if (res == Result::Success)
    {
        // If the current option has the ConsumeRemaining flag set,
        // parse all following options as positional options.
        if (opt->consume_remaining_ == ConsumeRemaining::Yes)
        {
            dashdash_ = true;
        }
    }

    return res;
}

} // namespace cl

//------------------------------------------------------------------------------
// Copyright 2017 A. Bolz
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
