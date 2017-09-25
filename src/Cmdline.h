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

#if defined(__has_include)
#define CL_HAS_INCLUDE(X) __has_include(X)
#else
#define CL_HAS_INCLUDE(X) 0
#endif

#if (CL_HAS_INCLUDE(<string_view>) && __cplusplus >= 201703) || (_MSC_VER >= 1910 && _HAS_CXX17)
#define CL_HAS_STD_STRING_VIEW 1
#include <string_view>
#endif

#if (CL_HAS_INCLUDE(<experimental/string_view>) && __cplusplus > 201103)
#define CL_HAS_STD_EXPERIMENTAL_STRING_VIEW 1
#include <experimental/string_view>
#endif

namespace cl {

#if CL_HAS_STD_STRING_VIEW
using std::string_view;
#elif CL_HAS_STD_EXPERIMENTAL_STRING_VIEW
using std::experimental::string_view;
#else
#error "The Cmdline library requires a <string_view> implementation"
#endif

class Cmdline;

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
enum class EndsOptions : unsigned char {
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
    string_view name_;
    // The description of this option
    string_view descr_;
    // Flags controlling how the option may/must be specified.
    NumOpts        num_opts_        = NumOpts::optional;
    HasArg         has_arg_         = HasArg::no;
    JoinArg        join_arg_        = JoinArg::no;
    MayGroup       may_group_       = MayGroup::no;
    Positional     positional_      = Positional::no;
    CommaSeparated comma_separated_ = CommaSeparated::no;
    EndsOptions    ends_options_    = EndsOptions::no;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    template <typename T> void Apply(T) = delete; // For slightly more useful error messages...

    void Apply(NumOpts        v) { num_opts_ = v; }
    void Apply(HasArg         v) { has_arg_ = v; }
    void Apply(JoinArg        v) { join_arg_ = v; }
    void Apply(MayGroup       v) { may_group_ = v; }
    void Apply(Positional     v) { positional_ = v; }
    void Apply(CommaSeparated v) { comma_separated_ = v; }
    void Apply(EndsOptions    v) { ends_options_ = v; }

protected:
    template <typename ...Args>
    explicit OptionBase(char const* name, char const* descr, Args&&... args)
        : name_(name)
        , descr_(descr)
    {
#if __cplusplus >= 201703 || __cpp_fold_expressions >= 201411
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
    string_view name() const { return name_; }

    // Returns the description of this option
    string_view descr() const { return descr_; }

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
    Option(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
        : OptionBase(name, descr, std::forward<Args>(args)...)
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

#if __cplusplus >= 201703 || __cpp_deduction_guides >= 201606

template <typename ParserInit, typename Args>
Option(char const*, char const*, ParserInit&&, Args&&...) -> Option<std::decay_t<ParserInit>>;

#endif

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
        string_view name   = {}; // Points into option->name_
        OptionBase* option = nullptr;
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

#ifdef __GNUC__
    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...) __attribute__((format(printf, 4, 5)));
#else
    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...);
#endif

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    // The Cmdline object owns this option.
    template <typename Parser, typename ...Args>
    auto Add(char const* name, char const* descr, Parser&& parser, Args&&... args);

    // Add an option to the commond line.
    // The Cmdline object takes ownership.
    void Add(std::unique_ptr<OptionBase> opt);

    // Add an option to the commond line.
    // The Cmdline object does not own this option.
    void Add(OptionBase* opt);
    void Add(std::initializer_list<OptionBase*> opts);

    // Resets the parser. Sets the COUNT members of all registered options to 0.
    void Reset();

    // Parse the command line arguments in ARGS.
    // Emits an error for unknown options.
    bool Parse(std::vector<std::string> const& args, bool check_missing = true);

#ifdef _WIN32
    // Parse the command line obtained from GetCommandLineW and CommandLineToArgvW.
    // The program-name is discarded.
    // The arguments are converted to UTF-8 before parsing.
    bool ParseCommandLine(bool check_missing = true);
#endif

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if any required options have not yet been (successfully) parsed.
    bool AnyMissing();

    // Prints error messages to stderr.
    void PrintDiag() const;

    // Returns a short help message listing all registered options.
    std::string FormatHelp(string_view program_name, size_t indent = 2, size_t descr_indent = 27, size_t max_width = 100) const;

    // Prints the help message to stderr
    void PrintHelp(string_view program_name, size_t indent = 2, size_t descr_indent = 27, size_t max_width = 100) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(string_view name) const;

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

    template <typename Fn>
    bool ForEachUniqueOption(Fn fn) const;
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename Parser, typename ...Args>
auto Cmdline::Add(char const* name, char const* descr, Parser&& parser, Args&&... args)
{
    using DecayedParser = std::decay_t<Parser>;

    auto opt = std::make_unique<Option<DecayedParser>>(name, descr, std::forward<Parser>(parser), std::forward<Args>(args)...);
    auto const p = opt.get();

    Add(std::move(opt));

    return p;
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

template <typename Alloc>
struct ConvertValue<std::basic_string<char, std::char_traits<char>, Alloc>>
{
    bool operator()(string_view str, std::basic_string<char, std::char_traits<char>, Alloc>& value) const
    {
        value.assign(str.data(), str.size());
        return true;
    }
};

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
template <typename T, typename U>
auto InRange(T lower, U upper)
{
    return [=](ParseContext& /*ctx*/, auto const& value) {
        return !(value < lower) && !(upper < value);
    };
}

// Returns a function object which checks whether a given value is > lower.
template <typename T>
auto GreaterThan(T lower)
{
    return [=](ParseContext& /*ctx*/, auto const& value) {
        return lower < value;
    };
}

// Returns a function object which checks whether a given value is >= lower.
template <typename T>
auto GreaterEqual(T lower)
{
    return [=](ParseContext& /*ctx*/, auto const& value) {
        return !(value < lower); // value >= lower
    };
}

// Returns a function object which checks whether a given value is < upper.
template <typename T>
auto LessThan(T upper)
{
    return [=](ParseContext& /*ctx*/, auto const& value) {
        return value < upper;
    };
}

// Returns a function object which checks whether a given value is <= upper.
template <typename T>
auto LessEqual(T upper)
{
    return [=](ParseContext& /*ctx*/, auto const& value) {
        return !(upper < value); // upper >= value
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

#if __cplusplus >= 201703 || __cpp_fold_expressions >= 201411
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
                return impl::ApplyFuncs(ctx, value, preds...);
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
