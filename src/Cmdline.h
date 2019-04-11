// Copyright 2019 Alexander Bolz
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CL_CMDLINE_H
#define CL_CMDLINE_H 1

#if _WIN32
static_assert(sizeof(wchar_t) == 2, "Invalid configuration");
#else
static_assert(sizeof(wchar_t) == 4, "Invalid configuration");
#endif

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32
#include <windows.h>
#endif

#if __cpp_lib_string_view >= 201606
#define CL_HAS_STD_STRING_VIEW 1
#include <string_view>
#endif

#if __cpp_lib_is_invocable >= 201703
#define CL_HAS_STD_INVOCABLE 1
#endif

#if __cpp_deduction_guides >= 201606
#define CL_HAS_DEDUCTION_GUIDES 1
#endif

#if __cpp_fold_expressions >= 201603
#define CL_HAS_FOLD_EXPRESSIONS 1
#endif

//#if __cpp_lib_to_chars >= 201611 || (_MSC_VER >= 1920 && defined(_HAS_CXX17))
//#define CL_HAS_STD_CHARCONV 1
//#include <charconv>
//#endif

#if defined(__GNUC__) || defined(__clang__)
#define CL_FORCE_INLINE __attribute__((always_inline)) inline
#define CL_NEVER_INLINE __attribute__((noinline)) inline
#elif defined(_MSC_VER)
#define CL_FORCE_INLINE __forceinline
#define CL_NEVER_INLINE __declspec(noinline) inline
#else
#define CL_FORCE_INLINE inline
#define CL_NEVER_INLINE inline
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
using string_view = std::string_view;
#else
class string_view { // A minimal std::string_view replacement
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

    /*implicit*/ string_view(const_pointer c_str) noexcept
        : data_(c_str)
        , size_(c_str != nullptr ? ::strlen(c_str) : 0u)
    {
        CL_ASSERT(c_str != nullptr
            && "Constructing a string_view from a nullptr is incompatible with the std version");
    }

    template <
        typename String,
        typename = std::enable_if_t<
            std::is_convertible<decltype(std::declval<String const&>().data()), const_pointer>::value &&
            std::is_convertible<decltype(std::declval<String const&>().size()), size_t>::value
        >
    >
    /*implicit*/ string_view(String const& str) noexcept
        : data_(str.data())
        , size_(str.size())
    {
        CL_ASSERT(size_ == 0 || data_ != nullptr);
    }

    template <
        typename T,
        typename = std::enable_if_t< std::is_constructible<T, const_iterator, const_iterator>::value >
    >
    explicit operator T() const {
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
    size_t find(char ch, size_t from = 0) const noexcept {
        if (from >= size()) {
            return npos;
        }

        if (auto I = Find(data() + from, size() - from, ch)) {
            return static_cast<size_t>(I - data());
        }

        return npos;
    }

    // Search for the last character in the sub-string [0, from)
    // which matches any of the characters in chars.
    size_t find_last_of(string_view chars, size_t from = npos) const noexcept {
        if (chars.empty()) {
            return npos;
        }

        if (from < size()) {
            ++from;
        } else {
            from = size();
        }

        for (auto I = from; I != 0; --I) {
            if (Find(chars.data(), chars.size(), data()[I - 1]) != nullptr) {
                return I - 1;
            }
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

inline std::ostream& operator<<(std::ostream& os, string_view sv) {
    os << std::string(sv);
    return os;
}
#endif

//==================================================================================================
//
//==================================================================================================

// Controls whether an option must appear on the command line.
enum class Required : uint8_t {
    // The option is not required to appear on the command line.
    // This is the default.
    no,
    // The option is required to appear on the command line.
    yes,
};

// Determines whether an option may appear multiple times on the command line.
enum class Multiple : uint8_t {
    // The option may appear (at most) once on the command line.
    // This is the default.
    no,
    // The option may appear multiple times on the command line.
    yes,
};

// Controls the number of arguments the option accepts.
enum class Arg : uint8_t {
    // An argument is not allowed.
    // This is the default.
    no,
    // An argument is optional.
    optional,
    // An argument is required.
    yes,
    // An argument is not allowed.
    // This is the default.
    disallowed = no,
    // An argument is required.
    required = yes,
};

// Controls whether the option may/must join its argument.
enum class MayJoin : uint8_t {
    // The option must not join its argument: "-I dir" and "-I=dir" are
    // possible. If the option is specified with an equals sign ("-I=dir") the
    // '=' will NOT be part of the option argument.
    // This is the default.
    no,
    // The option may join its argument: "-I dir" and "-Idir" are possible. If
    // the option is specified with an equals sign ("-I=dir") the '=' will be
    // part of the option argument.
    yes,
};

// May this option group with other options?
enum class MayGroup : uint8_t {
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
enum class Positional : uint8_t {
    // The option is not a positional option, i.e. requires '-' or '--' as a
    // prefix when specified.
    // This is the default.
    no,
    // Positional option, no '-' required.
    yes,
};

// Split the argument between commas?
enum class CommaSeparated : uint8_t {
    // Do not split the argument between commas.
    // This is the default.
    no,
    // If this flag is set, the option's argument is split between commas, e.g.
    // "-i=1,2,,3" will be parsed as ["-i=1", "-i=2", "-i=", "-i=3"].
    // Note that each comma-separated argument counts as an option occurrence.
    yes,
};

// Stop parsing early?
enum class StopParsing : uint8_t {
    // Nothing special.
    // This is the default.
    no,
    // If an option with this flag is (successfully) parsed, all the remaining
    // command line arguments are ignored and the parser returns immediately.
    yes,
};

struct OptionFlags {
    Required       required        = Required::no;
    Multiple       multiple        = Multiple::no;
    Arg            arg             = Arg::no;
    MayJoin        may_join        = MayJoin::no;
    MayGroup       may_group       = MayGroup::no;
    Positional     positional      = Positional::no;
    CommaSeparated comma_separated = CommaSeparated::no;
    StopParsing    stop_parsing    = StopParsing::no;

    constexpr /*implicit*/ OptionFlags() = default;
    constexpr /*implicit*/ OptionFlags(Required       v) : required(v) {}
    constexpr /*implicit*/ OptionFlags(Multiple       v) : multiple(v) {}
    constexpr /*implicit*/ OptionFlags(Arg            v) : arg(v) {}
    constexpr /*implicit*/ OptionFlags(MayJoin        v) : may_join(v) {}
    constexpr /*implicit*/ OptionFlags(MayGroup       v) : may_group(v) {}
    constexpr /*implicit*/ OptionFlags(Positional     v) : positional(v) {}
    constexpr /*implicit*/ OptionFlags(CommaSeparated v) : comma_separated(v) {}
    constexpr /*implicit*/ OptionFlags(StopParsing    v) : stop_parsing(v) {}
};

// XXX:
// operator| is not commutative here...
// Because of Arg; e.g.:
//  Arg::no  ==  Arg::yes | Arg::no  !=  Arg::no | Arg::yes  ==  Arg::yes

inline /*constexpr*/ OptionFlags operator|(OptionFlags f, Required       v) { f.required        = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, Multiple       v) { f.multiple        = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, Arg            v) { f.arg             = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, MayJoin        v) { f.may_join        = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, MayGroup       v) { f.may_group       = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, Positional     v) { f.positional      = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, CommaSeparated v) { f.comma_separated = v; return f; }
inline /*constexpr*/ OptionFlags operator|(OptionFlags f, StopParsing    v) { f.stop_parsing    = v; return f; }

// Provides information about the argument and the command line parser which
// is currently parsing the arguments.
// The members are only valid inside the callback (parser).
struct ParseContext {
    string_view name;           // Name of the option being parsed    (only valid in callback!)
    string_view arg;            // Option argument                    (only valid in callback!)
    int index = 0;              // Current index in the argv array
    Cmdline* cmdline = nullptr; // The command line parser which currently parses the argument list (never null)
};

class OptionBase {
    friend class Cmdline;

    // The name of the option.
    string_view name_;
    // The description of this option
    string_view descr_;
    // Flags controlling how the option may/must be specified.
    OptionFlags flags_;
    // The number of times this option was specified on the command line
    int count_ = 0;

protected:
    explicit OptionBase(char const* name, char const* descr, OptionFlags flags);

public:
    OptionBase(OptionBase const&) = default;
    OptionBase(OptionBase&&) = default;
    OptionBase& operator=(OptionBase const&) = default;
    OptionBase& operator=(OptionBase&&) = default;
    virtual ~OptionBase();

public:
    // Returns the name of this option
    string_view Name() const { return name_; }

    // Returns the description of this option
    string_view Descr() const { return descr_; }

    // Returns the flags controlling how the option may/must be specified.
    bool HasFlag(Required       f) const { return flags_.required        == f; }
    bool HasFlag(Multiple       f) const { return flags_.multiple        == f; }
    bool HasFlag(Arg            f) const { return flags_.arg             == f; }
    bool HasFlag(MayJoin        f) const { return flags_.may_join        == f; }
    bool HasFlag(MayGroup       f) const { return flags_.may_group       == f; }
    bool HasFlag(Positional     f) const { return flags_.positional      == f; }
    bool HasFlag(CommaSeparated f) const { return flags_.comma_separated == f; }
    bool HasFlag(StopParsing    f) const { return flags_.stop_parsing    == f; }

    // Returns the number of times this option was specified on the command line
    int Count() const { return count_; }

private:
    bool IsOccurrenceAllowed() const;
    bool IsOccurrenceRequired() const;

    // Parse the given value from NAME and/or ARG and store the result.
    // Return true on success, false otherwise.
    virtual bool Parse(ParseContext const& ctx) = 0;
};

template <typename ParserT>
class Option final : public OptionBase {
#if CL_HAS_STD_INVOCABLE
    static_assert(std::is_invocable_r<bool, ParserT, ParseContext&>::value ||
                  std::is_invocable_r<void, ParserT, ParseContext&>::value,
        "The parser must be invocable with an argument of type 'ParseContext&' "
        "and the return type must be 'bool' or 'void'");
#endif

    ParserT /*const*/ parser_;

public:
    template <typename ParserInit>
    Option(char const* name, char const* descr, OptionFlags flags, ParserInit&& parser);

    ParserT const& Parser() const { return parser_; }
    ParserT& Parser() { return parser_; }

private:
    bool Parse(ParseContext const& ctx) override;

    bool DoParse(ParseContext const& ctx, std::true_type /*parser_ returns bool*/) {
        return parser_(ctx);
    }

    bool DoParse(ParseContext const& ctx, std::false_type /*parser_ returns bool*/) {
        parser_(ctx);
        return true;
    }
};

#if CL_HAS_DEDUCTION_GUIDES
template <typename ParserInit>
Option(char const*, char const*, OptionFlags, ParserInit&&)
    -> Option<std::decay_t<ParserInit>>;
#endif

struct Diagnostic {
    enum Type : uint8_t {
        error,
        warning,
        note,
    };

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

// Check for missing options in Cmdline::Parse?
enum class CheckMissingOptions : uint8_t {
    // Do not emit errors if required options have not been specified on the command line.
    no,
    // Emit errors if required options have not been specified on the command line.
    yes,
};

class Cmdline final {
    struct NameOptionPair {
        string_view name; // Points into option->name_
        OptionBase* option = nullptr;

        NameOptionPair() = default;
        NameOptionPair(string_view name_, OptionBase* option_) : name(name_), option(option_) {}
    };

    using Diagnostics   = std::vector<Diagnostic>;
    using UniqueOptions = std::vector<std::unique_ptr<OptionBase>>;
    using Options       = std::vector<NameOptionPair>;

    string_view name_;             // Program/sub-command name
    string_view descr_;
    Diagnostics diag_;             // List of diagnostic messages
    UniqueOptions unique_options_; // Option storage.
    Options options_;              // List of options. Includes the positional options (in order).
    int max_prefix_len_ = 0;       // Maximum length of the names of all prefix options
    int curr_positional_ = 0;      // The current positional argument - if any
    int curr_index_ = 0;           // Index of the current argument
    bool dashdash_ = false;        // "--" seen?

public:
    // Note:
    // Option and Cmdline names
    //  - must not be empty,
    //  - must not start with a '-',
    //  - must not contain an '=' sign.

    explicit Cmdline(char const* name, char const* descr);
    Cmdline(Cmdline const&) = delete;
    Cmdline(Cmdline&&) = delete;
    Cmdline& operator=(Cmdline const&) = delete;
    Cmdline& operator=(Cmdline&&) = delete;
    ~Cmdline();

    // Returns the name of the program or sub-command
    string_view Name() const { return name_; }

    // Returns the description of the program or sub-command
    string_view Descr() const { return descr_; }

    // Returns the diagnostic messages
    std::vector<Diagnostic> const& Diag() const { return diag_; }

    // Adds a diagnostic message.
    // Every argument must be explicitly convertible to string_view.
    template <typename... Args>
    void EmitDiag(Diagnostic::Type type, int index, Args&&... args);

    // Add an option to the command line.
    // Returns a pointer to the newly created option.
    // The Cmdline object owns this option.
    template <typename ParserInit>
    Option<std::decay_t<ParserInit>>* Add(char const* name, char const* descr, OptionFlags flags, ParserInit&& parser);

    // Add an option to the commond line.
    // The Cmdline object takes ownership.
    OptionBase* Add(std::unique_ptr<OptionBase> opt);

    // Add an option to the commond line.
    // The Cmdline object does not own this option.
    OptionBase* Add(OptionBase* opt);

    // Resets the parser. Sets the COUNT members of all registered options to 0.
    void Reset();

    template <typename It>
    struct ParseResult {
        It next = It{};
        bool success = false;

        ParseResult() = default;
        ParseResult(It next_, bool success_) : next(next_), success(success_) {}

        // Test for success.
        explicit operator bool() const { return success; }
    };

    // Parse the command line arguments in [CURR, LAST).
    // Emits an error for unknown options.
    template <typename It, typename EndIt>
    ParseResult<It> Parse(It curr, EndIt last, CheckMissingOptions check_missing = CheckMissingOptions::yes);

    // Parse the command line arguments in ARGS.
    // Emits an error for unknown options.
    template <typename Container>
    bool ParseArgs(Container const& args, CheckMissingOptions check_missing = CheckMissingOptions::yes);

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
        size_t line_length;

        HelpFormat() : indent(2), descr_indent(27), line_length(100) {}
    };

    // Returns a short help message listing all registered options.
    std::string FormatHelp(HelpFormat const& fmt = {}) const;

    // Prints the help message to stderr
    void PrintHelp(HelpFormat const& fmt = {}) const;

private:
    void DebugCheck() const;

    enum class Status : uint8_t {
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

    void EmitDiagImpl(Diagnostic::Type type, int index, string_view const* strings, int num_strings);
};

//==================================================================================================
// Unicode support
//==================================================================================================

namespace impl {

constexpr char32_t kInvalidCodepoint = 0xFFFFFFFF;
constexpr char32_t kReplacementCharacter = 0xFFFD;

inline bool IsValidCodepoint(char32_t U) {
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    return U < 0xD800 || (U > 0xDFFF && U <= 0x10FFFF);
}

constexpr uint32_t kUTF8Accept = 0;
constexpr uint32_t kUTF8Reject = 1;

inline uint32_t DecodeUTF8Step(uint32_t state, uint8_t byte, char32_t& U) {
    // Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

    static constexpr uint8_t kUTF8Decoder[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
        0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
        0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff

        0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
        1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
        1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
        1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
    };

    uint8_t const type = kUTF8Decoder[byte];

    // NB:
    // The conditional here will likely be optimized out in the loop below.

    if (state != kUTF8Accept) {
        U = (U << 6) | (byte & 0x3Fu);
    } else {
        U = byte & (0xFFu >> type);
    }

    state = kUTF8Decoder[256 + state * 16 + type];
    return state;
}

template <typename It>
It DecodeUTF8Sequence(It next, It last, char32_t& U) {
    CL_ASSERT(next != last);

    // Always consume the first byte.
    // The following bytes will only be consumed while the UTF-8 sequence is still valid.
    uint8_t const b1 = static_cast<uint8_t>(*next);
    ++next;

    char32_t W = 0;
    uint32_t state = DecodeUTF8Step(kUTF8Accept, b1, W);

    if (state == kUTF8Reject) {
        U = kInvalidCodepoint;
        return next;
    }

    while (state != kUTF8Accept) {
        if (next == last) {
            U = kInvalidCodepoint;
            return next;
        }

        state = DecodeUTF8Step(state, static_cast<uint8_t>(*next), W);
        if (state == kUTF8Reject) {
            U = kInvalidCodepoint;
            return next;
        }

        ++next;
    }

    U = W;
    return next;
}

template <typename PutChar>
void EncodeUTF8(char32_t U, PutChar put) {
    CL_ASSERT(IsValidCodepoint(U));

    if (U <= 0x7F) {
        put( static_cast<char>(static_cast<uint8_t>( U )) );
    } else if (U <= 0x7FF) {
        put( static_cast<char>(static_cast<uint8_t>( 0xC0 | ((U >>  6)       ) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) )) );
    } else if (U <= 0xFFFF) {
        put( static_cast<char>(static_cast<uint8_t>( 0xE0 | ((U >> 12)       ) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) )) );
    } else {
        put( static_cast<char>(static_cast<uint8_t>( 0xF0 | ((U >> 18) & 0x3F) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U >> 12) & 0x3F) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) )) );
        put( static_cast<char>(static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) )) );
    }
}

template <typename It, typename PutChar32>
bool ForEachUTF8EncodedCodepoint(It next, It last, PutChar32 put) {
    while (next != last) {
        char32_t U = 0;
        next = cl::impl::DecodeUTF8Sequence(next, last, U);
        if (!put(U)) {
            return false;
        }
    }

    return true;
}

template <typename It>
It DecodeUTF16Sequence(It next, It last, char32_t& U) {
    CL_ASSERT(next != last);

    // Always consume the first UCN.
    // The second UCN - if any - will only be consumed if the UTF16-sequence is valid.
    char32_t const W1 = static_cast<char16_t>(*next);
    ++next;

    if (W1 < 0xD800 || W1 > 0xDFFF) {
        U = W1;
        return next;
    }

    if (W1 > 0xDBFF) {
        U = kInvalidCodepoint; // Invalid high surrogate
        return next;
    }

    if (next == last) {
        U = kInvalidCodepoint; // Incomplete UTF-16 sequence
        return next;
    }

    char32_t const W2 = static_cast<char16_t>(*next);

    if (W2 < 0xDC00 || W2 > 0xDFFF) {
        U = kInvalidCodepoint; // Invalid low surrogate
        return next;
    }

    ++next;

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return next;
}

template <typename PutChar16>
void EncodeUTF16(char32_t U, PutChar16 put) {
    CL_ASSERT(IsValidCodepoint(U));

    if (U < 0x10000) {
        put( static_cast<char16_t>(U) );
    } else {
        char32_t const Up = U - 0x10000;
        put( static_cast<char16_t>(0xD800 + ((Up >> 10) & 0x3FF)) );
        put( static_cast<char16_t>(0xDC00 + ((Up      ) & 0x3FF)) );
    }
}

template <typename It, typename PutChar32>
bool ForEachUTF16EncodedCodepoint(It next, It last, PutChar32 put) {
    while (next != last) {
        char32_t U = 0;
        next = cl::impl::DecodeUTF16Sequence(next, last, U);
        if (!put(U)) {
            return false;
        }
    }

    return true;
}

template <typename It, typename PutChar32>
bool ForEachUTF32EncodedCodepoint(It next, It last, PutChar32 put) {
    while (next != last) {
        char32_t const U = static_cast<char32_t>(*next);
        ++next;
        if (!put(IsValidCodepoint(U) ? U : kInvalidCodepoint)) {
            return false;
        }
    }

    return true;
}

// Convert to UTF-8.
// The internal encoding used by the library is UTF-8.

template <typename It>
bool IsUTF8(It next, It last) {
    return cl::impl::ForEachUTF8EncodedCodepoint(next, last, [](char32_t U) { return U != cl::impl::kInvalidCodepoint; });
}

template <typename It>
std::string ToUTF8_dispatch(It next, It last, char const* /*tag*/, std::input_iterator_tag /*cat*/) {
    std::string s;

    cl::impl::ForEachUTF8EncodedCodepoint(next, last, [&](char32_t U) {
        if (U == kInvalidCodepoint) {
            U = kReplacementCharacter;
        }
        cl::impl::EncodeUTF8(U, [&](char ch) { s.push_back(ch); });
        return true;
    });

    return s;
}

template <typename It>
std::string ToUTF8_dispatch(It next, It last, char const* /*tag*/, std::forward_iterator_tag /*cat*/) {
    std::string s;
//  s.reserve(std::distance(next, last));

    while (next != last) {
        char32_t U = 0;
        auto const I = cl::impl::DecodeUTF8Sequence(next, last, U);

        if (U == kInvalidCodepoint) {
            s.append("\xEF\xBF\xBD", 3);
        } else {
            s.append(next, I);
        }

        next = I;
    }

    return s;
}

template <typename It>
std::string ToUTF8_dispatch(It next, It last, char const* tag) {
    using Cat = typename std::iterator_traits<It>::iterator_category;

    return cl::impl::ToUTF8_dispatch(next, last, tag, Cat{});
}

template <typename It>
std::string ToUTF8_dispatch(It next, It last, char16_t const* /*tag*/) {
    std::string s;

    cl::impl::ForEachUTF16EncodedCodepoint(next, last, [&](char32_t U) {
        if (U == kInvalidCodepoint) {
            U = kReplacementCharacter;
        }
        cl::impl::EncodeUTF8(U, [&](char ch) { s.push_back(ch); });
        return true;
    });

    return s;
}

template <typename It>
std::string ToUTF8_dispatch(It next, It last, char32_t const* /*tag*/) {
    std::string s;

    cl::impl::ForEachUTF32EncodedCodepoint(next, last, [&](char32_t U) {
        if (U == kInvalidCodepoint) {
            U = kReplacementCharacter;
        }
        cl::impl::EncodeUTF8(U, [&](char ch) { s.push_back(ch); });
        return true;
    });

    return s;
}

template <typename It>
CL_FORCE_INLINE std::string ToUTF8_dispatch(It next, It last, wchar_t const* /*tag*/) {
#if _WIN32
    return cl::impl::ToUTF8_dispatch(next, last, static_cast<char16_t const*>(nullptr));
#else
    return cl::impl::ToUTF8_dispatch(next, last, static_cast<char32_t const*>(nullptr));
#endif
}

template <typename It, typename T>
std::string ToUTF8_dispatch(It next, It last, T const* /*tag*/) = delete;

template <typename It>
CL_FORCE_INLINE std::string ToUTF8(It next, It last) {
    using CharT = std::remove_reference_t<decltype(*next)>;

    return cl::impl::ToUTF8_dispatch(next, last, static_cast<CharT const*>(nullptr));
}

template <typename StringT>
CL_FORCE_INLINE std::string ToUTF8(StringT const& str) {
    return cl::impl::ToUTF8(str.begin(), str.end());
}

template <typename ElemT>
CL_FORCE_INLINE std::string ToUTF8(ElemT* const& c_str) {
    using CharT = std::remove_const_t<ElemT>;

    auto const len = (c_str != nullptr)
                         ? std::char_traits<CharT>::length(c_str)
                         : 0u;

    return cl::impl::ToUTF8(c_str, c_str + len);
}

} // namespace impl

//==================================================================================================
// Split strings
//==================================================================================================

namespace impl {

// Return type for delimiters. Returned by the ByX delimiters.
//
// +-----+-----+------------+
// ^ tok ^     ^    rest    ^
//       f     f+c
//
// Either FIRST or COUNT must be non-zero.
struct DelimiterResult {
    size_t first;
    size_t count;
};

struct ByChar {
    char const ch;

    explicit ByChar(char ch_) : ch(ch_) {}

    DelimiterResult operator()(string_view str) const {
        return {str.find(ch), 1};
    }
};

// Breaks a string into lines, i.e. searches for "\n" or "\r" or "\r\n".
struct ByLines {
    DelimiterResult operator()(string_view str) const {
        auto const first = str.begin();
        auto const last = str.end();

        // Find the position of the first CR or LF
        auto p = first;
        while (p != last && (*p != '\n' && *p != '\r')) {
            ++p;
        }

        if (p == last) {
            return {string_view::npos, 0};
        }

        auto const index = static_cast<size_t>(p - first);

        // If this is CRLF, skip the other half.
        if (*p == '\r' && ++p != last && *p == '\n') {
            return {index, 2};
        }

        return {index, 1};
    }
};

// Breaks a string into words, i.e. searches for the first whitespace preceding
// the given length. If there is no whitespace, breaks a single word at length
// characters.
struct ByWords {
    size_t const length;

    explicit ByWords(size_t length_)
        : length(length_)
    {
        CL_ASSERT(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(string_view str) const {
        // If the string fits into the current line, just return this last line.
        if (str.size() <= length) {
            return {string_view::npos, 0};
        }

        // Otherwise, search for the first space preceding the line length.
        auto const last_ws = str.find_last_of(" \t", length);

        if (last_ws != string_view::npos) {
            // TODO:
            // Further remove trailing whitespace?!
            return {last_ws, 1};
        }

        return {length, 0}; // No space in current line, break at length.
    }
};

struct DoSplitResult {
    string_view tok; // The current token.
    string_view str; // The rest of the string.
    bool last = false;
};

inline bool DoSplit(DoSplitResult& res, string_view str, DelimiterResult del) {
    if (del.first == string_view::npos) {
        res.tok = str;
        //res.str = {};
        res.last = true;
        return true;
    }

    CL_ASSERT(del.first + del.count >= del.first);
    CL_ASSERT(del.first + del.count <= str.size());

    auto const off = del.first + del.count;
    CL_ASSERT(off > 0 && "invalid delimiter result");

    res.tok = string_view(str.data(), del.first);
    res.str = string_view(str.data() + off, str.size() - off);
    return true;
}

// Split the string STR into substrings using the Delimiter (or Tokenizer) SPLIT
// and call FN for each substring.
// FN must return bool. If FN returns false, this method stops splitting
// the input string and returns false, too. Otherwise, returns true.
template <typename Splitter, typename Function>
bool Split(string_view str, Splitter&& split, Function&& fn) {
    CL_ASSERT(cl::impl::IsUTF8(str.begin(), str.end()));

    DoSplitResult curr;

    curr.tok = string_view();
    curr.str = str;
    curr.last = false;

    for (;;) {
        if (!cl::impl::DoSplit(curr, curr.str, split(curr.str))) {
            return true;
        }

        CL_ASSERT(cl::impl::IsUTF8(curr.tok.begin(), curr.tok.end()));
        CL_ASSERT(cl::impl::IsUTF8(curr.str.begin(), curr.str.end()));

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

namespace impl {

template <typename...> struct AlwaysVoid { using type = void; };
template <typename... Ts> using Void_t = typename AlwaysVoid<Ts...>::type;

template <typename T, typename /*Enable*/ = void>
struct IsStreamExtractable
    : std::false_type
{
};

template <typename T>
struct IsStreamExtractable<T, Void_t< decltype(std::declval<std::istream&>() >> std::declval<T&>()) >>
    : std::true_type
{
};

template <typename T>
struct RemoveCVRec {
    using type = std::remove_cv_t<T>;
};

template <template <typename...> class T, typename... Args>
struct RemoveCVRec<T<Args...>> {
    using type = T<typename RemoveCVRec<Args>::type...>;
};

template <typename T>
using RemoveCVRec_t = typename RemoveCVRec<T>::type;

template <typename T, typename /*Enable*/ = void>
struct IsContainerImpl
    : std::false_type
{
};

template <typename T>
struct IsContainerImpl<T, Void_t< decltype( std::declval<T&>().insert(std::declval<T&>().end(), std::declval<typename T::value_type>()) ) >>
    : std::true_type
{
};

template <typename T>
struct IsContainer
    : IsContainerImpl<T>
{
};

// Do not handle strings as containers.
template <typename Elem, typename Traits, typename Alloc>
struct IsContainer<std::basic_string<Elem, Traits, Alloc>>
    : std::false_type
{
};

template <typename T>
using IsContainer_t = typename IsContainer<std::decay_t<T>>::type;

#if CL_HAS_FOLD_EXPRESSIONS

template <typename Lhs, typename... Rhs>
CL_FORCE_INLINE bool IsAnyOf(Lhs const& lhs, Rhs&&... rhs) {
    return (false || ... || (lhs == std::forward<Rhs>(rhs)));
}

#else

template <typename Lhs>
CL_FORCE_INLINE bool IsAnyOf(Lhs const& /*lhs*/) {
    return false;
}

template <typename Lhs, typename Rhs1, typename... Rhs>
CL_FORCE_INLINE bool IsAnyOf(Lhs const& lhs, Rhs1&& rhs1, Rhs&&... rhs) {
    return (lhs == std::forward<Rhs1>(rhs1))
        ? true
        : cl::impl::IsAnyOf(lhs, std::forward<Rhs>(rhs)...);
}

#endif

inline bool StartsWith(string_view str, string_view prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

inline bool IsWhitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\v' || ch == '\f' || ch == '\r';
}

template <typename It>
It SkipWhitespace(It next, It last) {
    while (next != last && IsWhitespace(*next)) {
        ++next;
    }

    return next;
}

} // namespace impl

//==================================================================================================
// Parser
//==================================================================================================

namespace impl {

// Convert the string representation in CTX.ARG into an object of type T.
// Possibly emits diagnostics on error.
template <typename T = void, typename /*Enable*/ = void>
struct ConvertTo {
    static_assert(cl::impl::IsStreamExtractable<T>::value,
        "The default implementation of 'ConvertTo<T>' requires the type 'T' is stream-extractable");

    template <typename Stream = std::istringstream>
    bool operator()(ParseContext const& ctx, T& value) const {
        Stream stream{std::string(ctx.arg)};
        stream >> value;
        // This function should fail if there are any characters left in the
        // input stream, but there are cases for which all characters have been
        // extracted and the eofbit is not set yet.
        return !stream.fail() && (stream.rdbuf()->in_avail() == 0);
    }
};

template <>
struct ConvertTo<bool> {
    bool operator()(ParseContext const& ctx, bool& value) const {
        if (cl::impl::IsAnyOf(ctx.arg, string_view{}, "1", "y", "yes", "Yes", "on", "On", "true", "True")) {
            value = true;
        } else if (cl::impl::IsAnyOf(ctx.arg, "0", "n", "no", "No", "off", "Off", "false", "False")) {
            value = false;
        } else {
            return false;
        }

        return true;
    }
};

enum class ParseNumberStatus {
    success,
    syntax_error,
    overflow,
};

struct ParseNumberResult {
    char const* ptr;
    ParseNumberStatus ec;

    // Test for successful conversion.
    explicit operator bool() const {
        return ec == ParseNumberStatus::success;
    }
};

// Returns the decimal value for the given hexadecimal character.
// Returns UINT8_MAX, if the given character is not a hexadecimal character.
inline uint8_t HexDigitValue(char ch)
{
    enum : uint8_t { N = UINT8_MAX };
    static constexpr uint8_t kMap[256] = {
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, N, N, N, N, N, N,
        N, 10,11,12,13,14,15,N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, 10,11,12,13,14,15,N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    };

    return kMap[static_cast<uint8_t>(ch)];
}

inline bool IsHexDigit(char ch) {
    return HexDigitValue(ch) != UINT8_MAX;
}

inline ParseNumberResult ParseU64(char const* next, char const* last, uint64_t& value) {
    CL_ASSERT(next != last);

    uint64_t max_pre_multiply;
    uint32_t base;

    // Determine base and optionally skip prefix.
    if (*next == '0') {
        ++next;
        if (next == last) { // literal "0"
            value = 0;
            return {next, ParseNumberStatus::success};
        }

        switch (*next) {
        case 'x':
        case 'X':
            ++next;
            max_pre_multiply = UINT64_MAX / 16;
            base = 16;
            break;
        case 'b':
        case 'B':
            ++next;
            max_pre_multiply = UINT64_MAX / 2;
            base = 2;
            break;
        case 'o':
        case 'O':
            ++next;
            // fall through
        default:
            max_pre_multiply = UINT64_MAX / 8;
            base = 8;
            break;
        }
    }
    else
    {
        max_pre_multiply = UINT64_MAX / 10;
        base = 10;
    }

    uint64_t v = 0;
    for (;;) {
        if (next == last) {
            break;
        }
        uint32_t const d = HexDigitValue(*next);
        if (d >= base) {
            break;
        }
        if (v > max_pre_multiply || d > UINT64_MAX - base * v) {
            return {next, ParseNumberStatus::overflow};
        }
        v = base * v + d;
        ++next;
    }

    value = v;
    return {next, ParseNumberStatus::success};
}

inline ParseNumberResult StrToU64(char const* next, char const* last, uint64_t& value) {
    next = SkipWhitespace(next, last);

    if (next == last) {
        return {next, ParseNumberStatus::syntax_error};
    }

    // Skip optional '+' sign.
    // A leading minus sign is considered invalid while parsing *unsigned* integers
    // and will result in a syntax_error in ParseU64.
    if (*next == '+') {
        ++next;
        if (next == last) {
            return {next, ParseNumberStatus::syntax_error};
        }
    }

    return ParseU64(next, last, value);
}

inline ParseNumberResult StrToI64(char const* next, char const* last, int64_t& value) {
    next = SkipWhitespace(next, last);

    if (next == last) {
        return {next, ParseNumberStatus::syntax_error};
    }

    bool const is_negative = (*next == '-');

    // Skip optional '+' or '-' sign.
    if (is_negative || *next == '+') {
        ++next;
        if (next == last) {
            return {next, ParseNumberStatus::syntax_error};
        }
    }

    uint64_t v;
    auto const res = ParseU64(next, last, v);
    if (res) {
        // The number fits into an uint64_t. Check if it fits into an int64_t, too.
        // Assuming 2's complement.

        constexpr uint64_t Limit = 9223372036854775808ull; // -INT64_MIN

        if (is_negative && v == Limit) {
            value = INT64_MIN;
        } else if (v >= Limit) {
            return {next, ParseNumberStatus::overflow};
        } else {
            value = static_cast<int64_t>(is_negative ? (0 - v) : v);
        }
    }

    return res;
}

struct ConvertToInt {
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const {
        auto const next = ctx.arg.data();
        auto const last = ctx.arg.data() + ctx.arg.size();

        int64_t v = 0;
        auto const res = StrToI64(next, last, v);

        if (res && res.ptr == last && v >= (std::numeric_limits<T>::min)() && v <= (std::numeric_limits<T>::max)()) {
            value = static_cast<T>(v);
            return true;
        }

        return false;
    }
};

struct ConvertToUnsignedInt {
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const {
        auto const next = ctx.arg.data();
        auto const last = ctx.arg.data() + ctx.arg.size();

        uint64_t v = 0;
        auto const res = StrToU64(next, last, v);

        if (res && res.ptr == last && v <= (std::numeric_limits<T>::max)()) {
            value = static_cast<T>(v);
            return true;
        }

        return false;
    }
};

template <> struct ConvertTo< signed char        > : cl::impl::ConvertToInt {};
template <> struct ConvertTo< signed short       > : cl::impl::ConvertToInt {};
template <> struct ConvertTo< signed int         > : cl::impl::ConvertToInt {};
template <> struct ConvertTo< signed long        > : cl::impl::ConvertToInt {};
template <> struct ConvertTo< signed long long   > : cl::impl::ConvertToInt {};
template <> struct ConvertTo< unsigned char      > : cl::impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned short     > : cl::impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned int       > : cl::impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned long      > : cl::impl::ConvertToUnsignedInt {};
template <> struct ConvertTo< unsigned long long > : cl::impl::ConvertToUnsignedInt {};

#if CL_HAS_STD_CHARCONV
template <typename T>
inline ParseNumberResult StrToFloatingPoint(char const* next, char const* last, T& value) {
    std::chars_format fmt;

    // std::from_chars does not automatically determine the base (and does not skip any prefixes).
    bool const is_hex = (last - next >= 2) && (next[0] == '0') && (next[1] == 'x' || next[1] == 'X');
    if (is_hex) {
        fmt = std::chars_format::hex;
        next += 2; // Skip prefix ('0x' or '0X')
    } else {
        fmt = std::chars_format::general;
    }

    if (next == last) {
        return {next, ParseNumberStatus::syntax_error};
    }

    auto const res = std::from_chars(next, last, value, fmt);
    switch (res.ec) {
    case std::errc{}:
        return {res.ptr, ParseNumberStatus::success};
    case std::errc::result_out_of_range:
        // XXX:
        // Is result_out_of_range actually ever returned from from_chars for floating-point?!?!
#if 1
        return {res.ptr, ParseNumberStatus::success};
#else
        return {res.ptr, ParseNumberStatus::overflow};
#endif
    default:
        return {res.ptr, ParseNumberStatus::syntax_error};
    }
}

//template <typename T>
//inline ParseNumberResult StrToFloatingPoint(string_view sv, T& value) {
//    return cl::impl::StrToFloatingPoint(sv.data(), sv.data() + sv.size(), value);
//}
#else // ^^^ CL_HAS_STD_CHARCONV ^^^
template <typename T, typename Fn>
inline ParseNumberResult StrToFloatingPoint(char const* next, char const* last, T& value, Fn fn) {
    if (next == last) {
        return {next, ParseNumberStatus::syntax_error};
    }

    std::string str(next, last);

    char const* begin = str.c_str();
    char* end = nullptr;

#if 0
    // Seems that std::from_chars doesn't return std::errc::result_out_of_range even
    // if finite non-zero numbers round to infinity or 0.0.
    // So we ignore errno here... :-/
    auto const val = fn(begin, &end);
    next += end - begin;
#else
    int& ec = errno;

    auto const ec0 = std::exchange(ec, 0);
    auto const val = fn(begin, &end);
    auto const ec1 = std::exchange(ec, ec0);
    next += end - begin;

    if (ec1 == ERANGE) {
        return {next, ParseNumberStatus::overflow};
    }
#endif

    value = val;
    return {next, ParseNumberStatus::success};
}

inline ParseNumberResult StrToFloatingPoint(char const* next, char const* last, float& value) {
    return cl::impl::StrToFloatingPoint(next, last, value, [](char const* p, char** end) { return std::strtof(p, end); });
}

inline ParseNumberResult StrToFloatingPoint(char const* next, char const* last, double& value) {
    return cl::impl::StrToFloatingPoint(next, last, value, [](char const* p, char** end) { return std::strtod(p, end); });
}

inline ParseNumberResult StrToFloatingPoint(char const* next, char const* last, long double& value) {
    return cl::impl::StrToFloatingPoint(next, last, value, [](char const* p, char** end) { return std::strtold(p, end); });
}

//template <typename T>
//inline ParseNumberResult StrToFloatingPoint(string_view sv, T& value) {
//    return cl::impl::StrToFloatingPoint(sv.data(), sv.data() + sv.size(), value);
//}
#endif // ^^^ !CL_HAS_STD_CHARCONV ^^^

struct ConvertToFloatingPoint {
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) {
        auto const next = ctx.arg.data();
        auto const last = ctx.arg.data() + ctx.arg.size();

        T v = 0;
        auto const res = StrToFloatingPoint(next, last, v);

        if (res && res.ptr == last) {
            value = v;
            return true;
        }

        return false;
    }
};

template <> struct ConvertTo< float       > : cl::impl::ConvertToFloatingPoint {};
template <> struct ConvertTo< double      > : cl::impl::ConvertToFloatingPoint {};
template <> struct ConvertTo< long double > : cl::impl::ConvertToFloatingPoint {};

template <typename Traits, typename Alloc>
struct ConvertTo<std::basic_string<char, Traits, Alloc>> {
    bool operator()(ParseContext const& ctx, std::basic_string<char, Traits, Alloc>& value) const {
        // We know that ctx.arg is well-formed UTF-8 already.
        // No need to re-check here.
        value.assign(ctx.arg.begin(), ctx.arg.end());
        return true;
    }
};

template <typename Traits, typename Alloc>
struct ConvertTo<std::basic_string<char16_t, Traits, Alloc>> {
    bool operator()(ParseContext const& ctx, std::basic_string<char16_t, Traits, Alloc>& value) const {
        value.clear();

        // We know that ctx.arg is well-formed UTF-8 already.
        cl::impl::ForEachUTF8EncodedCodepoint(ctx.arg.begin(), ctx.arg.end(), [&](char32_t U) {
            CL_ASSERT(cl::impl::IsValidCodepoint(U));
            cl::impl::EncodeUTF16(U, [&](char16_t code_unit) { value.push_back(code_unit); });
            return true;
        });

        return true;
    }
};

template <typename Traits, typename Alloc>
struct ConvertTo<std::basic_string<char32_t, Traits, Alloc>> {
    bool operator()(ParseContext const& ctx, std::basic_string<char32_t, Traits, Alloc>& value) const {
        value.clear();

        // We know that ctx.arg is well-formed UTF-8 already.
        cl::impl::ForEachUTF8EncodedCodepoint(ctx.arg.begin(), ctx.arg.end(), [&](char32_t U) {
            CL_ASSERT(cl::impl::IsValidCodepoint(U));
            value.push_back(U);
            return true;
        });

        return true;
    }
};

template <typename Traits, typename Alloc>
struct ConvertTo<std::basic_string<wchar_t, Traits, Alloc>> {
    bool operator()(ParseContext const& ctx, std::basic_string<wchar_t, Traits, Alloc>& value) const {
        value.clear();

        // We know that ctx.arg is well-formed UTF-8 already.
        cl::impl::ForEachUTF8EncodedCodepoint(ctx.arg.begin(), ctx.arg.end(), [&](char32_t U) {
            CL_ASSERT(cl::impl::IsValidCodepoint(U));
#if _WIN32
            cl::impl::EncodeUTF16(U, [&](char16_t code_unit) { value.push_back(static_cast<wchar_t>(code_unit)); });
#else
            value.push_back(static_cast<wchar_t>(U));
#endif
            return true;
        });

        return true;
    }
};

template <>
struct ConvertTo<void> {
    template <typename T>
    bool operator()(ParseContext const& ctx, T& value) const {
        return ConvertTo<T>{}(ctx, value);
    }
};

} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace check {

// Returns a function object which checks whether a given value is in the range [lower, upper].
template <typename T, typename U>
auto InRange(T lower, U upper) {
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(value < lower) && !(upper < value);
    };
}

// Returns a function object which checks whether a given value is > lower.
template <typename T>
auto GreaterThan(T lower) {
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return lower < value;
    };
}

// Returns a function object which checks whether a given value is >= lower.
template <typename T>
auto GreaterEqual(T lower) {
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(value < lower); // value >= lower
    };
}

// Returns a function object which checks whether a given value is < upper.
template <typename T>
auto LessThan(T upper) {
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return value < upper;
    };
}

// Returns a function object which checks whether a given value is <= upper.
template <typename T>
auto LessEqual(T upper) {
    return [=](ParseContext const& /*ctx*/, auto const& value) {
        return !(upper < value); // upper >= value
    };
}

} // namespace check

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

#if !CL_HAS_FOLD_EXPRESSIONS
namespace impl {

// Calls f(CTX, VALUE) for all f in FUNCS (in order) until the first f returns false.
// Returns true iff all f return true.
template <typename T, typename... Funcs>
bool ApplyFuncs(ParseContext const& ctx_, T& value_, Funcs&&... funcs) {
    static_cast<void>(ctx_);   // may be unused if funcs is empty
    static_cast<void>(value_); // may be unused if funcs is empty

    bool res = true;
    bool const unused[] = {(res = res && funcs(ctx_, value_))..., false};
    static_cast<void>(unused);
    return res;
}

} // namespace impl
#endif

// Default parser for scalar types.
// Uses an instance of Parser<> to convert the string.
template <typename T, typename... Predicates>
auto Assign(T& target, Predicates&&... preds) {
    static_assert(!std::is_const<T>::value,
        "Assign() requires mutable lvalue-references");
    static_assert(std::is_default_constructible<T>::value,
        "Assign() requires default-constructible types");
    static_assert(std::is_move_assignable<T>::value,
        "Assign() requires move-assignable types");

    return [=, &target](ParseContext const& ctx) {
        // Parse into a local variable so that target is not assigned if any of the predicates returns false.
        T temp;
#if CL_HAS_FOLD_EXPRESSIONS
        if ((cl::impl::ConvertTo<>{}(ctx, temp) && ... && preds(ctx, temp))) {
#else
        if (cl::impl::ApplyFuncs(ctx, temp, cl::impl::ConvertTo<>{}, preds...)) {
#endif
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
auto Append(T& container, Predicates&&... preds) {
    static_assert(!std::is_const<T>::value,
        "Append() requires mutable lvalue-references");
    static_assert(cl::impl::IsContainer_t<T>::value,
        "Append() requires STL-style container types");
    static_assert(std::is_default_constructible<cl::impl::RemoveCVRec_t<typename T::value_type>>::value,
        "Append() requires default-constructible value_type's");

    using V = cl::impl::RemoveCVRec_t<typename T::value_type>;

    return [=, &container](ParseContext const& ctx) {
        V temp;
#if CL_HAS_FOLD_EXPRESSIONS
        if ((cl::impl::ConvertTo<>{}(ctx, temp) && ... && preds(ctx, temp))) {
#else
        if (cl::impl::ApplyFuncs(ctx, temp, cl::impl::ConvertTo<>{}, preds...)) {
#endif
            container.insert(container.end(), std::move(temp));
            return true;
        }
        return false;
    };
}

namespace impl {

template <typename T, typename... Predicates>
auto Var(std::false_type /*IsContainer*/, T& var, Predicates&&... preds) {
    return cl::Assign(var, std::forward<Predicates>(preds)...);
}

template <typename T, typename... Predicates>
auto Var(std::true_type /*IsContainer*/, T& var, Predicates&&... preds) {
    return cl::Append(var, std::forward<Predicates>(preds)...);
}

} // namespace impl

// Default parser.
// Can be used as a replacement for Assign or PushBack (in almost all cases).
template <typename T, typename... Predicates>
auto Var(T& var, Predicates&&... preds) {
    return cl::impl::Var(cl::impl::IsContainer_t<T>{}, var, std::forward<Predicates>(preds)...);
}

// Default parser for enum types.
// Look up the key in the map and if it exists, returns the mapped value.
template <typename T, typename... Predicates>
auto Map(T& value, std::initializer_list<std::pair<char const*, T>> ilist, Predicates&&... preds) {
    static_assert(!std::is_const<T>::value,
        "Map() requires mutable lvalue-references");
    static_assert(std::is_copy_constructible<T>::value,
        "Map() requires copy-constructible types");
    static_assert(std::is_move_assignable<T>::value,
        "Map() requires move-assignable types");

    using MapType = std::vector<std::pair<string_view, T>>;

    return [=, &value, map = MapType(ilist.begin(), ilist.end())](ParseContext const& ctx) {
        for (auto const& p : map) {
            if (p.first != ctx.arg) {
                continue;
            }

            // Parse into a local variable to allow the predicates to modify the value.
            T temp = p.second;
#if CL_HAS_FOLD_EXPRESSIONS
            if ((true && ... && preds(ctx, temp))) {
#else
            if (cl::impl::ApplyFuncs(ctx, temp, preds...)) {
#endif
                value = std::move(temp);
                return true;
            }

            break;
        }

        ctx.cmdline->EmitDiag(Diagnostic::error, ctx.index, "invalid argument '", ctx.arg, "' for option '", ctx.name, "'");
        for (auto const& p : map) {
            ctx.cmdline->EmitDiag(Diagnostic::note, ctx.index, "could be '", p.first, "'");
        }

        return false;
    };
}

// For (boolean) flags.
// Parses the options' argument and stores the result in 'var'.
// If the options' name starts with 'inverse_prefix', inverts the parsed value, using operator!.
template <typename T>
auto Flag(T& var, char const* inverse_prefix = "no-") {
    static_assert(!std::is_const<T>::value,
        "Flag() requires mutable lvalue-references");

    return [=, &var](ParseContext const& ctx) {
        if (!cl::impl::ConvertTo<>{}(ctx, var)) {
            return false;
        }
        if (cl::impl::StartsWith(ctx.name, inverse_prefix)) {
            var = !var;
        }
        return true;
    };
}

//==================================================================================================
// Tokenize
//==================================================================================================

namespace impl {

template <typename It, typename Fn>
It ParseArgUnix(It next, It last, Fn fn) {
    std::string arg;

    // See:
    // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
    // http://wiki.bash-hackers.org/syntax/quoting

    char quote_char = '\0';

    next = cl::impl::SkipWhitespace(next, last);

    for (; next != last; ++next) {
        auto const ch = *next;

        if (quote_char == '\\') { // Quoting a single character using the backslash?
            arg += ch;
            quote_char = '\0';
        } else if (quote_char != '\0' && ch != quote_char) { // Currently quoting using ' or "?
            arg += ch;
        } else if (ch == '\'' || ch == '"' || ch == '\\') { // Toggle quoting?
            quote_char = (quote_char != '\0') ? '\0' : ch;
        } else if (cl::impl::IsWhitespace(ch)) { // Arguments are separated by whitespace
            ++next;
            break;
        } else {
            arg += ch;
        }
    }

    fn(std::move(arg));

    return next;
}

template <typename It, typename Fn>
It ParseProgramNameWindows(It next, It last, Fn fn) {
    // TODO?!
    //
    // If the input string is empty, return a single command line argument
    // consisting of the absolute path of the executable...

    std::string arg;

    if (next != last && !cl::impl::IsWhitespace(*next)) {
        bool const quoting = (*next == '"');

        if (quoting) {
            ++next;
        }

        for (; next != last; ++next) {
            auto const ch = *next;
            if ((quoting && ch == '"') || (!quoting && cl::impl::IsWhitespace(ch))) {
                ++next;
                break;
            }
            arg += ch;
        }
    }

    fn(std::move(arg));

    return next;
}

template <typename It, typename Fn>
It ParseArgWindows(It next, It last, Fn fn) {
    std::string arg;

    bool quoting = false;
    bool recently_closed = false;
    size_t num_backslashes = 0;

    next = cl::impl::SkipWhitespace(next, last);

    for (; next != last; ++next) {
        auto const ch = *next;

        if (ch == '"' && recently_closed) {
            recently_closed = false;

            // If a closing " is followed immediately by another ", the 2nd
            // " is accepted literally and added to the parameter.
            //
            // See:
            // http://www.daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC

            arg += '"';
        } else if (ch == '"') {
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

            if (even) {
                recently_closed = quoting; // Remember if this is a closing "
                quoting = !quoting;
            } else {
                arg += '"';
            }
        } else if (ch == '\\') {
            recently_closed = false;

            ++num_backslashes;
        } else {
            recently_closed = false;

            // Backslashes are interpreted literally, unless they
            // immediately precede a double quotation mark.

            arg.append(num_backslashes, '\\');
            num_backslashes = 0;

            if (!quoting && cl::impl::IsWhitespace(ch)) {
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

    if (!arg.empty() || quoting || recently_closed) {
        fn(std::move(arg));
    }

    return next;
}

} // namespace impl

// Parse arguments from a command line string into an argv-array.
// Using Bash-style escaping.
inline std::vector<std::string> TokenizeUnix(string_view str) {
    std::vector<std::string> argv;

    auto push_back = [&](std::string arg) {
        argv.push_back(std::move(arg));
    };

    auto next = str.data();
    auto const last = str.data() + str.size();

    while (next != last) {
        next = cl::impl::ParseArgUnix(next, last, push_back);
    }

    return argv;
}

enum class ParseProgramName : uint8_t {
    no,
    yes,
};

// Parse arguments from a command line string into an argv-array.
// Using Windows-style escaping.
inline std::vector<std::string> TokenizeWindows(string_view str, ParseProgramName parse_program_name = ParseProgramName::yes) {
    std::vector<std::string> argv;

    auto push_back = [&](std::string arg) {
        argv.push_back(std::move(arg));
    };

    auto next = str.data();
    auto const last = str.data() + str.size();

    if (parse_program_name == ParseProgramName::yes) {
        next = cl::impl::ParseProgramNameWindows(next, last, push_back);
    }

    while (next != last) {
        next = cl::impl::ParseArgWindows(next, last, push_back);
    }

    return argv;
}

#if CL_HAS_STD_STRING_VIEW
inline std::vector<std::string> TokenizeWindows(std::wstring_view str, ParseProgramName parse_program_name = ParseProgramName::yes) {
    return cl::TokenizeWindows(cl::impl::ToUTF8(str.begin(), str.end()), parse_program_name);
}
#else
inline std::vector<std::string> TokenizeWindows(wchar_t const* wstr, ParseProgramName parse_program_name = ParseProgramName::yes) {
    return cl::TokenizeWindows(cl::impl::ToUTF8(wstr), parse_program_name);
}

inline std::vector<std::string> TokenizeWindows(wchar_t const* wstr, size_t wlen, ParseProgramName parse_program_name = ParseProgramName::yes) {
    return cl::TokenizeWindows(cl::impl::ToUTF8(wstr, wstr + wlen), parse_program_name);
}
#endif

//==================================================================================================
//
//==================================================================================================

inline OptionBase::OptionBase(char const* name, char const* descr, OptionFlags flags)
    : name_(name)
    , descr_(descr)
    , flags_(flags)
{
    CL_ASSERT(cl::impl::IsUTF8(name_.begin(), name_.end()));
    CL_ASSERT(cl::impl::IsUTF8(descr_.begin(), descr_.end()));
}

inline OptionBase::~OptionBase() = default;

inline bool OptionBase::IsOccurrenceAllowed() const {
    if (HasFlag(Multiple::no)) {
        return Count() == 0;
    }

    return true;
}

inline bool OptionBase::IsOccurrenceRequired() const {
    if (HasFlag(Required::yes)) {
        return Count() == 0;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

template <typename ParserT>
template <typename ParserInit>
inline Option<ParserT>::Option(char const* name, char const* descr, OptionFlags flags, ParserInit&& parser)
    : OptionBase(name, descr, flags)
    , parser_(std::forward<ParserInit>(parser))
{
}

template <typename ParserT>
bool Option<ParserT>::Parse(ParseContext const& ctx) {
    CL_ASSERT(cl::impl::IsUTF8(ctx.name.begin(), ctx.name.end()));
    CL_ASSERT(cl::impl::IsUTF8(ctx.arg.begin(), ctx.arg.end()));

    return DoParse(ctx, std::is_convertible<decltype(parser_(ctx)), bool>{});
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

inline Cmdline::Cmdline(char const* name, char const* descr)
    : name_(name)
    , descr_(descr)
{
}

inline Cmdline::~Cmdline() = default;

template <typename... Args>
CL_FORCE_INLINE void Cmdline::EmitDiag(Diagnostic::Type type, int index, Args&&... args) {
    string_view strings[] = {args...};
    EmitDiagImpl(type, index, strings, sizeof...(Args));
}

template <typename ParserInit>
Option<std::decay_t<ParserInit>>* Cmdline::Add(char const* name, char const* descr, OptionFlags flags, ParserInit&& parser) {
    auto opt = std::make_unique<Option<std::decay_t<ParserInit>>>(
        name, descr, flags, std::forward<ParserInit>(parser));

    auto const p = opt.get();
    Add(std::move(opt));
    return p;
}

inline OptionBase* Cmdline::Add(std::unique_ptr<OptionBase> opt) {
    auto const p = opt.get();
    unique_options_.push_back(std::move(opt));
    return Add(p);
}

inline OptionBase* Cmdline::Add(OptionBase* opt) {
    CL_ASSERT(opt != nullptr);
    CL_ASSERT(cl::impl::IsUTF8(opt->name_.begin(), opt->name_.end()));
    CL_ASSERT(cl::impl::IsUTF8(opt->descr_.begin(), opt->descr_.end()));

    cl::impl::Split(opt->Name(), cl::impl::ByChar('|'), [&](string_view name) {
        CL_ASSERT(!name.empty() && "Empty option names are not allowed");
        CL_ASSERT(name[0] != '-' && "Option names must not start with a '-'");
        CL_ASSERT(name.find('"') == string_view::npos && "An option name must not contain an '\"'");
        // TODO:
        // Abort/throw if an option with the given name already exists?
        CL_ASSERT(FindOption(name) == nullptr && "Option already exists");

        if (opt->HasFlag(MayJoin::yes)) {
            auto const n = static_cast<int>(name.size());
            if (max_prefix_len_ < n) {
                max_prefix_len_ = n;
            }
        }

        options_.emplace_back(name, opt);

        return true;
    });

    return opt;
}

inline void Cmdline::Reset() {
    diag_.clear();
    curr_positional_ = 0;
    curr_index_ = 0;
    dashdash_ = false;

    ForEachUniqueOption([](OptionBase* opt) {
        opt->count_ = 0;
        return true;
    });
}

template <typename It, typename EndIt>
Cmdline::ParseResult<It> Cmdline::Parse(It curr, EndIt last, CheckMissingOptions check_missing) {
    CL_ASSERT(curr_positional_ >= 0);
    CL_ASSERT(curr_index_ >= 0);

    while (curr != last) {
        // Make a copy of the current value.
        // NB: This is actually only needed for InputIterator's...
        auto const arg = cl::impl::ToUTF8(*curr);

        Status const res = Handle1(arg, curr, last);
        switch (res) {
        case Status::success:
            break;
        case Status::done:
            if (curr != last) {
                ++curr;
            }
            break;
        case Status::error:
            return {curr, false};
        case Status::ignored:
            EmitDiag(Diagnostic::error, curr_index_, "unknown option '", arg, "'");
            return {curr, false};
        }

        // Handle1 might have changed CURR.
        // Need to recheck if we're done.
        if (res == Status::done || curr == last) {
            break;
        }

        ++curr;
        ++curr_index_;
    }

    bool const success = (check_missing == CheckMissingOptions::yes)
                             ? !AnyMissing()
                             : true;

    return {curr, success};
}

template <typename Container>
bool Cmdline::ParseArgs(Container const& args, CheckMissingOptions check_missing) {
    using std::begin; // using ADL!
    using std::end;   // using ADL!

    return Parse(begin(args), end(args), check_missing).success;
}

inline bool Cmdline::AnyMissing() {
    bool res = false;
    ForEachUniqueOption([&](OptionBase* opt) {
        if (opt->IsOccurrenceRequired()) {
            EmitDiag(Diagnostic::error, -1, "option '", opt->Name(), "' is missing");
            res = true;
        }
        return true;
    });

    return res;
}

#if CL_WINDOWS_CONSOLE_COLORS && _WIN32

inline void Cmdline::PrintDiag() const {
    if (diag_.empty()) {
        return;
    }

    auto const stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
    if (stderr_handle == NULL) {
        return; // No console.
    }

    if (stderr_handle == INVALID_HANDLE_VALUE) {
        return; // Error (Print without colors?!)
    }

    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(stderr_handle, &sbi);

    auto const old_attributes = sbi.wAttributes;

    for (auto const& d : diag_) {
        fflush(stderr);

        fprintf(stderr, "%.*s: ", static_cast<int>(name_.size()), name_.data());
        fflush(stderr);

        switch (d.type) {
        case Diagnostic::error:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
            fprintf(stderr, "error:");
            break;
        case Diagnostic::warning:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            fprintf(stderr, "warning:");
            break;
        case Diagnostic::note:
            SetConsoleTextAttribute(stderr_handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            fprintf(stderr, "note:");
            break;
        }
        fflush(stderr);
        SetConsoleTextAttribute(stderr_handle, old_attributes);
        fprintf(stderr, " %s\n", d.message.c_str());
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

inline void Cmdline::PrintDiag() const {
    for (auto const& d : diag_) {
        fprintf(stderr, "%.*s: ", static_cast<int>(name_.size()), name_.data());

        switch (d.type) {
        case Diagnostic::error:
            fprintf(stderr, CL_VT100_RED "error:" CL_VT100_RESET " %s\n", d.message.c_str());
            break;
        case Diagnostic::warning:
            fprintf(stderr, CL_VT100_MAGENTA "warning:" CL_VT100_RESET " %s\n", d.message.c_str());
            break;
        case Diagnostic::note:
            fprintf(stderr, CL_VT100_CYAN "note:" CL_VT100_RESET " %s\n", d.message.c_str());
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

inline size_t AppendSingleLine(std::string& out, string_view line, size_t indent, size_t column_width, size_t col, bool indent_first_piece) {
    bool do_indent = indent_first_piece;

    cl::impl::Split(line, cl::impl::ByWords(column_width), [&](string_view piece) {
        if (do_indent) {
            out += '\n';
            out.append(indent, ' ');
            col = indent;
        } else {
            do_indent = true;
        }
        out.append(piece.data(), piece.size());
        col += piece.size();
        return true;
    });

    return col;
}

// Assumes: currently at column = 'indent'
inline void AppendLines(std::string& out, string_view text, size_t indent, size_t column_width) {
    CL_ASSERT(indent < column_width);

    bool do_indent = false; // Do not indent the first line.

    cl::impl::Split(text, cl::impl::ByLines(), [&](string_view line) {
        // Find the position of the first tab-character in this line (if any).
        auto const tab_pos = line.find('\t');
        CL_ASSERT((tab_pos == string_view::npos || line.find('\t', tab_pos + 1) == string_view::npos) && "Only a single tab-character per line is allowed");

        // Append the first (or only) part of this line.
        auto const col = cl::impl::AppendSingleLine(out, line.substr(0, tab_pos), indent, column_width, /*col*/ indent, do_indent);

        // If there is a tab-character, print the second half of this line.
        if (tab_pos != string_view::npos) {
            CL_ASSERT(col >= indent);

            auto const block_col  = (col - indent) % column_width;
            auto const new_indent = indent + block_col;
            auto const new_width  = column_width - block_col;

            cl::impl::AppendSingleLine(out, line.substr(tab_pos + 1), new_indent, new_width, /*col (ignored)*/ 0, /*do_indent*/ false);
        }

        do_indent = true;
        return true;
    });
}

// Append the name of the option along with a short description of its argument (if any)
// Note: not wrapped.
inline void AppendUsage(std::string& out, OptionBase* opt, size_t indent) {
    out.append(indent, ' ');
    if (opt->HasFlag(Positional::yes)) {
        out += '<';
        out.append(opt->Name().data(), opt->Name().size());
        out += '>';
        if (opt->HasFlag(Multiple::yes)) {
            out += "...";
        }
    } else {
        out += "--";
        out.append(opt->Name().data(), opt->Name().size());
        if (opt->HasFlag(Arg::required) && opt->HasFlag(MayJoin::yes)) {
            out += "<arg>";
        } else if (opt->HasFlag(Arg::required)) {
            out += " <arg>";
        } else if (opt->HasFlag(Arg::optional)) {
            out += "=<arg>";
        }
    }
}

inline void AppendDescr(std::string& out, OptionBase* opt, size_t indent, size_t descr_indent, size_t descr_width) {
    auto const col0 = out.size();
    CL_ASSERT(col0 == 0 || out[col0 - 1] == '\n');

    // Append the name of the option along with a short description of its argument (if any)
    // Note: not wrapped.
    AppendUsage(out, opt, indent);

    if (!opt->Descr().empty()) {
        // Move to column 'descr_width'.
        // Possibly on the next line.
        auto const col     = out.size() - col0;
        auto const wrap    = col >= descr_indent;
        auto const nspaces = wrap ? descr_indent : descr_indent - col;
        if (wrap) {
            out += '\n';
        }
        out.append(nspaces, ' ');

        // Append the options description.
        cl::impl::AppendLines(out, opt->Descr(), descr_indent, descr_width);
    }

    out += '\n';
}

} // namespace impl

inline std::string Cmdline::FormatHelp(HelpFormat const& fmt) const {
    CL_ASSERT(fmt.descr_indent > fmt.indent);
    CL_ASSERT(fmt.descr_indent < SIZE_MAX);

    auto const line_length = (fmt.line_length == 0) ? SIZE_MAX : fmt.line_length;
    CL_ASSERT(line_length > fmt.descr_indent);
    auto const descr_width = line_length - fmt.descr_indent;

    std::string out;

    out.append(Name().data(), Name().size());
    out += " - ";
    out.append(Descr().data(), Descr().size());
    out += "\n\nUsage:\n";
    out.append(fmt.indent, ' ');
    out.append(Name().data(), Name().size());

    std::string arg_descr = "\nArguments:\n";
    std::string opt_descr = "\nOptions:\n";
    std::string pos_descr = "\nPositional options:\n";

    bool has_pos = false;
    bool has_arg = false;
    bool has_opt = false;
    ForEachUniqueOption([&](OptionBase* opt) {
        std::string* descr;

        if (!opt->HasFlag(Required::no)) { // Required option, aka 'argument'.
            has_arg = true;
            descr = &arg_descr;

            // Append this argument to the usage line.
            cl::impl::AppendUsage(out, opt, 1);
        } else if (!opt->HasFlag(Positional::yes)) {
            has_opt = true;
            descr = &opt_descr;
        } else {
            has_pos = true;
            descr = &pos_descr;
        }

        cl::impl::AppendDescr(*descr, opt, fmt.indent, fmt.descr_indent, descr_width);
        return true;
    });

    if (has_opt) {
        out += " [options]";
    }
    out += '\n';
    if (has_arg) {
        out += arg_descr;
    }
    if (has_opt) {
        out += opt_descr;
    }
    if (has_pos) {
        out += pos_descr;
    }

    CL_ASSERT(cl::impl::IsUTF8(out.begin(), out.end()));
    return out;
}

inline void Cmdline::PrintHelp(HelpFormat const& fmt) const {
    auto const msg = FormatHelp(fmt);
    fprintf(stderr, "%s\n", msg.c_str());
}

inline void Cmdline::DebugCheck() const
{
}

inline OptionBase* Cmdline::FindOption(string_view name) const {
    for (auto&& p : options_) {
        // NB: Don't skip positional options.
        // Positional options have a name and might still be provided
        // in the form "--name=value".
        if (p.name == name) {
            return p.option;
        }
    }

    return nullptr;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::Handle1(string_view optstr, It& curr, EndIt last) {
    CL_ASSERT(curr != last);

    // This cannot happen if we're parsing the main's argv[] array, but it might
    // happen if we're parsing a user-supplied array of command line arguments.
    if (optstr.empty()) {
        return Status::success;
    }

    // Stop parsing if "--" has been found
    if (optstr == "--" && !dashdash_) {
        dashdash_ = true;
        return Status::success;
    }

    // This argument is considered to be positional if it doesn't start with
    // '-', if it is "-" itself, or if we have seen "--" already, or if the
    // argument doesn't look like a known option (see below).
    bool const is_positional = (optstr[0] != '-' || optstr == "-" || dashdash_);
    if (is_positional) {
        return HandlePositional(optstr);
    }

    auto const optstr_orig = optstr;

    CL_ASSERT(optstr[0] == '-');
    optstr.remove_prefix(1); // Remove the first dash.

    // If the name starts with a single dash, this is a short option and might
    // actually be an option group.
    bool const is_short = (optstr[0] != '-');
    if (!is_short) {
        optstr.remove_prefix(1); // Remove the second dash.
    }

    // 1. Try to handle options like "-f" and "-f file"
    Status res = HandleStandardOption(optstr, curr, last);

    // 2. Try to handle options like "-f=file"
    if (res == Status::ignored) {
        res = HandleOption(optstr);
    }

    // 3. Try to handle options like "-Idir"
    if (res == Status::ignored) {
        res = HandlePrefix(optstr);
    }

    // 4. Try to handle options like "-xvf=file" and "-xvf file"
    if (res == Status::ignored && is_short) {
        res = HandleGroup(optstr, curr, last);
    }

    // Otherwise this is an unknown option.
    //
    // 5. Try to handle this option as a positional option.
    //    If there are no more (hungry) positional options, this is an error.
    if (res == Status::ignored) {
        res = HandlePositional(optstr_orig);
    }

    return res;
}

inline Cmdline::Status Cmdline::HandlePositional(string_view optstr) {
    auto const E = static_cast<int>(options_.size());
    CL_ASSERT(curr_positional_ >= 0);
    CL_ASSERT(curr_positional_ <= E);

    for (; curr_positional_ != E; ++curr_positional_) { // find_if
        auto opt = options_[static_cast<size_t>(curr_positional_)].option;

        if (!opt->HasFlag(Positional::yes)) {
            continue;
        }
        if (!opt->IsOccurrenceAllowed()) {
            continue;
        }

        // The argument of a positional option is the value specified on the
        // command line.
        return HandleOccurrence(opt, opt->Name(), optstr);
    }

    return Status::ignored;
}

// If OPTSTR is the name of an option, handle the option.
template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleStandardOption(string_view optstr, It& curr, EndIt last) {
    if (auto const opt = FindOption(optstr)) {
        // OPTSTR is the name of an option, i.e. no argument was specified.
        // If the option requires an argument, steal one from the command line.
        return HandleOccurrence(opt, optstr, curr, last);
    }

    return Status::ignored;
}

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
inline Cmdline::Status Cmdline::HandleOption(string_view optstr) {
    auto arg_start = optstr.find('=');

    if (arg_start != string_view::npos) {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        if (auto const opt = FindOption(name)) {
            // Ok, something like "-f=file".

            // Discard the equals sign if this option may NOT join its value.
            if (opt->HasFlag(MayJoin::no)) {
                ++arg_start;
            }

            return HandleOccurrence(opt, name, optstr.substr(arg_start));
        }
    }

    return Status::ignored;
}

inline Cmdline::Status Cmdline::HandlePrefix(string_view optstr) {
    // Scan over all known prefix lengths.
    // Start with the longest to allow different prefixes like e.g. "-with" and
    // "-without".

    auto n = static_cast<size_t>(max_prefix_len_);
    if (n > optstr.size()) {
        n = optstr.size();
    }

    for (; n != 0; --n) {
        auto const name = optstr.substr(0, n);
        auto const opt = FindOption(name);

        if (opt != nullptr && !opt->HasFlag(MayJoin::no)) {
            return HandleOccurrence(opt, name, optstr.substr(n));
        }
    }

    return Status::ignored;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleGroup(string_view optstr, It& curr, EndIt last) {
    //
    // XXX:
    // Remove and call FindOption() in the second loop again?!
    //
    std::vector<OptionBase*> group;

    // First determine the largest prefix which is a valid option group.
    for (size_t n = 0; n < optstr.size(); ++n) {
        if (optstr[n] == '=') {
            break;
        }

        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->HasFlag(MayGroup::no)) {
            return Status::ignored;
        }

        group.push_back(opt);

        if (!opt->HasFlag(Arg::no)) {
            // The option accepts an argument.
            // This terminates the option group.
            break;
        }
    }

    if (group.empty()) { // "-=" is invalid
        return Status::ignored;
    }

    // Then process all options.
    for (size_t n = 0; n < group.size(); ++n) {
        auto const name = optstr.substr(n, 1);
        auto const opt = group[n];

        CL_ASSERT(opt != nullptr);
        CL_ASSERT(opt->HasFlag(MayGroup::yes));

        if (n + 1 != group.size() || group.size() == optstr.size()) {
            // This is either an option which does not allow an argument (which may
            // or may not be the last option in the group), or it is the last option and
            // an argument has not been provided.
            // In case the option has the HasArg::required flag set, HandleOccurrence
            // will try to get an argument from the command line.
            if (Status::success != HandleOccurrence(opt, name, curr, last)) {
                return Status::error;
            }
        } else {
            // This is the last option in the group and the argument is the rest of optstr.
            // In case an argument is not allowed, HandleOccurrence will emit an error.
            size_t arg_start = n + 1;

            if (opt->HasFlag(MayJoin::no)) {
                // The option may not join its argument.
                // If the next character is an '=', this is like "--f=filename",
                // so discard the equals sign.
                // Otherwise this is an error.
                if (optstr[arg_start] != '=') {
                    EmitDiag(Diagnostic::error, curr_index_, "option '", name, "' must not join its argument");
                    return Status::error;
                }

                ++arg_start;
            }

            return HandleOccurrence(opt, name, optstr.substr(arg_start));
        }
    }

    return Status::success;
}

template <typename It, typename EndIt>
Cmdline::Status Cmdline::HandleOccurrence(OptionBase* opt, string_view name, It& curr, EndIt last) {
    CL_ASSERT(curr != last);

    // We get here if no argument was specified.
    // If an argument is required, try to steal one from the command line.

    if (!opt->HasFlag(Arg::required)) {
        return ParseOptionArgument(opt, name, {});
    }

    // If the option requires an argument, steal one from the command line.
    ++curr;
    ++curr_index_;

    if (curr != last) {
        auto const arg = cl::impl::ToUTF8(*curr);

#if 1
        // If the string is of the form "--K=V" and "K" is the name of
        // an option, emit a warning.
        if (arg.size() >= 2 && arg[0] == '-') {
            auto n = string_view(arg);
            n.remove_prefix(1);
            if (n[0] == '-') {
                n.remove_prefix(1);
            }
            n = n.substr(0, n.find('='));

            if (FindOption(n) != nullptr) {
                EmitDiag(Diagnostic::warning, curr_index_, "option '", n, "' is used as an argument for option '", name, "'");
                EmitDiag(Diagnostic::note, curr_index_, "use '--", name, opt->HasFlag(MayJoin::yes) ? "" : "=", arg, "' to suppress this warning");
            }
        }
#endif

        return ParseOptionArgument(opt, name, arg);
    }

    EmitDiag(Diagnostic::error, curr_index_, "option '", name, "' requires an argument");
    return Status::error;
}

inline Cmdline::Status Cmdline::HandleOccurrence(OptionBase* opt, string_view name, string_view arg) {
    // An argument was specified for OPT.

    if (opt->HasFlag(Positional::no) && opt->HasFlag(Arg::no)) {
        EmitDiag(Diagnostic::error, curr_index_, "option '", name, "' does not accept an argument");
        return Status::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

inline Cmdline::Status Cmdline::ParseOptionArgument(OptionBase* opt, string_view name, string_view arg) {
    auto Parse1 = [&](string_view arg1) {
        if (!opt->IsOccurrenceAllowed()) {
            // Use opt->Name() instead of name here.
            // This gives slightly nicer error messages in case an option has
            // multiple names.
            EmitDiag(Diagnostic::error, curr_index_, "option '", opt->Name(), "' already specified");
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
        auto const num_diagnostics = diag_.size();

        if (!opt->Parse(ctx)) {
            bool const diagnostic_emitted = diag_.size() > num_diagnostics;
            if (!diagnostic_emitted) {
                EmitDiag(Diagnostic::error, curr_index_, "invalid argument '", arg1, "' for option '", name, "'");
            }

            return Status::error;
        }

        ++opt->count_;
        return Status::success;
    };

    Status res = Status::success;

    if (opt->HasFlag(CommaSeparated::yes)) {
        cl::impl::Split(arg, cl::impl::ByChar(','), [&](string_view s) {
            res = Parse1(s);
            if (res != Status::success) {
                EmitDiag(Diagnostic::note, curr_index_, "in comma-separated argument '", arg, "'");
                return false;
            }
            return true;
        });
    } else {
        res = Parse1(arg);
    }

    if (res == Status::success) {
        // If the current option has the StopParsing flag set, we're done.
        if (opt->HasFlag(StopParsing::yes)) {
            res = Status::done;
        }
    }

    return res;
}

template <typename Fn>
bool Cmdline::ForEachUniqueOption(Fn fn) const {
    auto I = options_.begin();
    auto const E = options_.end();

    if (I == E) {
        return true;
    }

    for (;;) {
        if (!fn(I->option)) {
            return false;
        }

        // Skip duplicate options.
        auto const curr_opt = I->option;
        for (;;) {
            if (++I == E) {
                return true;
            }
            if (I->option != curr_opt) {
                break;
            }
        }
    }
}

inline void Cmdline::EmitDiagImpl(Diagnostic::Type type, int index, string_view const* strings, int num_strings) {
    diag_.emplace_back(type, index, std::string{});

    auto& text = diag_.back().message;
    for (int i = 0; i < num_strings; ++i) {
        auto s = strings[i];
        CL_ASSERT(cl::impl::IsUTF8(s.begin(), s.end()));
        text.append(s.data(), s.size());
    }

    //fprintf(stderr, "%s\n", text.c_str());
}

} // namespace cl

#endif // CL_CMDLINE_H
