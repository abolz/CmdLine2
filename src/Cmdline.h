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
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32
#include <windows.h>
#endif

#if __cplusplus >= 201703 || __cpp_deduction_guides >= 201606
#define CL_HAS_DEDUCTION_GUIDES 1
#endif
#if __cplusplus >= 201703 || __cpp_fold_expressions >= 201411 || (_MSC_VER >= 1912 && _HAS_CXX17)
#define CL_HAS_FOLD_EXPRESSIONS 1
#endif
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17) // XXX: stdlib, not compiler...
#define CL_HAS_STD_INVOCABLE 1
#endif

#if __GNUC__
#define CL_ATTRIBUTE_PRINTF(X, Y) __attribute__((format(printf, X, Y)))
#else
#define CL_ATTRIBUTE_PRINTF(X, Y)
#endif

#ifndef CL_DCHECK
#define CL_DCHECK(X) assert(X)
#endif

namespace cl {

class Cmdline;

//==================================================================================================
//
//==================================================================================================

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

//==================================================================================================
//
//==================================================================================================

namespace impl {

// Return type for delimiters. Returned by the ByX delimiters.
//
// +-----+-----+------------+
// ^ tok ^     ^    rest    ^
//       f     f+c
//
// Either FIRST or COUNT must be non-zero.
struct DelimiterResult
{
    size_t first;
    size_t count;
};

struct ByChar
{
    char const ch;

    explicit ByChar(char ch_)
        : ch(ch_)
    {
    }

    DelimiterResult operator()(cxx::string_view const& str) const
    {
        return {str.find(ch), 1};
    }
};

// Breaks a string into lines, i.e. searches for "\n" or "\r" or "\r\n".
struct ByLine
{
    DelimiterResult operator()(cxx::string_view str) const
    {
        auto const first = str.data();
        auto const last = str.data() + str.size();

        // Find the position of the first CR or LF
        auto p = first;
        while (p != last && (*p != '\n' && *p != '\r')) {
            ++p;
        }

        if (p == last) {
            return {cxx::string_view::npos, 0};
        }

        auto const index = static_cast<size_t>(p - first);

        // If this is CRLF, skip the other half.
        if (p[0] == '\r' && p + 1 != last && p[1] == '\n') {
            return {index, 2};
        }

        return {index, 1};
    }
};

// Breaks a string into words, i.e. searches for the first whitespace preceding
// the given length. If there is no whitespace, breaks a single word at length
// characters.
struct ByMaxLength
{
    size_t const length;

    explicit ByMaxLength(size_t length_)
        : length(length_)
    {
        CL_DCHECK(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(cxx::string_view str) const
    {
        // If the string fits into the current line, just return this last line.
        if (str.size() <= length) {
            return {cxx::string_view::npos, 0};
        }

        // Otherwise, search for the first space preceding the line length.
        auto I = str.find_last_of(" \t", length);

        if (I != cxx::string_view::npos) { // There is a space.
            return {I, 1};
        }

        return {length, 0}; // No space in current line, break at length.
    }
};

struct DoSplitResult
{
    cxx::string_view tok; // The current token.
    cxx::string_view str; // The rest of the string.
    bool             last = false;
};

inline bool DoSplit(DoSplitResult& res, cxx::string_view str, DelimiterResult del)
{
    if (del.first == cxx::string_view::npos)
    {
        res.tok = str;
        //res.str = {};
        res.last = true;
        return true;
    }

    CL_DCHECK(del.first + del.count >= del.first);
    CL_DCHECK(del.first + del.count <= str.size());

    size_t const off = del.first + del.count;
    CL_DCHECK(off > 0 && "invalid delimiter result");

    res.tok = cxx::string_view(str.data(), del.first);
    res.str = cxx::string_view(str.data() + off, str.size() - off);
    return true;
}

// Split the string STR into substrings using the Delimiter (or Tokenizer) SPLIT
// and call FN for each substring.
// FN must return void or bool. If FN returns false, this method stops splitting
// the input string and returns false, too. Otherwise, returns true.
template <typename Splitter, typename Function>
bool Split(cxx::string_view str, Splitter&& split, Function&& fn)
{
    DoSplitResult curr;

    curr.tok = cxx::string_view();
    curr.str = str;
    curr.last = false;

    for (;;)
    {
        if (!impl::DoSplit(curr, curr.str, split(curr.str))) {
            return true;
        }
        if (!fn(curr.tok)) {
            return false;
        }
        if (curr.last) {
            return true;
        }
    }
}

} // namespace impl

//==================================================================================================
//
//==================================================================================================

// Provides information about the argument and the command line parser which
// is currently parsing the arguments.
// The members are only valid inside the callback (parser).
struct ParseContext
{
    cxx::string_view name;    // Name of the option being parsed    (only valid in callback!)
    cxx::string_view arg;     // Option argument                    (only valid in callback!)
    int              index;   // Current index in the argv array
    Cmdline*         cmdline; // The command line parser which currently parses the argument list (never null)
};

//==================================================================================================
//
//==================================================================================================

class OptionBase
{
    friend class Cmdline;

    // The name of the option.
    cxx::string_view name_;
    // The description of this option
    cxx::string_view descr_;
    // Flags controlling how the option may/must be specified.
    NumOpts        num_opts_ = NumOpts::optional;
    HasArg         has_arg_ = HasArg::no;
    JoinArg        join_arg_ = JoinArg::no;
    MayGroup       may_group_ = MayGroup::no;
    Positional     positional_ = Positional::no;
    CommaSeparated comma_separated_ = CommaSeparated::no;
    EndsOptions    ends_options_ = EndsOptions::no;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    template <typename T>
    void Apply(T) = delete; // For slightly more useful error messages...

    void Apply(NumOpts        v) { num_opts_ = v; }
    void Apply(HasArg         v) { has_arg_ = v; }
    void Apply(JoinArg        v) { join_arg_ = v; }
    void Apply(MayGroup       v) { may_group_ = v; }
    void Apply(Positional     v) { positional_ = v; }
    void Apply(CommaSeparated v) { comma_separated_ = v; }
    void Apply(EndsOptions    v) { ends_options_ = v; }

protected:
    template <typename... Args>
    explicit OptionBase(char const* name, char const* descr, Args&&... args)
        : name_(name)
        , descr_(descr)
    {
#if CL_HAS_FOLD_EXPRESSIONS
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
    cxx::string_view name() const
    {
        return name_;
    }

    // Returns the description of this option
    cxx::string_view descr() const
    {
        return descr_;
    }

    // Returns the number of times this option was specified on the command line
    int count() const
    {
        return count_;
    }

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
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::optional) {
        return count_ == 0;
    }
    return true;
}

inline bool OptionBase::IsOccurrenceRequired() const
{
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::one_or_more) {
        return count_ == 0;
    }
    return false;
}

//==================================================================================================
//
//==================================================================================================

template <typename Parser>
class Option : public OptionBase
{
#if CL_HAS_STD_INVOCABLE
    static_assert(std::is_invocable_r<bool, Parser, ParseContext&>::value || std::is_invocable_r<void, Parser, ParseContext&>::value,
        "The parser must be invocable with an argument of type 'ParseContext&' and the return type should be convertible to 'bool'");
#endif

    Parser /*const*/ parser_;

public:
    template <typename ParserInit, typename... Args>
    Option(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
        : OptionBase(name, descr, std::forward<Args>(args)...)
        , parser_(std::forward<ParserInit>(parser))
    {
    }

    Parser const& parser() const
    {
        return parser_;
    }

    Parser& parser()
    {
        return parser_;
    }

private:
    bool Parse(ParseContext& ctx) override
    {
        return DoParse(ctx, std::is_convertible<decltype(parser_(ctx)), bool>{});
    }

    bool DoParse(ParseContext& ctx, std::true_type /*parser_ returns bool*/)
    {
        return parser_(ctx);
    }

    bool DoParse(ParseContext& ctx, std::false_type /*parser_ returns bool*/)
    {
        parser_(ctx);
        return true;
    }
};

#if CL_HAS_DEDUCTION_GUIDES
template <typename ParserInit, typename ...Args>
Option(char const*, char const*, ParserInit&&, Args&&...) -> Option<std::decay_t<ParserInit>>;
#endif

template <typename ParserInit, typename ...Args>
Option<std::decay_t<ParserInit>> MakeOption(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
{
    return Option<std::decay_t<ParserInit>>(name, descr, std::forward<ParserInit>(parser), std::forward<Args>(args)...);
}

//==================================================================================================
//
//==================================================================================================

struct Diagnostic
{
    enum Type { error, warning, note };

    Type        type = Type::error;
    int         index = -1;
    std::string message = {};

    Diagnostic() = default;
    Diagnostic(Type type_, int index_, std::string message_)
        : type(type_)
        , index(index_)
        , message(std::move(message_))
    {
    }
};

//==================================================================================================
//
//==================================================================================================

class Cmdline
{
    struct NameOptionPair
    {
        cxx::string_view name = {}; // Points into option->name_
        OptionBase*      option = nullptr;

        NameOptionPair() = default;
        NameOptionPair(cxx::string_view name_, OptionBase* option_)
            : name(name_)
            , option(option_)
        {
        }
    };

    using Diagnostics = std::vector<Diagnostic>;
    using UniqueOptions = std::vector<std::unique_ptr<OptionBase>>;
    using Options = std::vector<NameOptionPair>;

    Diagnostics   diag_ = {};           // List of diagnostic messages
    UniqueOptions unique_options_ = {}; // Option storage.
    Options       options_ = {};        // List of options. Includes the positional options (in order).
    int           max_prefix_len_ = 0;  // Maximum length of the names of all prefix options
    int           curr_positional_ = 0; // The current positional argument - if any
    int           curr_index_ = 0;      // Index of the current argument
    bool          dashdash_ = false;    // "--" seen?

public:
    Cmdline() = default;
    Cmdline(Cmdline const&) = delete;
    Cmdline& operator=(Cmdline const&) = delete;

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& diag() const
    {
        return diag_;
    }

    // Adds a diagnostic message
    void EmitDiag(Diagnostic::Type type, int index, std::string message);

    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...) CL_ATTRIBUTE_PRINTF(4, 5);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    // The Cmdline object owns this option.
    template <typename ParserInit, typename... Args>
    auto Add(char const* name, char const* descr, ParserInit&& parser, Args&&... args);

    // Add an option to the commond line.
    // The Cmdline object takes ownership.
    OptionBase* Add(std::unique_ptr<OptionBase> opt);

    // Add an option to the commond line.
    // The Cmdline object does not own this option.
    OptionBase* Add(OptionBase* opt);

    // Resets the parser. Sets the COUNT members of all registered options to 0.
    void Reset();

    // Parse the command line arguments in ARGS.
    // Emits an error for unknown options.
    template <typename Container>
    bool ParseArgs(Container const& args, bool check_missing = true);

    // Parse the command line arguments in [CURR, LAST).
    // Emits an error for unknown options.
    template <typename It, typename EndIt>
    bool Parse(It curr, EndIt last, bool check_missing = true);

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if any required options have not yet been (successfully) parsed.
    // Emits errors for ALL missing options.
    bool AnyMissing();

    // Prints error messages to stderr.
    void PrintDiag() const;

    struct HelpFormat {
        size_t indent;
        size_t descr_indent;
        size_t max_width;

        HelpFormat() : indent(2), descr_indent(27), max_width(100) {}
    };

    // Returns a short help message listing all registered options.
    std::string FormatHelp(cxx::string_view program_name, HelpFormat const& fmt = {}) const;

    // Prints the help message to stderr
    void PrintHelp(cxx::string_view program_name, HelpFormat const& fmt = {}) const;

private:
    enum class Result { success, error, ignored };

    OptionBase* FindOption(cxx::string_view name) const;

    template <typename It, typename EndIt>
    Result Handle1(cxx::string_view optstr, It& curr, EndIt last);

    // <file>
    Result HandlePositional(cxx::string_view optstr);

    // -f
    // -f <file>
    template <typename It, typename EndIt>
    Result HandleStandardOption(cxx::string_view optstr, It& curr, EndIt last);

    // -f=<file>
    Result HandleOption(cxx::string_view optstr);

    // -I<dir>
    Result HandlePrefix(cxx::string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    Result DecomposeGroup(cxx::string_view optstr, std::vector<OptionBase*>& group);

    template <typename It, typename EndIt>
    Result HandleGroup(cxx::string_view optstr, It& curr, EndIt last);

    template <typename It, typename EndIt>
    Result HandleOccurrence(OptionBase* opt, cxx::string_view name, It& curr, EndIt last);
    Result HandleOccurrence(OptionBase* opt, cxx::string_view name, cxx::string_view arg);

    Result ParseOptionArgument(OptionBase* opt, cxx::string_view name, cxx::string_view arg);

    template <typename Fn>
    bool ForEachUniqueOption(Fn fn) const;
};

inline void Cmdline::EmitDiag(Diagnostic::Type type, int index, std::string message)
{
    diag_.emplace_back(type, index, std::move(message));
}

inline void Cmdline::FormatDiag(Diagnostic::Type type, int index, char const* format, ...)
{
    const size_t kBufSize = 1024;
    char         buf[kBufSize];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, kBufSize, format, args);
    va_end(args);

    diag_.emplace_back(type, index, std::string(buf));
}

template <typename ParserInit, typename... Args>
auto Cmdline::Add(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
{
    using DecayedParser = std::decay_t<ParserInit>;

    auto opt = std::make_unique<Option<DecayedParser>>(
            name, descr, std::forward<ParserInit>(parser), std::forward<Args>(args)...);

    auto const p = opt.get();
    Add(std::move(opt));
    return p;
}

inline OptionBase* Cmdline::Add(std::unique_ptr<OptionBase> opt)
{
    auto const p = opt.get();
    unique_options_.push_back(std::move(opt));
    return Add(p);
}

inline OptionBase* Cmdline::Add(OptionBase* opt)
{
    CL_DCHECK(opt != nullptr);

    impl::Split(opt->name_, impl::ByChar('|'), [&](cxx::string_view name) {
        CL_DCHECK(!name.empty());
        CL_DCHECK(FindOption(name) == nullptr); // option already exists?!

        if (opt->join_arg_ != JoinArg::no)
        {
            int const n = static_cast<int>(name.size());
            if (max_prefix_len_ < n) {
                max_prefix_len_ = n;
            }
        }

        options_.emplace_back(name, opt);

        return true;
    });

    return opt;
}

inline void Cmdline::Reset()
{
    curr_positional_ = 0;
    curr_index_ = 0;
    dashdash_ = false;

    ForEachUniqueOption([](cxx::string_view /*name*/, OptionBase* opt) {
        opt->count_ = 0;
        return true;
    });
}

template <typename Container>
bool Cmdline::ParseArgs(Container const& args, bool check_missing)
{
    using std::begin; // using ADL!
    using std::end;   // using ADL!

    return Parse(begin(args), end(args), check_missing);
}

template <typename It, typename EndIt>
bool Cmdline::Parse(It curr, EndIt last, bool check_missing)
{
    CL_DCHECK(curr_positional_ >= 0);
    CL_DCHECK(curr_index_ >= 0);

    while (curr != last)
    {
        Result const res = Handle1(*curr, curr, last);

        if (res == Result::error) {
            return false;
        }

        if (res == Result::ignored)
        {
            std::string arg(*curr);
            FormatDiag(Diagnostic::error, curr_index_, "Unknown option '%s'", arg.c_str());
            return false;
        }

        // Handle1 might have changed CURR.
        // Need to recheck if we're done.
        if (curr == last) {
            break;
        }
        ++curr;
        ++curr_index_;
    }

    if (check_missing) {
        return !AnyMissing();
    }

    return true;
}

inline bool Cmdline::AnyMissing()
{
    bool res = false;
    ForEachUniqueOption([&](cxx::string_view /*name*/, OptionBase* opt) {
        if (opt->IsOccurrenceRequired())
        {
            FormatDiag(Diagnostic::error, -1, "Option '%.*s' is missing", static_cast<int>(opt->name_.size()), opt->name_.data());
            res = true;
        }
        return true;
    });

    return res;
}

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32

inline void Cmdline::PrintDiag() const
{
    if (diag_.empty()) {
        return;
    }

    HANDLE const stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
    if (stderr_handle == NULL) { // No console.
        return;
    }
    if (stderr_handle == INVALID_HANDLE_VALUE) { // Error (Print without colors?!)
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(stderr_handle, &sbi);

    WORD const old_attributes = sbi.wAttributes;

    for (auto const& d : diag_)
    {
        fflush(stderr);
        switch (d.type)
        {
        case Diagnostic::error:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
            fprintf(stderr, "Error");
            break;
        case Diagnostic::warning:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            fprintf(stderr, "Warning");
            break;
        case Diagnostic::note:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            fprintf(stderr, "Note");
            break;
        }
        fflush(stderr);
        SetConsoleTextAttribute(stderr_handle, old_attributes);
        fprintf(stderr, ": %s\n", d.message.c_str());
    }
}

#else

#if CL_ANSI_CONSOLE_COLORS
#define CL_VT100_RESET      "\x1B[0m"
#define CL_VT100_RED        "\x1B[31;1m"
#define CL_VT100_MAGENTA    "\x1B[35;1m"
#define CL_VT100_CYAN       "\x1B[36;1m"
#else
#define CL_VT100_RESET
#define CL_VT100_RED
#define CL_VT100_MAGENTA
#define CL_VT100_CYAN
#endif

inline void Cmdline::PrintDiag() const
{
    for (auto const& d : diag_)
    {
        switch (d.type)
        {
        case Diagnostic::error:
            fprintf(stderr, CL_VT100_RED "Error" CL_VT100_RESET ": %s\n", d.message.c_str());
            break;
        case Diagnostic::warning:
            fprintf(stderr, CL_VT100_MAGENTA "Warning" CL_VT100_RESET ": %s\n", d.message.c_str());
            break;
        case Diagnostic::note:
            fprintf(stderr, CL_VT100_CYAN "Note" CL_VT100_RESET ": %s\n", d.message.c_str());
            break;
        }
    }
}

#undef CL_VT100_RESET
#undef CL_VT100_RED
#undef CL_VT100_MAGENTA
#undef CL_VT100_CYAN

#endif

namespace impl {
inline void AppendWrapped(std::string& out, cxx::string_view text, size_t indent, size_t width)
{
    CL_DCHECK(indent < width);

    bool first = true;

    // Break the string into paragraphs
    impl::Split(text, impl::ByLine(), [&](cxx::string_view par) {
        // Break the paragraphs at the maximum width into lines
        impl::Split(par, impl::ByMaxLength(width), [&](cxx::string_view line) {
            if (first)
            {
                first = false;
            }
            else
            {
                out += '\n';
                out.append(indent, ' ');
            }
            out.append(line.data(), line.size());
            return true;
        });
        return true;
    });
}
} // namespace impl

inline std::string Cmdline::FormatHelp(cxx::string_view program_name, HelpFormat const& fmt) const
{
    CL_DCHECK(fmt.descr_indent > fmt.indent);
    CL_DCHECK(fmt.max_width == 0 || fmt.max_width > fmt.descr_indent);

    const size_t descr_width = (fmt.max_width == 0) ? SIZE_MAX : (fmt.max_width - fmt.descr_indent);

    std::string spos;
    std::string sopt;

    ForEachUniqueOption([&](cxx::string_view /*name*/, OptionBase* opt) {
        if (opt->positional_ == Positional::yes)
        {
            spos += ' ';
            if (opt->num_opts_ == NumOpts::optional || opt->num_opts_ == NumOpts::zero_or_more)
            {
                spos += '[';
                spos.append(opt->name_.data(), opt->name_.size());
                spos += ']';
            }
            else
            {
                spos.append(opt->name_.data(), opt->name_.size());
            }
        }
        else
        {
            const size_t col0 = sopt.size();
            CL_DCHECK(col0 == 0 || sopt[col0 - 1] == '\n');

            sopt.append(fmt.indent, ' ');
            sopt += '-';
            sopt.append(opt->name_.data(), opt->name_.size());

            switch (opt->has_arg_)
            {
            case HasArg::no:
                break;
            case HasArg::optional:
                sopt += "=<ARG>";
                break;
            case HasArg::required:
                sopt += " <ARG>";
                break;
            }

            const size_t col = sopt.size() - col0;
            if (col < fmt.descr_indent)
            {
                sopt.append(fmt.descr_indent - col, ' ');
            }
            else
            {
                sopt += '\n';
                sopt.append(fmt.descr_indent, ' ');
            }

            impl::AppendWrapped(sopt, opt->descr_, fmt.descr_indent, descr_width);

            sopt += '\n'; // One option per line
        }

        return true;
    });

    if (sopt.empty()) {
        return "Usage: " + std::string(program_name) + spos + '\n';
    }

    return "Usage: " + std::string(program_name) + " [options]" + spos + "\nOptions:\n" + sopt;
}

inline void Cmdline::PrintHelp(cxx::string_view program_name, HelpFormat const& fmt) const
{
    auto const msg = FormatHelp(program_name, fmt);
    fprintf(stderr, "%s\n", msg.c_str());
}

inline OptionBase* Cmdline::FindOption(cxx::string_view name) const
{
    for (auto&& p : options_)
    {
        if (p.option->positional_ == Positional::yes) {
            continue;
        }
        if (p.name == name) {
            return p.option;
        }
    }

    return nullptr;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::Handle1(cxx::string_view optstr, It& curr, EndIt last)
{
    CL_DCHECK(curr != last);

    // This cannot happen if we're parsing the main's argv[] array, but it might
    // happen if we're parsing a user-supplied array of command line arguments.
    if (optstr.empty()) {
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
    if (optstr[0] != '-' || optstr == "-" || dashdash_) {
        return HandlePositional(optstr);
    }

    // Starts with a dash, must be an option.

    optstr.remove_prefix(1); // Remove the first dash.

    // If the name starts with a single dash, this is a short option and might
    // actually be an option group.
    bool const is_short = (optstr[0] != '-');
    if (!is_short) {
        optstr.remove_prefix(1); // Remove the second dash.
    }

    // 1. Try to handle options like "-f" and "-f file"
    Result res = HandleStandardOption(optstr, curr, last);

    // 2. Try to handle options like "-f=file"
    if (res == Result::ignored) {
        res = HandleOption(optstr);
    }

    // 3. Try to handle options like "-Idir"
    if (res == Result::ignored) {
        res = HandlePrefix(optstr);
    }

    // 4. Try to handle options like "-xvf=file" and "-xvf file"
    if (res == Result::ignored && is_short) {
        res = HandleGroup(optstr, curr, last);
    }

    // Otherwise this is an unknown option.

    return res;
}

inline Cmdline::Result Cmdline::HandlePositional(cxx::string_view optstr)
{
    int const E = static_cast<int>(options_.size());
    CL_DCHECK(curr_positional_ >= 0);
    CL_DCHECK(curr_positional_ <= E);

    for (; curr_positional_ != E; ++curr_positional_)
    {
        auto&& opt = options_[static_cast<size_t>(curr_positional_)].option;
        if (opt->positional_ == Positional::yes && opt->IsOccurrenceAllowed())
        {
            // The "argument" of a positional option, is the option name itself.
            return HandleOccurrence(opt, opt->name_, optstr);
        }
    }

    return Result::ignored;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleStandardOption(cxx::string_view optstr, It& curr, EndIt last)
{
    if (auto const opt = FindOption(optstr))
    {
        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Result::ignored;
}

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
inline Cmdline::Result Cmdline::HandleOption(cxx::string_view optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != cxx::string_view::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        if (auto const opt = FindOption(name))
        {
            // Ok, something like "-f=file".

            // Discard the equals sign if this option may NOT join its value.
            if (opt->join_arg_ == JoinArg::no) {
                ++arg_start;
            }

            return HandleOccurrence(opt, name, optstr.substr(arg_start));
        }
    }

    return Result::ignored;
}

inline Cmdline::Result Cmdline::HandlePrefix(cxx::string_view optstr)
{
    // Scan over all known prefix lengths.
    // Start with the longest to allow different prefixes like e.g. "-with" and
    // "-without".

    size_t n = static_cast<size_t>(max_prefix_len_);
    if (n > optstr.size()) {
        n = optstr.size();
    }

    for (; n != 0; --n)
    {
        auto const name = optstr.substr(0, n);
        auto const opt = FindOption(name);

        if (opt != nullptr && opt->join_arg_ != JoinArg::no) {
            return HandleOccurrence(opt, name, optstr.substr(n));
        }
    }

    return Result::ignored;
}

// Check if OPTSTR is actually a group of single letter options and store the
// options in GROUP.
inline Cmdline::Result Cmdline::DecomposeGroup(cxx::string_view optstr, std::vector<OptionBase*>& group)
{
    group.reserve(optstr.size());

    for (size_t n = 0; n < optstr.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->may_group_ == MayGroup::no) {
            return Result::ignored;
        }

        if (opt->has_arg_ == HasArg::no || n + 1 == optstr.size())
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
        FormatDiag(Diagnostic::error, curr_index_, "Option '%c' must be the last in a group", optstr[n]);
        return Result::error;
    }

    return Result::success;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleGroup(cxx::string_view optstr, It& curr, EndIt last)
{
    std::vector<OptionBase*> group;

    // First determine if this is a valid option group.
    auto const res = DecomposeGroup(optstr, group);

    if (res != Result::success) {
        return res;
    }

    // Then process all options.
    for (size_t n = 0; n < group.size(); ++n)
    {
        auto const opt = group[n];
        auto const name = optstr.substr(n, 1);

        if (opt->has_arg_ == HasArg::no || n + 1 == optstr.size())
        {
            if (Result::success != HandleOccurrence(opt, name, curr, last)) {
                return Result::error;
            }
            continue;
        }

        // Almost done. Process the last option which accepts an argument.

        size_t arg_start = n + 1;

        // If the next character is '=' and the option may not join its
        // argument, discard the equals sign.
        if (optstr[arg_start] == '=' && opt->join_arg_ == JoinArg::no) {
            ++arg_start;
        }

        return HandleOccurrence(opt, name, optstr.substr(arg_start));
    }

    return Result::success;
}

template <typename It, typename EndIt>
Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, cxx::string_view name, It& curr, EndIt last)
{
    CL_DCHECK(curr != last);

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    if (opt->join_arg_ != JoinArg::yes)
    {
        if (opt->has_arg_ != HasArg::required) {
            return ParseOptionArgument(opt, name, {});
        }

        // If the option requires an argument, steal one from the command line.
        ++curr;
        ++curr_index_;

        if (curr != last) {
            return ParseOptionArgument(opt, name, *curr);
        }
    }

    FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' requires an argument", static_cast<int>(name.size()), name.data());
    return Result::error;
}

inline Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, cxx::string_view name, cxx::string_view arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->has_arg_ == HasArg::no)
    {
        FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' does not accept an argument", static_cast<int>(name.size()), name.data());
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, cxx::string_view name, cxx::string_view arg)
{
    auto Parse1 = [&](cxx::string_view arg1) {
        if (!opt->IsOccurrenceAllowed())
        {
            FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' already specified", static_cast<int>(name.size()), name.data());
            return Result::error;
        }

        ParseContext ctx;

        ctx.name = name;
        ctx.arg = arg1;
        ctx.index = curr_index_;
        ctx.cmdline = this;

        //
        // XXX:
        // Implement a better way to determine if a diagnostic should be emitted here...
        //
        size_t const num_diagnostics = diag_.size();

        if (!opt->Parse(ctx))
        {
            bool const diagnostic_emitted = diag_.size() > num_diagnostics;
            if (!diagnostic_emitted) {
                FormatDiag(Diagnostic::error, curr_index_, "Invalid argument '%.*s' for option '%.*s'", static_cast<int>(arg1.size()), arg1.data(), static_cast<int>(name.size()), name.data());
            }

            return Result::error;
        }

        ++opt->count_;
        return Result::success;
    };

    Result res = Result::success;

    if (opt->comma_separated_ == CommaSeparated::yes)
    {
        impl::Split(arg, impl::ByChar(','), [&](cxx::string_view s) {
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
        // If the current option has the StopsParsing flag set, parse all
        // following options as positional options.
        if (opt->ends_options_ == EndsOptions::yes) {
            dashdash_ = true;
        }
    }

    return res;
}

template <typename Fn>
bool Cmdline::ForEachUniqueOption(Fn fn) const
{
    auto I = options_.begin();
    auto E = options_.end();

    if (I == E) {
        return true;
    }

    for (;;)
    {
        if (!fn(I->name, I->option)) {
            return false;
        }

        // Skip duplicate options.
        auto const curr_opt = I->option;
        for (;;)
        {
            if (++I == E) {
                return true;
            }
            if (I->option != curr_opt) {
                break;
            }
        }
    }
}

//==================================================================================================
//
//==================================================================================================

namespace impl {

template <typename... Match>
bool IsAnyOf(cxx::string_view value, Match&&... match)
{
#if CL_HAS_FOLD_EXPRESSIONS
    return (... || (value == match));
#else
    bool res = false;
    bool unused[] = {false, (res = res || (value == match))...};
    static_cast<void>(unused);
    return res;
#endif
}

template <typename T, typename Fn>
bool StrToX(cxx::string_view sv, T& value, Fn fn)
{
    if (sv.empty()) {
        return false;
    }

    std::string str(sv);

    char const* const ptr = str.c_str();
    char*             end = nullptr;

    int& ec = errno;

    auto const ec0 = std::exchange(ec, 0);
    auto const val = fn(ptr, &end);
    auto const ec1 = std::exchange(ec, ec0);

    if (ec1 == ERANGE) {
        return false;
    }
    if (end != ptr + str.size()) { // not all characters extracted
        return false;
    }

    value = val;
    return true;
}

// Note: Wrap into local function, to avoid instantiating StrToX with different
// lambdas which actually all do the same thing: call strtol.
inline bool StrToLongLong(cxx::string_view str, long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoll(p, end, 0); });
}

inline bool StrToUnsignedLongLong(cxx::string_view str, unsigned long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoull(p, end, 0); });
}

struct ConvertToInt
{
    template <typename T>
    bool operator()(cxx::string_view str, T& value) const
    {
        long long v = 0;
        if (StrToLongLong(str, v) && v >= (std::numeric_limits<T>::min)() && v <= (std::numeric_limits<T>::max)())
        {
            value = static_cast<T>(v);
            return true;
        }
        return false;
    }
};

struct ConvertToUnsignedInt
{
    template <typename T>
    bool operator()(cxx::string_view str, T& value) const
    {
        unsigned long long v = 0;
        if (StrToUnsignedLongLong(str, v) && v <= (std::numeric_limits<T>::max)())
        {
            value = static_cast<T>(v);
            return true;
        }
        return false;
    }
};

} // namespace impl

// Convert the string representation in STR into an object of type T.
template <typename T = void, typename /*Enable*/ = void>
struct ConvertTo
{
    template <typename Stream = std::stringstream>
    bool operator()(cxx::string_view str, T& value) const
    {
        Stream stream{std::string(str)};
        stream >> value;
        return !stream.fail() && stream.eof();
    }
};

template <>
struct ConvertTo<bool>
{
    bool operator()(cxx::string_view str, bool& value) const
    {
        if (str.empty() || impl::IsAnyOf(str, "1", "y", "true", "True", "yes", "Yes", "on", "On")) {
            value = true;
        } else if (impl::IsAnyOf(str, "0", "n", "false", "False", "no", "No", "off", "Off")) {
            value = false;
        } else {
            return false;
        }

        return true;
    }
};

template <> struct ConvertTo< signed char        > : impl::ConvertToInt {};
template <> struct ConvertTo< signed short       > : impl::ConvertToInt {};
template <> struct ConvertTo< signed int         > : impl::ConvertToInt {};
template <> struct ConvertTo< signed long        > : impl::ConvertToInt {};
template <> struct ConvertTo< signed long long   > : impl::ConvertToInt {};
template <> struct ConvertTo< unsigned char      > : impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned short     > : impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned int       > : impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned long      > : impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned long long > : impl::ConvertToUnsignedInt {};

template <>
struct ConvertTo<float>
{
    bool operator()(cxx::string_view str, float& value) const
    {
        return impl::StrToX(str, value, [](char const* p, char** end) { return std::strtof(p, end); });
    }
};

template <>
struct ConvertTo<double>
{
    bool operator()(cxx::string_view str, double& value) const
    {
        return impl::StrToX(str, value, [](char const* p, char** end) { return std::strtod(p, end); });
    }
};

template <>
struct ConvertTo<long double>
{
    bool operator()(cxx::string_view str, long double& value) const
    {
        return impl::StrToX(str, value, [](char const* p, char** end) { return std::strtold(p, end); });
    }
};

template <typename Alloc>
struct ConvertTo<std::basic_string<char, std::char_traits<char>, Alloc>>
{
    bool operator()(cxx::string_view str, std::basic_string<char, std::char_traits<char>, Alloc>& value) const
    {
        value.assign(str.data(), str.size());
        return true;
    }
};

template <typename Key, typename Value>
struct ConvertTo<std::pair<Key, Value>>
{
    bool operator()(cxx::string_view str, std::pair<Key, Value>& value) const
    {
        auto const p = str.find(':');

        if (p == cxx::string_view::npos) {
            return false;
        }

        return ConvertTo<Key>{}(str.substr(0, p), value.first)
            && ConvertTo<Value>{}(str.substr(p + 1), value.second);
    }
};

// The default implementation uses template argument deduction to select the correct specialization.
template <>
struct ConvertTo<void>
{
    template <typename T>
    bool operator()(cxx::string_view str, T& value) const
    {
        return ConvertTo<T>{}(str, value);
    }
};

// Convert the string representation in CTX.ARG into an object of type T.
// Possibly emits diagnostics on error.
template <typename T = void, typename /*Enable*/ = void>
struct ParseValue
{
    bool operator()(ParseContext const& ctx, T& value) const
    {
        return ConvertTo<T>{}(ctx.arg, value);
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

//==================================================================================================
//
//==================================================================================================

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

//==================================================================================================
//
//==================================================================================================

namespace impl {
template <typename T>
struct RemoveCVRec
{
    using type = std::remove_cv_t<T>;
};

template <template <typename...> class T, typename... Args>
struct RemoveCVRec<T<Args...>>
{
    using type = T<typename RemoveCVRec<Args>::type...>;
};

// Calls f(CTX, VALUE) for all f in FUNCS (in order) until the first f returns false.
// Returns true iff all f return true.
template <typename T, typename... Funcs>
bool ApplyFuncs(ParseContext& ctx, T& value, Funcs&&... funcs)
{
    static_cast<void>(ctx);   // may be unused if funcs is empty
    static_cast<void>(value); // may be unused if funcs is empty

#if CL_HAS_FOLD_EXPRESSIONS
    return (... && funcs(ctx, value));
#else
    bool res = true;
    bool unused[] = {(res = res && funcs(ctx, value))..., false};
    static_cast<void>(unused);
    return res;
#endif
}
} // namespace impl

// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
template <typename T, typename... Predicates>
auto Assign(T& target, Predicates&&... preds)
{
    return [=, &target](ParseContext& ctx) {
        // Parse into a local variable so that target is not assigned if any of the predicates returns false.
        T temp;
        if (impl::ApplyFuncs(ctx, temp, ParseValue<>{}, preds...))
        {
            target = std::move(temp);
            return true;
        }
        return false;
    };
}

// Default parser for list types.
// Uses an instance of Parser<> to convert the string and then inserts the
// converted value into the container.
// Predicates apply to the currently parsed value, not the whole list.
template <typename T, typename... Predicates>
auto PushBack(T& container, Predicates&&... preds)
{
    using V = typename impl::RemoveCVRec<typename T::value_type>::type;

    return [=, &container](ParseContext& ctx) {
        V temp;
        if (impl::ApplyFuncs(ctx, temp, ParseValue<>{}, preds...))
        {
            container.insert(container.end(), std::move(temp));
            return true;
        }
        return false;
    };
}

// Default parser for enum types.
// Look up the key in the map and if it exists, returns the mapped value.
template <typename T, typename... Predicates>
auto Map(T& value, std::initializer_list<std::pair<char const*, T>> ilist, Predicates&&... preds)
{
    using MapType = std::vector<std::pair<char const*, T>>;

    return [=, &value, map = MapType(ilist)](ParseContext & ctx) {
        for (auto const& p : map)
        {
            if (p.first == ctx.arg)
            {
                // Parse into a local variable to allow the predicates to modify the value.
                T temp = p.second;
                if (impl::ApplyFuncs(ctx, temp, preds...))
                {
                   value = std::move(temp);
                   return true;
                }
            }
        }

        ctx.cmdline->FormatDiag(Diagnostic::error, ctx.index, "Invalid argument '%.*s' for option '%.*s'", static_cast<int>(ctx.arg.size()), ctx.arg.data(), static_cast<int>(ctx.name.size()), ctx.name.data());
        for (auto const& p : map) {
            ctx.cmdline->FormatDiag(Diagnostic::note, ctx.index, "Could be '%s'", p.first);
        }

        return false;
    };
}

//==================================================================================================
//
//==================================================================================================

namespace impl {
inline bool IsWhitespace(char ch)
{
    switch (ch)
    {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
        return true;
    default:
        return false;
    }
}

inline void SkipWhitespace(char const*& next, char const* last)
{
    auto p = next;
    while (p != last && IsWhitespace(*p)) {
        ++p;
    }

    next = p;
}
} // namespace impl

struct ParseArgUnix
{
    std::string operator()(char const*& next, char const* last) const
    {
        // See:
        // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
        // http://wiki.bash-hackers.org/syntax/quoting

        std::string arg;

        auto it = next;
        char quote_char{};

        impl::SkipWhitespace(it, last);

        for (; it != last; ++it)
        {
            const auto ch = *it;

            // Quoting a single character using the backslash?
            if (quote_char == '\\')
            {
                arg += ch;
                quote_char = char{};
            }
            // Currently quoting using ' or "?
            else if (quote_char != char{} && ch != quote_char)
            {
                arg += ch;
            }
            // Toggle quoting?
            else if (ch == '\'' || ch == '"' || ch == '\\')
            {
                quote_char = (quote_char != char{}) ? char{} : ch;
            }
            // Arguments are separated by whitespace
            else if (impl::IsWhitespace(ch))
            {
                ++it;
                break;
            }
            else
            {
                arg += ch;
            }
        }

        next = it;

        return arg;
    }
};

struct ParseProgramNameWindows
{
    std::string operator()(char const*& next, char const* last) const
    {
        std::string arg;

        auto it = next;

        impl::SkipWhitespace(it, last);

        if (it != last)
        {
            const bool quoting = (*it == '"');

            if (quoting) {
                ++it;
            }

            for (; it != last; ++it)
            {
                const auto ch = *it;
                if ((quoting && ch == '"') || (!quoting && impl::IsWhitespace(ch)))
                {
                    ++it;
                    break;
                }
                arg += ch;
            }
        }

        next = it;

        return arg;
    }
};

struct ParseArgWindows
{
    std::string operator()(char const*& next, char const* last) const
    {
        std::string arg;

        auto    it = next;
        bool    quoting = false;
        bool    recently_closed = false;
        size_t  num_backslashes = 0;

        impl::SkipWhitespace(it, last);

        for (; it != last; ++it)
        {
            const auto ch = *it;

            if (ch == '"' && recently_closed)
            {
                recently_closed = false;

                // If a closing " is followed immediately by another ", the 2nd
                // " is accepted literally and added to the parameter.
                //
                // See:
                // http://www.daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC

                arg += '"';
            }
            else if (ch == '"')
            {
                // If an even number of backslashes is followed by a double
                // quotation mark, one backslash is placed in the argv array for
                // every pair of backslashes, and the double quotation mark is
                // interpreted as a string delimiter.
                //
                // If an odd number of backslashes is followed by a double
                // quotation mark, one backslash is placed in the argv array for
                // every pair of backslashes, and the double quotation mark is
                // "escaped" by the remaining backslash, causing a literal
                // double quotation mark (") to be placed in argv.

                const bool even = (num_backslashes % 2) == 0;

                arg.append(num_backslashes / 2, '\\');
                num_backslashes = 0;

                if (even)
                {
                    recently_closed = quoting; // Remember if this is a closing "
                    quoting = !quoting;
                }
                else
                {
                    arg += '"';
                }
            }
            else if (ch == '\\')
            {
                recently_closed = false;

                ++num_backslashes;
            }
            else
            {
                recently_closed = false;

                // Backslashes are interpreted literally, unless they
                // immediately precede a double quotation mark.

                arg.append(num_backslashes, '\\');
                num_backslashes = 0;

                if (!quoting && impl::IsWhitespace(ch))
                {
                    // Arguments are delimited by white space, which is either a
                    // space or a tab.
                    //
                    // A string surrounded by double quotation marks ("string")
                    // is interpreted as single argument, regardless of white
                    // space contained within. A quoted string can be embedded
                    // in an argument.
                    ++it;
                    break;
                }

                arg += ch;
            }
        }

        next = it;

        return arg;
    }
};

// Parse arguments from a command line string into an argv-array.
// Using Bash-style escaping.
inline std::vector<std::string> TokenizeUnix(cxx::string_view str)
{
    std::vector<std::string> argv;

    auto       next = str.data();
    auto const last = str.data() + str.size();

    while (next != last) {
        argv.push_back(ParseArgUnix{}(next, last));
    }

    return argv;
}

// Parse arguments from a command line string into an argv-array.
// Using Windows-style escaping.
inline std::vector<std::string> TokenizeWindows(cxx::string_view str, bool parse_program_name = true, bool discard_program_name = true)
{
    std::vector<std::string> argv;

    auto       next = str.data();
    auto const last = str.data() + str.size();

    if (parse_program_name || discard_program_name)
    {
        auto exe = ParseProgramNameWindows{}(next, last);
        if (!discard_program_name) {
            argv.push_back(std::move(exe));
        }
    }

    while (next != last) {
        argv.push_back(ParseArgWindows{}(next, last));
    }

    return argv;
}

} // namespace cl

#endif // CL_CMDLINE_H
