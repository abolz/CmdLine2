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

#include "Cmdline.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <limits>
#if defined(_WIN32)
#include <windows.h>
#undef min
#undef max
#endif

using namespace cl;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

static int MemCompare(char const* s1, char const* s2, size_t n)
{
    // memcmp is undefined for nullptr's even if n == 0.
    return n == 0 ? 0 : ::memcmp(s1, s2, n);
}

static char const* MemFind(char const* s, size_t n, char ch)
{
    // memchr is undefined for nullptr's even if n == 0.
    // But here it should never be called with n == 0...
    assert(n != 0);
    return static_cast<char const*>( ::memchr(s, static_cast<unsigned char>(ch), n) );
}

bool string_view::_cmp_eq(string_view other) const
{
    return size() == other.size() && MemCompare(data(), other.data(), size()) == 0;
}

bool string_view::_cmp_lt(string_view other) const
{
    int const c = MemCompare(data(), other.data(), Min(size(), other.size()));
    return c < 0 || (c == 0 && size() < other.size());
}

int string_view::compare(string_view other) const
{
    int const c = MemCompare(data(), other.data(), Min(size(), other.size()));
    if (c != 0)
        return c;
    if (size() < other.size())
        return -1;
    if (size() > other.size())
        return +1;
    return 0;
}

size_t string_view::find(char ch, size_t from) const
{
    if (from >= size())
        return npos;

    if (auto I = MemFind(data() + from, size() - from, ch))
        return static_cast<size_t>(I - data());

    return npos;
}

static std::string& operator+=(std::string& lhs, string_view rhs)
{
    lhs.append(rhs.data(), rhs.size());
    return lhs;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace {

// Return type for delimiters.
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

struct CharDelimiter
{
    char const ch;
    explicit CharDelimiter(char ch_) : ch(ch_) {}

    DelimiterResult operator()(string_view str) const
    {
        return {str.find(ch), 1};
    }
};

struct LineDelimiter
{
    DelimiterResult operator()(string_view str) const
    {
        auto const first = str.data();
        auto const last  = str.data() + str.size();

        // Find the position of the first CR or LF
        auto p = first;
        while (p != last && (*p != '\n' && *p != '\r'))
        {
            ++p;
        }

        if (p == last)
            return {string_view::npos, 0};

        auto const index = static_cast<size_t>(p - first);

        // If this is CRLF, skip the other half.
        if (p + 1 != last)
        {
            if (p[0] == '\r' && p[1] == '\n')
                return {index, 2};
        }

        return {index, 1};
    }
};

struct WrapDelimiter
{
    size_t const length;
    explicit WrapDelimiter(size_t length_) : length(length_)
    {
        assert(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(string_view str) const
    {
        // If the string fits into the current line, just return this last line.
        if (str.size() <= length)
            return {string_view::npos, 0};

        // Otherwise, search for the first space preceding the line length.
        for (size_t i = length; i != 0; /**/)
        {
            char const ch = str[--i];

            if (ch == ' ' || ch == '\t')
                return {i, 1};
        }

        return {length, 0}; // No space in current line, break at length.
    }
};

struct DoSplitResult
{
    string_view tok; // The current token.
    string_view str; // The rest of the string.
    bool last = false;
};

static bool DoSplit(DoSplitResult& res, string_view str, DelimiterResult del)
{
    //
    // +-----+-----+------------+
    // ^ tok ^     ^    rest    ^
    //       f     f+c
    //

    if (del.first == string_view::npos)
    {
        res.tok = str;
//      res.str = {};
        res.last = true;
        return true;
    }

    assert(del.first + del.count >= del.first);
    assert(del.first + del.count <= str.size());

    size_t const off = del.first + del.count;
    assert(off > 0 && "invalid delimiter result");

    res.tok = str.substr(0, del.first);
    res.str = str.substr(off);
    return true;
}

template <typename Delimiter, typename Function>
static bool SplitString(string_view str, Delimiter&& delim, Function&& fn)
{
    DoSplitResult curr;

    curr.tok = {};
    curr.str = str;
    curr.last = false;

    for (;;)
    {
        if (!DoSplit(curr, curr.str, delim(curr.str)))
            return true;
        if (!fn(curr.tok))
            return false;
        if (curr.last)
            return true;
    }
}

} // namespace

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

OptionBase::~OptionBase()
{
}

bool OptionBase::IsOccurrenceAllowed() const
{
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::optional)
        return count_ == 0;
    return true;
}

bool OptionBase::IsOccurrenceRequired() const
{
    if (num_opts_ == NumOpts::required || num_opts_ == NumOpts::one_or_more)
        return count_ == 0;
    return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

Cmdline::Cmdline()
{
}

Cmdline::~Cmdline()
{
}

void Cmdline::EmitDiag(Diagnostic::Type type, int index, std::string message)
{
    diag_.push_back({type, index, std::move(message)});
}

void Cmdline::FormatDiag(Diagnostic::Type type, int index, char const* format, ...)
{
    const size_t kBufSize = 1024;
    char buf[kBufSize];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, kBufSize, format, args);
    va_end(args);

    diag_.push_back({type, index, std::string(buf)});
}

void Cmdline::Add(std::unique_ptr<OptionBase> opt)
{
    auto const p = opt.get();

    unique_options_.push_back(std::move(opt));

    Add(p);
}

void Cmdline::Add(OptionBase* opt)
{
    assert(opt != nullptr);

    SplitString(opt->name_, CharDelimiter('|'), [&](string_view name)
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

void Cmdline::Add(std::initializer_list<OptionBase*> opts)
{
    for (auto& opt : opts)
    {
        Add(opt);
    }
}

void Cmdline::Reset()
{
    curr_positional_ = 0;
    curr_index_      = 0;
    dashdash_        = false;

    ForEachUniqueOption([](string_view /*name*/, OptionBase* opt)
    {
        opt->count_ = 0;
        return true;
    });
}

// Emit errors for ALL missing options.
bool Cmdline::AnyMissing()
{
    bool res = false;

    ForEachUniqueOption([&](string_view /*name*/, OptionBase* opt)
    {
        if (opt->IsOccurrenceRequired())
        {
            FormatDiag(Diagnostic::error, -1, "Option '%.*s' is missing", static_cast<int>(opt->name_.size()), opt->name_.data());
            res = true;
        }

        return true;
    });

    return res;
}

#if defined(_WIN32)

void Cmdline::PrintDiag() const
{
    if (diag_.empty())
        return;

    HANDLE const stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(stderr_handle, &sbi);

    WORD const old_color_attributes = sbi.wAttributes;

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
        SetConsoleTextAttribute(stderr_handle, old_color_attributes);

        fprintf(stderr, ": %s\n", d.message.c_str());
    }
}

#else

#define CL_VT100_RESET      "\x1B[0m"
#define CL_VT100_RED        "\x1B[31;1m"
#define CL_VT100_MAGENTA    "\x1B[35;1m"
#define CL_VT100_CYAN       "\x1B[36;1m"

void Cmdline::PrintDiag() const
{
    for (auto const& d : diag_)
    {
        switch (d.type)
        {
        case Diagnostic::error:
            fprintf(stderr, CL_VT100_RED "Error" CL_VT100_RESET);
            break;
        case Diagnostic::warning:
            fprintf(stderr, CL_VT100_MAGENTA "Warning" CL_VT100_RESET);
            break;
        case Diagnostic::note:
            fprintf(stderr, CL_VT100_CYAN "Note" CL_VT100_RESET);
            break;
        }

        fprintf(stderr, ": %s\n", d.message.c_str());
    }
}

#undef CL_VT100_RESET
#undef CL_VT100_RED
#undef CL_VT100_MAGENTA
#undef CL_VT100_CYAN

#endif

static void AppendAligned(std::string& out, string_view text, size_t indent, size_t width)
{
    if (text.size() < width && indent < width - text.size())
    {
        out.append(indent, ' ');
        out += text;
        out.append(width - text.size() - indent, ' ');
    }
    else
    {
        out.append(indent, ' ');
        out += text;
        out += '\n';
        out.append(width, ' ');
    }
}

static void AppendWrapped(std::string& out, string_view text, size_t indent, size_t width)
{
    assert(indent < width);

    bool first = true;

    // Break the string into paragraphs
    SplitString(text, LineDelimiter(), [&](string_view par)
    {
        // Break the paragraphs at the maximum width into lines
        return SplitString(par, WrapDelimiter(width), [&](string_view line)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                out += '\n';
                out.append(indent, ' ');
            }

            out += line;

            return true;
        });
    });
}

std::string Cmdline::FormatHelp(string_view program_name, size_t indent, size_t descr_indent, size_t max_width) const
{
    assert(descr_indent > indent);
    assert(max_width == 0 || max_width > descr_indent);

    std::string spos;
    std::string sopt;

    ForEachUniqueOption([&](string_view /*name*/, OptionBase* opt)
    {
        if (opt->positional_ == Positional::yes)
        {
            spos += ' ';
            if ((opt->num_opts_ == NumOpts::optional) || (opt->num_opts_ == NumOpts::zero_or_more))
            {
                spos += '[';
                spos += opt->name_;
                spos += ']';
            }
            else
            {
                spos += opt->name_;
            }
        }
        else
        {
            std::string usage;

            usage += '-';
            usage += opt->name_;

            switch (opt->has_arg_)
            {
            case HasArg::no:
                break;
            case HasArg::optional:
                usage += "=<ARG>";
                break;
            case HasArg::required:
                usage += " <ARG>";
                break;
            }

            if (opt->descr_.empty())
            {
                sopt.append(indent, ' ');
                sopt += usage;
            }
            else
            {
                AppendAligned(sopt, usage, indent, descr_indent);
                if (max_width == 0)
                    sopt += opt->descr_;
                else
                    AppendWrapped(sopt, opt->descr_, descr_indent, max_width - descr_indent);
            }

            sopt += '\n'; // One option per line
        }

        return true;
    });

    if (sopt.empty())
        return "Usage: " + std::string(program_name) + spos + '\n';
    else
        return "Usage: " + std::string(program_name) + " [options]" + spos + "\nOptions:\n" + sopt;
}

void Cmdline::PrintHelp(string_view program_name, size_t indent, size_t descr_indent, size_t max_width) const
{
    auto const msg = FormatHelp(program_name, indent, descr_indent, max_width);
    fprintf(stderr, "%s\n", msg.c_str());
}

OptionBase* Cmdline::FindOption(string_view name) const
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

Cmdline::Result Cmdline::HandlePositional(string_view optstr)
{
    int const E = static_cast<int>(options_.size());
    assert(curr_positional_ >= 0);
    assert(curr_positional_ <= E);

    for ( ; curr_positional_ != E; ++curr_positional_)
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

// Look for an equal sign in OPTSTR and try to handle cases like "-f=file".
Cmdline::Result Cmdline::HandleOption(string_view optstr)
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
            {
                ++arg_start;
            }

            return HandleOccurrence(opt, name, optstr.substr(arg_start));
        }
    }

    return Result::ignored;
}

Cmdline::Result Cmdline::HandlePrefix(string_view optstr)
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

        if (opt != nullptr && opt->join_arg_ != JoinArg::no)
        {
            return HandleOccurrence(opt, name, optstr.substr(n));
        }
    }

    return Result::ignored;
}

// Check if OPTSTR is actually a group of single letter options and store the
// options in GROUP.
Cmdline::Result Cmdline::DecomposeGroup(string_view optstr, std::vector<OptionBase*>& group)
{
    group.reserve(optstr.size());

    for (size_t n = 0; n < optstr.size(); ++n)
    {
        auto const name = optstr.substr(n, 1);
        auto const opt = FindOption(name);

        if (opt == nullptr || opt->may_group_ == MayGroup::no)
            return Result::ignored;

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

Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, string_view name, string_view arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->has_arg_ == HasArg::no)
    {
        FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' does not accept an argument",
            static_cast<int>(name.size()), name.data());
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, string_view name, string_view arg)
{
    auto Parse1 = [&](string_view arg1)
    {
        if (!opt->IsOccurrenceAllowed())
        {
            FormatDiag(Diagnostic::error, curr_index_, "Option '%.*s' already specified",
                static_cast<int>(name.size()), name.data());
            return Result::error;
        }

        ParseContext ctx;

        ctx.name    = name;
        ctx.arg     = arg1;
        ctx.index   = curr_index_;
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

            return Result::error;
        }

        ++opt->count_;
        return Result::success;
    };

    Result res = Result::success;

    if (opt->comma_separated_ == CommaSeparated::yes)
    {
        SplitString(arg, CharDelimiter(','), [&](string_view s)
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
        {
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

    if (I == E)
        return true;

    for (;;)
    {
        if (!fn(I->name, I->option))
            return false;

        // Skip duplicate options.
        auto const* curr_opt = I->option;
        for (;;)
        {
            if (++I == E)
                return true;
            if (I->option != curr_opt)
                break;
        }
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename ...Match>
static bool IsAnyOf(string_view value, Match&&... match)
{
#if __cplusplus >= 201703 || __cpp_fold_expressions >= 201411
    return (... || (value == match));
#else
    bool res = false;
    bool unused[] = { false, (res = res || (value == match))... };
    static_cast<void>(unused);
    return res;
#endif
}

bool cl::ConvertValue<bool>::operator()(string_view str, bool& value) const
{
    if (str.empty() || IsAnyOf(str, "1", "y", "true", "True", "yes", "Yes", "on", "On"))
        value = true;
    else if (IsAnyOf(str, "0", "n", "false", "False", "no", "No", "off", "Off"))
        value = false;
    else
        return false;

    return true;
}

template <typename T, typename Fn>
static bool StrToX(string_view sv, T& value, Fn fn)
{
    if (sv.empty())
        return false;

    std::string str(sv);

    char const* const ptr = str.c_str();
    char*             end = nullptr;

    int& ec = errno;

    auto const ec0 = std::exchange(ec, 0);
    auto const val = fn(ptr, &end);
    auto const ec1 = std::exchange(ec, ec0);

    if (ec1 == ERANGE)
        return false;
    if (end != ptr + str.size()) // not all characters extracted
        return false;

    value = val;
    return true;
}

// Note:
// Wrap into local function, to avoid instantiating StrToX with different lambdas which
// actually all do the same thing: call strtol.
static bool StrToLongLong(string_view str, long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoll(p, end, 0); });
}

template <typename T>
static bool ParseInt(string_view str, T& value)
{
    long long v = 0;
    if (StrToLongLong(str, v) && v >= std::numeric_limits<T>::min() && v <= std::numeric_limits<T>::max())
    {
        value = static_cast<T>(v);
        return true;
    }
    return false;
}

bool ConvertValue<signed char>::operator()(string_view str, signed char& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed short>::operator()(string_view str, signed short& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed int>::operator()(string_view str, signed int& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed long>::operator()(string_view str, signed long& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed long long>::operator()(string_view str, signed long long& value) const
{
    return StrToLongLong(str, value);
}

// (See above)
static bool StrToUnsignedLongLong(string_view str, unsigned long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoull(p, end, 0); });
}

template <typename T>
static bool ParseUnsignedInt(string_view str, T& value)
{
    unsigned long long v = 0;
    if (StrToUnsignedLongLong(str, v) && v <= std::numeric_limits<T>::max())
    {
        value = static_cast<T>(v);
        return true;
    }
    return false;
}

bool ConvertValue<unsigned char>::operator()(string_view str, unsigned char& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned short>::operator()(string_view str, unsigned short& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned int>::operator()(string_view str, unsigned int& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned long>::operator()(string_view str, unsigned long& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned long long>::operator()(string_view str, unsigned long long& value) const
{
    return StrToUnsignedLongLong(str, value);
}

bool ConvertValue<float>::operator()(string_view str, float& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtof(p, end); });
}

bool ConvertValue<double>::operator()(string_view str, double& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtod(p, end); });
}

bool ConvertValue<long double>::operator()(string_view str, long double& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtold(p, end); });
}

bool cl::ConvertValue<std::string>::operator()(string_view str, std::string& value) const
{
    value.assign(str.data(), str.size());
    return true;
}
