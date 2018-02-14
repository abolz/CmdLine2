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
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32
#include <windows.h>
#endif

#if defined(__has_include)
#define CL_HAS_INCLUDE(X) __has_include(X)
#else
#define CL_HAS_INCLUDE(X) 0
#endif

#if __cplusplus >= 201703 || (_MSC_VER >= 1910 && _HAS_CXX17)
#define CL_HAS_STD_STRING_VIEW 1
#include <string_view>
#endif

#if CL_HAS_INCLUDE(<experimental/string_view>) && __cplusplus > 201103
#define CL_HAS_STD_EXPERIMENTAL_STRING_VIEW 1
#include <experimental/string_view>
#endif

#if __cplusplus >= 201703 || __cpp_deduction_guides >= 201606
#define CL_HAS_DEDUCTION_GUIDES 1
#endif
#if __cplusplus >= 201703 || __cpp_fold_expressions >= 201411 || (_MSC_VER >= 1912 && _HAS_CXX17)
#define CL_HAS_FOLD_EXPRESSIONS 1
#endif
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
#define CL_HAS_STD_INVOCABLE 1
#endif

#if __GNUC__
#define CL_ATTRIBUTE_PRINTF(X, Y) __attribute__((format(printf, X, Y)))
#else
#define CL_ATTRIBUTE_PRINTF(X, Y)
#endif

#ifndef CL_ASSERT
#define CL_ASSERT(X) assert(X)
#endif

namespace cl {

class Cmdline;

//==================================================================================================
//
//==================================================================================================

#if CL_HAS_STD_STRING_VIEW
using std::string_view;
#elif CL_HAS_STD_EXPERIMENTAL_STRING_VIEW
using std::experimental::string_view;
#else
class string_view // A minimal std::string_view replacement
{
public:
    using value_type      = char;
    using pointer         = char const*;
    using const_pointer   = char const*;
    using reference       = char const&;
    using const_reference = char const&;
    using iterator        = char const*;
    using const_iterator  = char const*;
    using size_type       = size_t;

private:
    const_pointer data_ = nullptr;
    size_t size_ = 0;

private:
    static size_t Min(size_t x, size_t y) { return y < x ? y : x; }
    static size_t Max(size_t x, size_t y) { return y < x ? x : y; }

    static int Compare(char const* s1, char const* s2, size_t n) noexcept {
        return n == 0 ? 0 : ::memcmp(s1, s2, n);
    }

    static char const* Find(char const* s, size_t n, char ch) noexcept {
        CL_ASSERT(n != 0);
        return static_cast<char const*>(::memchr(s, static_cast<unsigned char>(ch), n));
    }

public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    constexpr string_view() noexcept = default;
    constexpr string_view(string_view const&) noexcept = default;
    constexpr string_view(string_view&&) noexcept = default;
    /*constexpr*/ string_view& operator=(string_view const&) noexcept = default;
    /*constexpr*/ string_view& operator=(string_view&&) noexcept = default;
    ~string_view() = default;

    string_view(const_pointer ptr, size_t len) noexcept
        : data_(ptr)
        , size_(len)
    {
        CL_ASSERT(size_ == 0 || data_ != nullptr);
    }

    string_view(const_pointer c_str) noexcept // (NOLINT)
        : data_(c_str)
        , size_(c_str != nullptr ? ::strlen(c_str) : 0u)
    {
    }

    template <
        typename String,
        typename = std::enable_if_t<
            std::is_convertible<decltype(std::declval<String const&>().data()), const_pointer>::value &&
            std::is_convertible<decltype(std::declval<String const&>().size()), size_t>::value
        >
    >
    string_view(String const& str) noexcept // (NOLINT)
        : data_(str.data())
        , size_(str.size())
    {
        CL_ASSERT(size_ == 0 || data_ != nullptr);
    }

    template <
        typename T,
        typename = std::enable_if_t< std::is_constructible<T, const_iterator, const_iterator>::value >
    >
    explicit operator T() const
    {
        return T(begin(), end());
    }

    // Returns a pointer to the start of the string.
    constexpr const_pointer data() const noexcept { return data_; }

    // Returns the length of the string.
    constexpr size_t size() const noexcept { return size_; }

    // Returns whether the string is empty.
    constexpr bool empty() const noexcept { return size_ == 0; }

    // Returns an iterator pointing to the start of the string.
    constexpr const_iterator begin() const noexcept { return data_; }

    // Returns an iterator pointing past the end of the string.
    constexpr const_iterator end() const noexcept { return data_ + size_; }

    // Returns a reference to the N-th character of the string.
    const_reference operator[](size_t n) const noexcept {
        CL_ASSERT(n < size_);
        return data_[n];
    }

    // Removes the first N characters from the string.
    void remove_prefix(size_t n) noexcept {
        CL_ASSERT(n <= size_);
        data_ += n;
        size_ -= n;
    }

    // Removes the last N characters from the string.
    void remove_suffix(size_t n) noexcept {
        CL_ASSERT(n <= size_);
        size_ -= n;
    }

    // Returns the substring [first, +count)
    string_view substr(size_t first = 0) const noexcept {
        CL_ASSERT(first <= size_);
        return {data_ + first, size_ - first};
    }

    // Returns the substring [first, +count)
    string_view substr(size_t first, size_t count) const noexcept {
        CL_ASSERT(first <= size_);
        return {data_ + first, Min(count, size_ - first)};
    }

    // Search for the first character ch in the sub-string [from, end)
    size_t find(char ch, size_t from = 0) const noexcept
    {
        if (from >= size())
            return npos;

        if (auto I = Find(data() + from, size() - from, ch))
            return static_cast<size_t>(I - data());

        return npos;
    }

    // Search for the last character in the sub-string [0, from)
    // which matches any of the characters in chars.
    size_t find_last_of(string_view chars, size_t from = npos) const noexcept
    {
        if (chars.empty())
            return npos;

        if (from < size()) {
            ++from;
        } else {
            from = size();
        }

        for (auto I = from; I != 0; --I) {
            if (Find(chars.data(), chars.size(), data()[I - 1]) != nullptr)
                return I - 1;
        }

        return npos;
    }

    bool _cmp_eq(string_view other) const noexcept {
        return size() == other.size() && Compare(data(), other.data(), size()) == 0;
    }

    bool _cmp_lt(string_view other) const noexcept {
        int const c = Compare(data(), other.data(), Min(size(), other.size()));
        return c < 0 || (c == 0 && size() < other.size());
    }
};

inline bool operator==(string_view s1, string_view s2) noexcept {
    return s1._cmp_eq(s2);
}

inline bool operator!=(string_view s1, string_view s2) noexcept {
    return !(s1 == s2);
}

inline bool operator<(string_view s1, string_view s2) noexcept {
    return s1._cmp_lt(s2);
}

inline bool operator<=(string_view s1, string_view s2) noexcept {
    return !(s2 < s1);
}

inline bool operator>(string_view s1, string_view s2) noexcept {
    return s2 < s1;
}

inline bool operator>=(string_view s1, string_view s2) noexcept {
    return !(s1 < s2);
}
#endif

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

    explicit ByChar(char ch_) : ch(ch_) {}

    DelimiterResult operator()(string_view const& str) const {
        return {str.find(ch), 1};
    }
};

// Breaks a string into lines, i.e. searches for "\n" or "\r" or "\r\n".
struct ByLine
{
    DelimiterResult operator()(string_view str) const
    {
        auto const first = str.data();
        auto const last = str.data() + str.size();

        // Find the position of the first CR or LF
        auto p = first;
        while (p != last && (*p != '\n' && *p != '\r')) {
            ++p;
        }

        if (p == last)
            return {string_view::npos, 0};

        auto const index = static_cast<size_t>(p - first);

        // If this is CRLF, skip the other half.
        if (p[0] == '\r' && p + 1 != last && p[1] == '\n')
            return {index, 2};

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
        CL_ASSERT(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(string_view str) const
    {
        // If the string fits into the current line, just return this last line.
        if (str.size() <= length)
            return {string_view::npos, 0};

        // Otherwise, search for the first space preceding the line length.
        auto const I = str.find_last_of(" \t", length);

        if (I != string_view::npos)
            return {I, 1};

        return {length, 0}; // No space in current line, break at length.
    }
};

struct DoSplitResult
{
    string_view tok; // The current token.
    string_view str; // The rest of the string.
    bool last = false;
};

inline bool DoSplit(DoSplitResult& res, string_view str, DelimiterResult del)
{
    if (del.first == string_view::npos)
    {
        res.tok = str;
        //res.str = {};
        res.last = true;
        return true;
    }

    CL_ASSERT(del.first + del.count >= del.first);
    CL_ASSERT(del.first + del.count <= str.size());

    size_t const off = del.first + del.count;
    CL_ASSERT(off > 0 && "invalid delimiter result");

    res.tok = string_view(str.data(), del.first);
    res.str = string_view(str.data() + off, str.size() - off);
    return true;
}

// Split the string STR into substrings using the Delimiter (or Tokenizer) SPLIT
// and call FN for each substring.
// FN must return void or bool. If FN returns false, this method stops splitting
// the input string and returns false, too. Otherwise, returns true.
template <typename Splitter, typename Function>
bool Split(string_view str, Splitter&& split, Function&& fn)
{
    DoSplitResult curr;

    curr.tok = string_view();
    curr.str = str;
    curr.last = false;

    for (;;)
    {
        if (!impl::DoSplit(curr, curr.str, split(curr.str)))
            return true;
        if (!fn(curr.tok))
            return false;
        if (curr.last)
            return true;
    }
}

} // namespace impl

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
    yes,
    // An argument is required.
    required = yes,
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

// Stop parsing early?
enum class StopParsing : unsigned char {
    // Nothing special.
    // This is the default.
    no,
    // If an option with this flag is (successfully) parser, all the remaining
    // command line arguments are ignored and the parser returns immediately.
    yes,
};

// Provides information about the argument and the command line parser which
// is currently parsing the arguments.
// The members are only valid inside the callback (parser).
struct ParseContext
{
    string_view name;           // Name of the option being parsed    (only valid in callback!)
    string_view arg;            // Option argument                    (only valid in callback!)
    int index = 0;              // Current index in the argv array
    Cmdline* cmdline = nullptr; // The command line parser which currently parses the argument list (never null)
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
    StopParsing    stop_parsing_    = StopParsing::no;
    // The number of times this option was specified on the command line
    int count_ = 0;

private:
    template <typename T>
    void Apply(T) = delete; // For slightly more useful error messages... (NOLINT)

    void Apply(NumOpts        v) { num_opts_        = v; }
    void Apply(HasArg         v) { has_arg_         = v; }
    void Apply(JoinArg        v) { join_arg_        = v; }
    void Apply(MayGroup       v) { may_group_       = v; }
    void Apply(Positional     v) { positional_      = v; }
    void Apply(CommaSeparated v) { comma_separated_ = v; }
    void Apply(EndsOptions    v) { ends_options_    = v; }
    void Apply(StopParsing    v) { stop_parsing_    = v; }

protected:
    template <typename... Args>
    explicit OptionBase(char const* name, char const* descr, Args&&... args)
        : name_(name)
        , descr_(descr)
    {
#if CL_HAS_FOLD_EXPRESSIONS
        (Apply(args), ...);
#else
        int const unused[] = {(Apply(args), 0)..., 0};
        static_cast<void>(unused);
#endif
    }

public:
    OptionBase(OptionBase const&) = default;
    OptionBase(OptionBase&&) = default;
    OptionBase& operator=(OptionBase const&) = default;
    OptionBase& operator=(OptionBase&&) = default;
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
    virtual bool Parse(ParseContext const& ctx) = 0;
};

inline OptionBase::~OptionBase() = default;

inline bool OptionBase::IsOccurrenceAllowed() const
{
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::optional)
        return count_ == 0;

    return true;
}

inline bool OptionBase::IsOccurrenceRequired() const
{
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::one_or_more)
        return count_ == 0;

    return false;
}

template <typename Parser>
class Option final : public OptionBase
{
#if CL_HAS_STD_INVOCABLE
    static_assert(std::is_invocable_r<bool, Parser, ParseContext&>::value ||
                  std::is_invocable_r<void, Parser, ParseContext&>::value,
        "The parser must be invocable with an argument of type 'ParseContext&' "
        "and the return type should be convertible to 'bool'");
#endif

    Parser /*const*/ parser_;

public:
    template <typename ParserInit, typename... Args>
    Option(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
        : OptionBase(name, descr, std::forward<Args>(args)...)
        , parser_(std::forward<ParserInit>(parser))
    {
    }

    Parser const& parser() const { return parser_; }
    Parser& parser() { return parser_; }

private:
    bool Parse(ParseContext const& ctx) override {
        return DoParse(ctx, std::is_convertible<decltype(parser_(ctx)), bool>{});
    }

    bool DoParse(ParseContext const& ctx, std::true_type /*parser_ returns bool*/) {
        return parser_(ctx);
    }

    bool DoParse(ParseContext const& ctx, std::false_type /*parser_ returns bool*/) {
        parser_(ctx);
        return true;
    }
};

#if CL_HAS_DEDUCTION_GUIDES
template <typename ParserInit, typename... Args>
Option(char const*, char const*, ParserInit&&, Args&&...) -> Option<std::decay_t<ParserInit>>;
#endif

// Creates a new option from the given arguments.
template <typename ParserInit, typename... Args>
auto MakeOption(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
{
    return Option<std::decay_t<ParserInit>>(
        name, descr, std::forward<ParserInit>(parser), std::forward<Args>(args)...);
}

// Creates a new option from the given arguments wrapped into a unique_ptr.
template <typename ParserInit, typename... Args>
auto MakeUniqueOption(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
{
    return std::make_unique<Option<std::decay_t<ParserInit>>>(
        name, descr, std::forward<ParserInit>(parser), std::forward<Args>(args)...);
}

struct Diagnostic
{
    enum Type { error, warning, note };

    Type type = Type::error;
    int index = -1;
    std::string message;

    Diagnostic() = default;
    Diagnostic(Type type_, int index_, std::string message_)
        : type(type_)
        , index(index_)
        , message(std::move(message_))
    {
    }
};

enum class CheckMissingOptions {
    no,
    yes,
};

class Cmdline final
{
    struct NameOptionPair
    {
        string_view name; // Points into option->name_
        OptionBase* option = nullptr;

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

    Diagnostics diag_;             // List of diagnostic messages
    UniqueOptions unique_options_; // Option storage.
    Options options_;              // List of options. Includes the positional options (in order).
    int max_prefix_len_ = 0;       // Maximum length of the names of all prefix options
    int curr_positional_ = 0;      // The current positional argument - if any
    int curr_index_ = 0;           // Index of the current argument
    bool dashdash_ = false;        // "--" seen?

public:
    Cmdline();
    Cmdline(Cmdline const&) = delete;
    Cmdline(Cmdline&&) = delete;
    Cmdline& operator=(Cmdline const&) = delete;
    Cmdline& operator=(Cmdline&&) = delete;
    ~Cmdline();

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& diag() const { return diag_; }

    // Adds a diagnostic message
    void EmitDiag(Diagnostic::Type type, int index, std::string message);

    // Adds a diagnostic message
    void FormatDiag(Diagnostic::Type type, int index, char const* format, ...) CL_ATTRIBUTE_PRINTF(4, 5);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    // The Cmdline object owns this option.
    template <typename ParserInit, typename... Args>
    Option<std::decay_t<ParserInit>>* Add(char const* name, char const* descr, ParserInit&& parser, Args&&... args);

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
    bool ParseArgs(Container const& args, CheckMissingOptions check_missing = CheckMissingOptions::yes);

    template <typename It>
    struct ParseResult
    {
        bool success = false;
        It next = It{};

        ParseResult() = default;
        ParseResult(bool success_, It next_)
            : success(success_)
            , next(next_)
        {
        }
    };

    // Parse the command line arguments in [CURR, LAST).
    // Emits an error for unknown options.
    template <typename It, typename EndIt>
    ParseResult<It> Parse(It curr, EndIt last, CheckMissingOptions check_missing = CheckMissingOptions::yes);

    // Returns whether all required options have been parsed since the last call
    // to Parse() and emits errors for all missing options.
    // Returns true if any required options have not yet been (successfully) parsed.
    // Emits errors for ALL missing options.
    bool AnyMissing();

    // Prints error messages to stderr.
    void PrintDiag() const;

    struct HelpFormat
    {
        size_t indent;       // (NOLINT)
        size_t descr_indent; // (NOLINT)
        size_t max_width;    // (NOLINT)

        HelpFormat()
            : indent(2)
            , descr_indent(27)
            , max_width(100)
        {
        }
    };

    // Returns a short help message listing all registered options.
    std::string FormatHelp(string_view program_name, HelpFormat const& fmt = {}) const;

    // Prints the help message to stderr
    void PrintHelp(string_view program_name, HelpFormat const& fmt = {}) const;

private:
    enum class Status {
        success,
        done,
        error,
        ignored
    };

    OptionBase* FindOption(string_view name) const;

    template <typename It, typename EndIt>
    Status Handle1(string_view optstr, It& curr, EndIt last);

    // <file>
    Status HandlePositional(string_view optstr);

    // -f
    // -f <file>
    template <typename It, typename EndIt>
    Status HandleStandardOption(string_view optstr, It& curr, EndIt last);

    // -f=<file>
    Status HandleOption(string_view optstr);

    // -I<dir>
    Status HandlePrefix(string_view optstr);

    // -xvf <file>
    // -xvf=<file>
    // -xvf<file>
    template <typename It, typename EndIt>
    Status HandleGroup(string_view optstr, It& curr, EndIt last);

    template <typename It, typename EndIt>
    Status HandleOccurrence(OptionBase* opt, string_view name, It& curr, EndIt last);
    Status HandleOccurrence(OptionBase* opt, string_view name, string_view arg);

    Status ParseOptionArgument(OptionBase* opt, string_view name, string_view arg);

    template <typename Fn>
    bool ForEachUniqueOption(Fn fn) const;
};

inline Cmdline::Cmdline() = default;

inline Cmdline::~Cmdline() = default;

inline void Cmdline::EmitDiag(Diagnostic::Type type, int index, std::string message)
{
    diag_.emplace_back(type, index, std::move(message));
}

inline void Cmdline::FormatDiag(Diagnostic::Type type, int index, char const* format, ...) // (NOLINT)
{
    constexpr size_t kBufSize = 1024;
    char buf[kBufSize];

    va_list args;
    va_start(args, format);
    vsnprintf(&buf[0], kBufSize, format, args);
    va_end(args);

    diag_.emplace_back(type, index, std::string(&buf[0]));
}

template <typename ParserInit, typename... Args>
Option<std::decay_t<ParserInit>>* Cmdline::Add(char const* name, char const* descr, ParserInit&& parser, Args&&... args)
{
    auto opt = MakeUniqueOption(name, descr, std::forward<ParserInit>(parser), std::forward<Args>(args)...);

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
    CL_ASSERT(opt != nullptr);

    impl::Split(opt->name_, impl::ByChar('|'), [&](string_view name) {
        CL_ASSERT(!name.empty());
        CL_ASSERT(FindOption(name) == nullptr); // option already exists?!

        if (opt->join_arg_ != JoinArg::no)
        {
            auto const n = static_cast<int>(name.size());
            if (max_prefix_len_ < n)
                max_prefix_len_ = n;
        }

        options_.emplace_back(name, opt);

        return true;
    });

    return opt;
}

inline void Cmdline::Reset()
{
    diag_.clear();
    curr_positional_ = 0;
    curr_index_ = 0;
    dashdash_ = false;

    ForEachUniqueOption([](string_view /*name*/, OptionBase* opt) {
        opt->count_ = 0;
        return true;
    });
}

template <typename Container>
bool Cmdline::ParseArgs(Container const& args, CheckMissingOptions check_missing)
{
    using std::begin; // using ADL!
    using std::end;   // using ADL!

    return Parse(begin(args), end(args), check_missing).success;
}

template <typename It, typename EndIt>
Cmdline::ParseResult<It> Cmdline::Parse(It curr, EndIt last, CheckMissingOptions check_missing)
{
    CL_ASSERT(curr_positional_ >= 0);
    CL_ASSERT(curr_index_ >= 0);

    while (curr != last)
    {
        // Make a copy of the current value.
        // NB: This is actually only needed for InputIterator's...
        std::string arg(*curr);

        Status const res = Handle1(arg, curr, last);
        switch (res)
        {
        case Status::success:
            break;
        case Status::done:
            return {true, curr};
        case Status::error:
            return {false, curr};
        case Status::ignored:
            FormatDiag(Diagnostic::error, curr_index_, "Unknown option '%s'", arg.c_str());
            return {false, curr};
        }

        // Handle1 might have changed CURR.
        // Need to recheck if we're done.
        if (curr == last)
            break;

        ++curr;
        ++curr_index_;
    }

    if (check_missing == CheckMissingOptions::yes)
        return {!AnyMissing(), curr};

    return {true, curr};
}

inline bool Cmdline::AnyMissing()
{
    bool res = false;
    ForEachUniqueOption([&](string_view /*name*/, OptionBase* opt) {
        if (opt->IsOccurrenceRequired())
        {
            FormatDiag(Diagnostic::error, -1, "Option '%.*s' is missing",
                       static_cast<int>(opt->name_.size()), opt->name_.data());
            res = true;
        }
        return true;
    });

    return res;
}

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32

inline void Cmdline::PrintDiag() const
{
    if (diag_.empty())
        return;

    HANDLE const stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

    if (stderr_handle == NULL)
        return; // No console.

    if (stderr_handle == INVALID_HANDLE_VALUE)
        return; // Error (Print without colors?!)

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
#define CL_VT100_RESET   "\x1B[0m"
#define CL_VT100_RED     "\x1B[31;1m"
#define CL_VT100_MAGENTA "\x1B[35;1m"
#define CL_VT100_CYAN    "\x1B[36;1m"
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

inline void AppendWrapped(std::string& out, string_view text, size_t indent, size_t width)
{
    CL_ASSERT(indent < width);

    bool first = true;

    // Break the string into paragraphs
    impl::Split(text, impl::ByLine(), [&](string_view par) {
        // Break the paragraphs at the maximum width into lines
        impl::Split(par, impl::ByMaxLength(width), [&](string_view line) {
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

inline std::string Cmdline::FormatHelp(string_view program_name, HelpFormat const& fmt) const
{
    CL_ASSERT(fmt.descr_indent > fmt.indent);
    CL_ASSERT(fmt.max_width == 0 || fmt.max_width > fmt.descr_indent);

    size_t const descr_width = (fmt.max_width == 0) ? SIZE_MAX : (fmt.max_width - fmt.descr_indent);

    std::string spos;
    std::string sopt = "Options:\n";

    bool has_options = false;

    ForEachUniqueOption([&](string_view /*name*/, OptionBase* opt) {
        if (opt->positional_ == Positional::yes)
        {
            bool const is_optional = (opt->num_opts_ == NumOpts::optional || opt->num_opts_ == NumOpts::zero_or_more);

            spos += ' ';
            if (is_optional) {
                spos += '[';
            }
            spos.append(opt->name_.data(), opt->name_.size());
            if (is_optional) {
                spos += ']';
            }
        }
        else
        {
            has_options = true;

            auto const col0 = sopt.size();
            CL_ASSERT(col0 == 0 || sopt[col0 - 1] == '\n');

            sopt.append(fmt.indent, ' ');
            sopt += '-';
            sopt.append(opt->name_.data(), opt->name_.size());

            if (opt->has_arg_ != HasArg::no)
            {
                sopt += (opt->has_arg_ == HasArg::optional) // (NOLINT)
                            ? "=<ARG>"
                            : " <ARG>";
            }

            auto const col = sopt.size() - col0;
            auto const wrap = (col >= fmt.descr_indent);
            auto const descr_indent = wrap ? fmt.descr_indent : fmt.descr_indent - col;

            if (wrap) {
                sopt += '\n';
            }
            sopt.append(descr_indent, ' ');

            impl::AppendWrapped(sopt, opt->descr_, fmt.descr_indent, descr_width);

            sopt += '\n'; // One option per line
        }

        return true;
    });

    std::string res = "Usage: ";

    res.append(program_name.data(), program_name.size());

    if (has_options) {
        res += " [options]";
    }
    res += spos; // Might be empty. If not: starts with a ' '
    res += '\n';
    if (has_options) {
        res += sopt; // Ends with an '\n'
    }

    return res;
}

inline void Cmdline::PrintHelp(string_view program_name, HelpFormat const& fmt) const
{
    auto const msg = FormatHelp(program_name, fmt);
    fprintf(stderr, "%s\n", msg.c_str());
}

inline OptionBase* Cmdline::FindOption(string_view name) const
{
    for (auto&& p : options_)
    {
        if (p.option->positional_ == Positional::yes)
            continue;
        if (p.name == name)
            return p.option;
    }

    return nullptr;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::Handle1(string_view optstr, It& curr, EndIt last)
{
    CL_ASSERT(curr != last);

    // This cannot happen if we're parsing the main's argv[] array, but it might
    // happen if we're parsing a user-supplied array of command line arguments.
    if (optstr.empty())
        return Status::success;

    // Stop parsing if "--" has been found
    if (optstr == "--" && !dashdash_)
    {
        dashdash_ = true;
        return Status::success;
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

    // 1. Try to handle options like "-f" and "-f file"
    Status res = HandleStandardOption(optstr, curr, last);

    // 2. Try to handle options like "-f=file"
    if (res == Status::ignored)
        res = HandleOption(optstr);

    // 3. Try to handle options like "-Idir"
    if (res == Status::ignored)
        res = HandlePrefix(optstr);

    // 4. Try to handle options like "-xvf=file" and "-xvf file"
    if (res == Status::ignored && is_short)
        res = HandleGroup(optstr, curr, last);

    // Otherwise this is an unknown option.

    return res;
}

inline Cmdline::Status Cmdline::HandlePositional(string_view optstr)
{
    auto const E = static_cast<int>(options_.size());
    CL_ASSERT(curr_positional_ >= 0);
    CL_ASSERT(curr_positional_ <= E);

    for (; curr_positional_ != E; ++curr_positional_)
    {
        auto&& opt = options_[static_cast<size_t>(curr_positional_)].option;
        if (opt->positional_ == Positional::yes && opt->IsOccurrenceAllowed())
        {
            // The "argument" of a positional option, is the option name itself.
            return HandleOccurrence(opt, opt->name_, optstr);
        }
    }

    return Status::ignored;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleStandardOption(string_view optstr, It& curr, EndIt last)
{
    if (auto const opt = FindOption(optstr))
    {
        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Status::ignored;
}

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
inline Cmdline::Status Cmdline::HandleOption(string_view optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != string_view::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        if (auto const opt = FindOption(name))
        {
            // Ok, something like "-f=file".

            // Discard the equals sign if this option may NOT join its value.
            if (opt->join_arg_ == JoinArg::no)
                ++arg_start;

            return HandleOccurrence(opt, name, optstr.substr(arg_start));
        }
    }

    return Status::ignored;
}

inline Cmdline::Status Cmdline::HandlePrefix(string_view optstr)
{
    // Scan over all known prefix lengths.
    // Start with the longest to allow different prefixes like e.g. "-with" and
    // "-without".

    auto n = static_cast<size_t>(max_prefix_len_);
    if (n > optstr.size())
        n = optstr.size();

    for (; n != 0; --n)
    {
        auto const name = optstr.substr(0, n);
        auto const opt = FindOption(name);

        if (opt != nullptr && opt->join_arg_ != JoinArg::no)
            return HandleOccurrence(opt, name, optstr.substr(n));
    }

    return Status::ignored;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleGroup(string_view optstr, It& curr, EndIt last)
{
    //
    // XXX:
    // Remove and call FindOption() in the second loop again?!
    //
    std::vector<OptionBase*> group;

    // First determine if this is a valid option group.
    for (size_t n = 0; n < optstr.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->may_group_ == MayGroup::no)
            return Status::ignored;

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
        return Status::error;
    }

    // Then process all options.
    for (size_t n = 0; n < group.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = group[n];

        if (opt->has_arg_ == HasArg::no || n + 1 == optstr.size())
        {
            if (Status::success != HandleOccurrence(opt, name, curr, last))
                return Status::error;

            continue;
        }

        // Almost done. Process the last option which accepts an argument.

        size_t arg_start = n + 1;

        // If the next character is '=' and the option may not join its
        // argument, discard the equals sign.
        if (optstr[arg_start] == '=' && opt->join_arg_ == JoinArg::no)
            ++arg_start;

        return HandleOccurrence(opt, name, optstr.substr(arg_start));
    }

    return Status::success;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleOccurrence(OptionBase* opt, string_view name, It& curr, EndIt last)
{
    CL_ASSERT(curr != last);

    // We get here if no argument was specified.
    // If the option must join its argument, this is an error.
    if (opt->join_arg_ != JoinArg::yes)
    {
        if (opt->has_arg_ != HasArg::required)
            return ParseOptionArgument(opt, name, {});

        // If the option requires an argument, steal one from the command line.
        ++curr;
        ++curr_index_;

        if (curr != last)
            return ParseOptionArgument(opt, name, *curr);
    }

    FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' requires an argument",
               static_cast<int>(name.size()), name.data());
    return Status::error;
}

inline Cmdline::Status Cmdline::HandleOccurrence(OptionBase* opt, string_view name, string_view arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->has_arg_ == HasArg::no)
    {
        FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' does not accept an argument",
                   static_cast<int>(name.size()), name.data());
        return Status::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Status Cmdline::ParseOptionArgument(OptionBase* opt, string_view name, string_view arg)
{
    auto Parse1 = [&](string_view arg1) {
        if (!opt->IsOccurrenceAllowed())
        {
            FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' already specified",
                       static_cast<int>(name.size()), name.data());
            return Status::error;
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
            if (!diagnostic_emitted)
            {
                FormatDiag(Diagnostic::error, curr_index_, "Invalid argument '%.*s' for option '%.*s'",
                           static_cast<int>(arg1.size()), arg1.data(),
                           static_cast<int>(name.size()), name.data());
            }

            return Status::error;
        }

        ++opt->count_;
        return Status::success;
    };

    Status res = Status::success;

    if (opt->comma_separated_ == CommaSeparated::yes)
    {
        impl::Split(arg, impl::ByChar(','), [&](string_view s) {
            res = Parse1(s);
            return res == Status::success;
        });
    }
    else
    {
        res = Parse1(arg);
    }

    if (res == Status::success)
    {
        // If the current option has the StopsParsing flag set, parse all
        // following options as positional options.
        if (opt->ends_options_ == EndsOptions::yes)
            dashdash_ = true;

        if (opt->stop_parsing_ == StopParsing::yes)
            res = Status::done;
    }

    return res;
}

template <typename Fn>
bool Cmdline::ForEachUniqueOption(Fn fn) const
{
    auto I = options_.begin();
    auto const E = options_.end();

    if (I == E)
        return true;

    for (;;)
    {
        if (!fn(I->name, I->option))
            return false;

        // Skip duplicate options.
        auto const curr_opt = I->option;
        for (;;)
        {
            if (++I == E)
                return true;
            if (I->option != curr_opt)
                break;
        }
    }
}

//==================================================================================================
//
//==================================================================================================

namespace impl {

inline bool IsAnyOf(string_view value, std::initializer_list<string_view> matches)
{
    for (auto const& m : matches) {
        if (value == m)
            return true;
    }

    return false;
}

template <typename T, typename Fn>
bool StrToX(string_view sv, T& value, Fn fn)
{
    if (sv.empty())
        return false;

    std::string str(sv);

    char const* const ptr = str.c_str();
    char* end = nullptr;

    int& ec = errno;

    auto const ec0 = std::exchange(ec, 0);
    auto const val = fn(ptr, &end);
    auto const ec1 = std::exchange(ec, ec0);

    if (ec1 == ERANGE)
        return false;

    if (end != ptr + str.size()) // (NOLINT)
        return false; // not all characters extracted

    value = val;
    return true;
}

// Note: Wrap into local function, to avoid instantiating StrToX with different
// lambdas which actually all do the same thing: call strtol.
inline bool StrToLongLong(string_view str, long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoll(p, end, 0); });
}

inline bool StrToUnsignedLongLong(string_view str, unsigned long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoull(p, end, 0); });
}

struct ParseInt
{
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const
    {
        long long v = 0;
        if (StrToLongLong(ctx.arg, v) && v >= (std::numeric_limits<T>::min)() && v <= (std::numeric_limits<T>::max)())
        {
            value = static_cast<T>(v);
            return true;
        }
        return false;
    }
};

struct ParseUnsignedInt
{
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const
    {
        unsigned long long v = 0;
        if (StrToUnsignedLongLong(ctx.arg, v) && v <= (std::numeric_limits<T>::max)())
        {
            value = static_cast<T>(v);
            return true;
        }
        return false;
    }
};

constexpr uint32_t kInvalidCodepoint = 0xFFFFFFFF;
constexpr uint32_t kReplacementCharacter = 0xFFFD;

inline bool IsValidCodePoint(uint32_t U)
{
    //
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    //
    return U <= 0x10FFFF && (U < 0xD800 || U > 0xDFFF);
}

inline int GetUTF8SequenceLengthFromLeadByte(char ch, uint32_t& U)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    if (b < 0x80) { U = b;        return 1; }
    if (b < 0xC0) {               return 0; }
    if (b < 0xE0) { U = b & 0x1F; return 2; }
    if (b < 0xF0) { U = b & 0x0F; return 3; }
    if (b < 0xF8) { U = b & 0x07; return 4; }
    return 0;
}

inline int GetUTF8SequenceLengthFromCodepoint(uint32_t U)
{
    CL_ASSERT(IsValidCodePoint(U));

    if (U <=   0x7F) { return 1; }
    if (U <=  0x7FF) { return 2; }
    if (U <= 0xFFFF) { return 3; }
    return 4;
}

inline bool IsUTF8OverlongSequence(uint32_t U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

inline bool IsUTF8ContinuationByte(char ch)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    return 0x80 == (b & 0xC0); // b == 10xxxxxx???
}

template <typename It>
It DecodeUTF8Sequence(It next, It last, uint32_t& U)
{
    CL_ASSERT(next != last);
    if (next == last)
    {
        U = kInvalidCodepoint; // Insuffient data
        return next;
    }

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Decoding a UTF-8 character proceeds as follows:
    //
    // 1.  Initialize a binary number with all bits set to 0.  Up to 21 bits
    // may be needed.
    //
    // 2.  Determine which bits encode the character number from the number
    // of octets in the sequence and the second column of the table
    // above (the bits marked x).
    //

    int const slen = GetUTF8SequenceLengthFromLeadByte(*next, U);
    ++next;

    if (slen == 0 || last - next < slen - 1)
    {
        U = kInvalidCodepoint; // Invalid lead byte or insufficient data
        return next;
    }

    //
    // 3.  Distribute the bits from the sequence to the binary number, first
    // the lower-order bits from the last octet of the sequence and
    // proceeding to the left until no x bits are left.  The binary
    // number is now equal to the character number.
    //

    auto const end = next + (slen - 1);
    for (; next != end; ++next)
    {
        if (!IsUTF8ContinuationByte(*next))
        {
            U = kInvalidCodepoint; // Invalid continuation byte
            return next;
        }

        U = (U << 6) | (static_cast<uint8_t>(*next) & 0x3F);
    }

    //
    // Implementations of the decoding algorithm above MUST protect against
    // decoding invalid sequences.  For instance, a naive implementation may
    // decode the overlong UTF-8 sequence C0 80 into the character U+0000,
    // or the surrogate pair ED A1 8C ED BE B4 into U+233B4.  Decoding
    // invalid sequences may have security consequences or cause other
    // problems.
    //

    if (!IsValidCodePoint(U) || IsUTF8OverlongSequence(U, slen))
    {
        U = kInvalidCodepoint; // Invalid codepoint or overlong sequence
        return next;
    }

    return next;
}

template <typename It>
It DecodeUTF16Sequence(It next, It last, uint32_t& U)
{
    CL_ASSERT(next != last);
    if (next == last)
    {
        U = kInvalidCodepoint;
        return next;
    }

    //
    // Decoding of a single character from UTF-16 to an ISO 10646 character
    // value proceeds as follows. Let W1 be the next 16-bit integer in the
    // sequence of integers representing the text. Let W2 be the (eventual)
    // next integer following W1.
    //

    uint32_t const W1 = static_cast<uint16_t>(*next);
    ++next;

    //
    // 1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
    // of W1. Terminate.
    //

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return next;
    }

    //
    // 2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
    // is in error and no valid character can be obtained using W1.
    // Terminate.
    //

    if (W1 > 0xDBFF)
    {
        U = kInvalidCodepoint;
        return next;
    }

    //
    // 3) If there is no W2 (that is, the sequence ends with W1), or if W2
    // is not between 0xDC00 and 0xDFFF, the sequence is in error.
    // Terminate.
    //

    if (next == last)
    {
        U = kInvalidCodepoint;
        return next;
    }

    uint32_t const W2 = static_cast<uint16_t>(*next);
    ++next;

    if (W2 < 0xDC00 || W2 > 0xDFFF)
    {
        U = kInvalidCodepoint;
        return next;
    }

    //
    // 4) Construct a 20-bit unsigned integer U', taking the 10 low-order
    // bits of W1 as its 10 high-order bits and the 10 low-order bits of
    // W2 as its 10 low-order bits.
    //
    //
    // 5) Add 0x10000 to U' to obtain the character value U. Terminate.
    //

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return next;
}

template <typename Put8>
void EncodeUTF8(uint32_t U, Put8 put)
{
    CL_ASSERT(IsValidCodePoint(U));

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Encoding a character to UTF-8 proceeds as follows:
    //
    // 1.  Determine the number of octets required from the character number
    // and the first column of the table above.  It is important to note
    // that the rows of the table are mutually exclusive, i.e., there is
    // only one valid way to encode a given character.
    //
    // 2.  Prepare the high-order bits of the octets as per the second
    // column of the table.
    //
    // 3.  Fill in the bits marked x from the bits of the character number,
    // expressed in binary.  Start by putting the lowest-order bit of
    // the character number in the lowest-order position of the last
    // octet of the sequence, then put the next higher-order bit of the
    // character number in the next higher-order position of that octet,
    // etc.  When the x bits of the last octet are filled in, move on to
    // the next to last octet, then to the preceding one, etc. until all
    // x bits are filled in.
    //

    if (U <= 0x7F)
    {
        put( static_cast<uint8_t>( U ) );
    }
    else if (U <= 0x7FF)
    {
        put( static_cast<uint8_t>( 0xC0 | ((U >>  6)       ) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
    else if (U <= 0xFFFF)
    {
        put( static_cast<uint8_t>( 0xE0 | ((U >> 12)       ) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
    else if (U <= 0x10FFFF)
    {
        put( static_cast<uint8_t>( 0xF0 | ((U >> 18)       ) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >> 12) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
}

template <typename Put16>
void EncodeUTF16(uint32_t U, Put16 put)
{
    CL_ASSERT(IsValidCodePoint(U));

    //
    // Encoding of a single character from an ISO 10646 character value to
    // UTF-16 proceeds as follows. Let U be the character number, no greater
    // than 0x10FFFF.
    //
    // 1) If U < 0x10000, encode U as a 16-bit unsigned integer and terminate.
    //

    if (U < 0x10000)
    {
        put( static_cast<uint16_t>(U) );
        return;
    }

    //
    // 2) Let U' = U - 0x10000. Because U is less than or equal to 0x10FFFF,
    // U' must be less than or equal to 0xFFFFF. That is, U' can be
    // represented in 20 bits.
    //
    // 3) Initialize two 16-bit unsigned integers, W1 and W2, to 0xD800 and
    // 0xDC00, respectively. These integers each have 10 bits free to
    // encode the character value, for a total of 20 bits.
    //
    // 4) Assign the 10 high-order bits of the 20-bit U' to the 10 low-order
    // bits of W1 and the 10 low-order bits of U' to the 10 low-order
    // bits of W2. Terminate.
    //

    uint32_t const Up = U - 0x10000;

    put( static_cast<uint16_t>(0xD800 + ((Up >> 10) & 0x3FF)) );
    put( static_cast<uint16_t>(0xDC00 + ((Up      ) & 0x3FF)) );
}

template <typename It, typename Put32>
bool ConvertUTF8ToUTF32(It next, It last, Put32 put)
{
    while (next != last)
    {
        uint32_t U = 0;

        auto const next1 = impl::DecodeUTF8Sequence(next, last, U);
        CL_ASSERT(next != next1);
        next = next1;

        if (U == impl::kInvalidCodepoint)
            return false;

        put(U);
    }

    return true;
}

template <typename It, typename Put16>
bool ConvertUTF8ToUTF16(It next, It last, Put16 put)
{
    return impl::ConvertUTF8ToUTF32(next, last, [&](uint32_t U) { impl::EncodeUTF16(U, put); });
}

template <typename It>
bool IsValidUTF8(It next, It last)
{
    return impl::ConvertUTF8ToUTF32(next, last, [](uint32_t /*U*/) {});
}

} // namespace impl

// Convert the string representation in CTX.ARG into an object of type T.
// Possibly emits diagnostics on error.
template <typename T = void, typename /*Enable*/ = void>
struct ParseValue
{
    template <typename Stream = std::stringstream>
    bool operator()(ParseContext const& ctx, T& value) const
    {
        Stream stream{std::string(ctx.arg)};
        stream >> value;
        return !stream.fail() && stream.eof();
    }
};

template <>
struct ParseValue<bool>
{
    bool operator()(ParseContext const& ctx, bool& value) const
    {
        if (impl::IsAnyOf(ctx.arg, {"", "1", "y", "true", "True", "yes", "Yes", "on", "On"})) {
            value = true;
        } else if (impl::IsAnyOf(ctx.arg, {"0", "n", "false", "False", "no", "No", "off", "Off"})) {
            value = false;
        } else {
            return false;
        }

        return true;
    }
};

template <> struct ParseValue< signed char        > : impl::ParseInt {};
template <> struct ParseValue< signed short       > : impl::ParseInt {};
template <> struct ParseValue< signed int         > : impl::ParseInt {};
template <> struct ParseValue< signed long        > : impl::ParseInt {};
template <> struct ParseValue< signed long long   > : impl::ParseInt {};
template <> struct ParseValue< unsigned char      > : impl::ParseUnsignedInt {};
template <> struct ParseValue< unsigned short     > : impl::ParseUnsignedInt {};
template <> struct ParseValue< unsigned int       > : impl::ParseUnsignedInt {};
template <> struct ParseValue< unsigned long      > : impl::ParseUnsignedInt {};
template <> struct ParseValue< unsigned long long > : impl::ParseUnsignedInt {};

template <>
struct ParseValue<float>
{
    bool operator()(ParseContext const& ctx, float& value) const
    {
        return impl::StrToX(ctx.arg, value, [](char const* p, char** end) { return std::strtof(p, end); });
    }
};

template <>
struct ParseValue<double>
{
    bool operator()(ParseContext const& ctx, double& value) const
    {
        return impl::StrToX(ctx.arg, value, [](char const* p, char** end) { return std::strtod(p, end); });
    }
};

template <>
struct ParseValue<long double>
{
    bool operator()(ParseContext const& ctx, long double& value) const
    {
        return impl::StrToX(ctx.arg, value, [](char const* p, char** end) { return std::strtold(p, end); });
    }
};

template <typename Alloc>
struct ParseValue<std::basic_string<char, std::char_traits<char>, Alloc>>
{
    bool operator()(ParseContext const& ctx, std::basic_string<char, std::char_traits<char>, Alloc>& value) const
    {
        bool const ok = impl::IsValidUTF8(ctx.arg.begin(), ctx.arg.end());
        if (!ok) {
            ctx.cmdline->FormatDiag(Diagnostic::error, ctx.index, "Invalid UTF-8 encoded string");
            return false;
        }

        value.assign(ctx.arg.begin(), ctx.arg.end());
        return true;
    }
};

template <typename Alloc>
struct ParseValue<std::basic_string<wchar_t, std::char_traits<wchar_t>, Alloc>>
{
    bool operator()(ParseContext const& ctx, std::basic_string<wchar_t, std::char_traits<wchar_t>, Alloc>& value) const
    {
        value.clear();

#if _WIN32
        static_assert(sizeof(wchar_t) == 2, "Invalid configuration");
        bool const ok = impl::ConvertUTF8ToUTF16(ctx.arg.begin(), ctx.arg.end(), [&](uint16_t ch) { value.push_back(static_cast<wchar_t>(ch)); });
#else
        static_assert(sizeof(wchar_t) == 4, "Invalid configuration");
        bool const ok = impl::ConvertUTF8ToUTF32(ctx.arg.begin(), ctx.arg.end(), [&](uint32_t ch) { value.push_back(static_cast<wchar_t>(ch)); });
#endif
        if (!ok) {
            ctx.cmdline->FormatDiag(Diagnostic::error, ctx.index, "Invalid UTF-8 encoded string");
            return false;
        }

        return true;
    }
};

template <>
struct ParseValue<void>
{
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const
    {
        return ParseValue<T>{}(ctx, value);
    }
};

namespace check {

// Returns a function object which checks whether a given value is in the range [lower, upper].
template <typename T, typename U>
auto InRange(T lower, U upper)
{
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(value < lower) && !(upper < value);
    };
}

// Returns a function object which checks whether a given value is > lower.
template <typename T>
auto GreaterThan(T lower)
{
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return lower < value;
    };
}

// Returns a function object which checks whether a given value is >= lower.
template <typename T>
auto GreaterEqual(T lower)
{
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(value < lower); // value >= lower
    };
}

// Returns a function object which checks whether a given value is < upper.
template <typename T>
auto LessThan(T upper)
{
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return value < upper;
    };
}

// Returns a function object which checks whether a given value is <= upper.
template <typename T>
auto LessEqual(T upper)
{
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(upper < value); // upper >= value
    };
}

} // namespace check

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
bool ApplyFuncs(ParseContext const& ctx, T& value, Funcs&&... funcs)
{
    static_cast<void>(ctx);   // may be unused if funcs is empty
    static_cast<void>(value); // may be unused if funcs is empty

#if CL_HAS_FOLD_EXPRESSIONS
    return (... && funcs(ctx, value));
#else
    bool res = true;
    bool const unused[] = {(res = res && funcs(ctx, value))..., false};
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
    return [=, &target](ParseContext const& ctx) {
        // Parse into a local variable so that target is not assigned if any of the predicates returns false.
        T temp;
        if (impl::ApplyFuncs(ctx, temp, ParseValue<>{}, preds...)) {
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

    return [=, &container](ParseContext const& ctx) {
        V temp;
        if (impl::ApplyFuncs(ctx, temp, ParseValue<>{}, preds...)) {
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

    return [=, &value, map = MapType(ilist)](ParseContext const& ctx) {
        for (auto const& p : map)
        {
            if (p.first != ctx.arg)
                continue;

            // Parse into a local variable to allow the predicates to modify the value.
            T temp = p.second;
            if (impl::ApplyFuncs(ctx, temp, preds...)) {
                value = std::move(temp);
                return true;
            }

            break;
        }

        ctx.cmdline->FormatDiag(Diagnostic::error, ctx.index, "Invalid argument '%.*s' for option '%.*s'",
                                static_cast<int>(ctx.arg.size()), ctx.arg.data(),
                                static_cast<int>(ctx.name.size()), ctx.name.data());
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

template <typename It>
It SkipWhitespace(It next, It last)
{
    while (next != last && IsWhitespace(*next)) {
        ++next;
    }

    return next;
}

} // namespace impl

struct ParseArgUnix
{
    template <typename It, typename Fn>
    It operator()(It next, It last, Fn fn) const
    {
        std::string arg;

        // See:
        // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
        // http://wiki.bash-hackers.org/syntax/quoting

        char quote_char = '\0';

        next = impl::SkipWhitespace(next, last);

        for (; next != last; ++next)
        {
            auto const ch = *next;

            // Quoting a single character using the backslash?
            if (quote_char == '\\')
            {
                arg += ch;
                quote_char = '\0';
            }
            // Currently quoting using ' or "?
            else if (quote_char != '\0' && ch != quote_char)
            {
                arg += ch;
            }
            // Toggle quoting?
            else if (ch == '\'' || ch == '"' || ch == '\\')
            {
                quote_char = (quote_char != '\0') ? '\0' : ch;
            }
            // Arguments are separated by whitespace
            else if (impl::IsWhitespace(ch))
            {
                ++next;
                break;
            }
            else
            {
                arg += ch;
            }
        }

        fn(std::move(arg));

        return next;
    }
};

struct ParseProgramNameWindows
{
    // TODO?!
    //
    // If the input string is empty, return a single command line argument
    // consisting of the absolute path of the executable...

    template <typename It, typename Fn>
    It operator()(It next, It last, Fn fn) const
    {
        std::string arg;

        if (next != last && !impl::IsWhitespace(*next))
        {
            bool const quoting = (*next == '"');

            if (quoting)
                ++next;

            for (; next != last; ++next)
            {
                auto const ch = *next;
                if ((quoting && ch == '"') || (!quoting && impl::IsWhitespace(ch)))
                {
                    ++next;
                    break;
                }
                arg += ch;
            }
        }

        fn(std::move(arg));

        return next;
    }
};

struct ParseArgWindows
{
    template <typename It, typename Fn>
    It operator()(It next, It last, Fn fn) const
    {
        std::string arg;

        bool quoting = false;
        bool recently_closed = false;
        size_t num_backslashes = 0;

        next = impl::SkipWhitespace(next, last);

        for (; next != last; ++next)
        {
            auto const ch = *next;

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

                bool const even = (num_backslashes % 2) == 0;

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
                    ++next;
                    break;
                }

                arg += ch;
            }
        }

        if (!arg.empty() || quoting || recently_closed)
        {
            fn(std::move(arg));
        }

        return next;
    }
};

// Parse arguments from a command line string into an argv-array.
// Using Bash-style escaping.
inline std::vector<std::string> TokenizeUnix(string_view str)
{
    std::vector<std::string> argv;

    auto push_back = [&](std::string arg) {
        argv.push_back(std::move(arg));
    };

    auto next = str.data();
    auto const last = str.data() + str.size();

    while (next != last) {
        next = ParseArgUnix{}(next, last, push_back);
    }

    return argv;
}

enum class ParseProgramName {
    no,
    yes,
};

// Parse arguments from a command line string into an argv-array.
// Using Windows-style escaping.
inline std::vector<std::string> TokenizeWindows(string_view str, ParseProgramName parse_program_name = ParseProgramName::yes)
{
    std::vector<std::string> argv;

    auto push_back = [&](std::string arg) {
        argv.push_back(std::move(arg));
    };

    auto next = str.data();
    auto const last = str.data() + str.size();

    if (parse_program_name == ParseProgramName::yes) {
        next = ParseProgramNameWindows{}(next, last, push_back);
    }

    while (next != last) {
        next = ParseArgWindows{}(next, last, push_back);
    }

    return argv;
}

#if _WIN32
inline std::vector<std::string> CommandLineToArgvUTF8(wchar_t const* command_line, ParseProgramName parse_program_name = ParseProgramName::yes)
{
    static_assert(sizeof(wchar_t) == 2, "Invalid configuration");

    CL_ASSERT(command_line != nullptr);

    auto next = command_line;
    auto const last = command_line + std::char_traits<wchar_t>::length(command_line); // (NOLINT)

    std::string command_line_utf8;

    while (next != last)
    {
        uint32_t U = 0;

        auto const next1 = impl::DecodeUTF16Sequence(next, last, U);
        CL_ASSERT(next != next1);
        next = next1;

        if (U == impl::kInvalidCodepoint)
            U = impl::kReplacementCharacter;

        impl::EncodeUTF8(U, [&](uint8_t ch) { command_line_utf8.push_back(static_cast<char>(ch)); });
    }

    return cl::TokenizeWindows(command_line_utf8, parse_program_name);
}
#endif // _WIN32

} // namespace cl

#endif // CL_CMDLINE_H
