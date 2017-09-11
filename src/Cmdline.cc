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

    DelimiterResult operator()(std::string const& str) const
    {
        return {str.find(ch), 1};
    }
};

struct LineDelimiter
{
    DelimiterResult operator()(std::string const& str) const
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
            return {std::string::npos, 0};

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

    DelimiterResult operator()(std::string const& str) const
    {
        // If the string fits into the current line, just return this last line.
        if (str.size() <= length)
            return {std::string::npos, 0};

        // Otherwise, search for the first space preceding the line length.
        auto I = str.find_last_of(" \t", length);

        if (I != std::string::npos) // There is a space.
            return {I, 1};

        return {length, 0}; // No space in current line, break at length.
    }
};

struct DoSplitResult
{
    std::string tok; // The current token.
    std::string str; // The rest of the string.
    bool last = false;
};

static bool DoSplit(DoSplitResult& res, std::string const& str, DelimiterResult del)
{
    //
    // +-----+-----+------------+
    // ^ tok ^     ^    rest    ^
    //       f     f+c
    //

    if (del.first == std::string::npos)
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
static bool SplitString(std::string const& str, Delimiter&& delim, Function&& fn)
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
    if (num_occurrences_ == Opt::required || num_occurrences_ == Opt::optional)
        return count_ == 0;
    return true;
}

bool OptionBase::IsOccurrenceRequired() const
{
    if (num_occurrences_ == Opt::required || num_occurrences_ == Opt::one_or_more)
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

void Cmdline::Reset()
{
    curr_positional_ = 0;
    curr_index_      = 0;
    dashdash_        = false;

    for (auto& opt : unique_options_)
        opt->count_ = 0;
}

// Emit errors for ALL missing options.
bool Cmdline::CheckMissingOptions()
{
    bool res = true;

    for (auto const& opt : unique_options_)
    {
        if (opt->IsOccurrenceRequired())
        {
            EmitDiag(Diagnostic::error, -1, "Option '" + opt->name_ + "' is missing");
            res = false;
        }
    }

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
#define CL_VT100_MAGENTA    "\x1B[36;1m"
#define CL_VT100_CYAN       "\x1B[36;1m"

void Cmdline::PrintDiag() const
{
    if (diag_.empty())
        return;

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

static void AppendAligned(std::string& out, std::string const& text, size_t indent, size_t width)
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

static void AppendWrapped(std::string& out, std::string const& text, size_t indent, size_t width)
{
    if (width <= indent)
        width = SIZE_MAX;

    bool first = true;

    // Break the string into paragraphs
    SplitString(text, LineDelimiter(), [&](std::string const& par)
    {
        // Break the paragraphs at the maximum width into lines
        return SplitString(par, WrapDelimiter(width), [&](std::string const& line)
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

static constexpr size_t kMaxWidth    = 0;
static constexpr size_t kOptIndent   = 2;
static constexpr size_t kDescrIndent = 27;

std::string Cmdline::GetHelp(std::string const& program_name) const
{
    std::string spos;
    std::string sopt;

    for (auto const& opt : unique_options_)
    {
        if (opt->positional_ == Positional::yes)
        {
            spos += ' ';
            if ((opt->num_occurrences_ == Opt::optional) || (opt->num_occurrences_ == Opt::zero_or_more))
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

            switch (opt->num_args_)
            {
            case Arg::no:
                break;
            case Arg::optional:
                usage += "=<arg>";
                break;
            case Arg::required:
                usage += " <arg>";
                break;
            }

            if (opt->descr_.empty())
            {
                sopt.append(kOptIndent, ' ');
                sopt += usage;
            }
            else
            {
                AppendAligned(sopt, usage, kOptIndent, kDescrIndent);
                AppendWrapped(sopt, opt->descr_, kDescrIndent, kMaxWidth);
            }

            sopt += '\n'; // One option per line
        }
    }

    if (sopt.empty())
        return "Usage: " + program_name + spos + '\n';
    else
        return "Usage: " + program_name + " [options]" + spos + "\nOptions:\n" + sopt;
}

void Cmdline::PrintHelp(std::string const& program_name) const
{
    auto const msg = GetHelp(program_name);
    fprintf(stderr, "%s\n", msg.c_str());
}

OptionBase* Cmdline::FindOption(std::string const& name) const
{
    for (auto&& p : options_)
    {
        if (p.name == name)
            return p.option;
    }

    return nullptr;
}

OptionBase* Cmdline::FindOption(std::string const& name, bool& ambiguous) const
{
    ambiguous = false;

#if 0 // allow abbreviations
    OptionBase* opt = nullptr;

    for (auto&& p : options_)
    {
        if (p.name == name) // exact match
        {
            ambiguous = false;
            return p.option;
        }

        if (p.name.size() > name.size() && p.name.compare(0, name.size(), name) == 0)
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

void Cmdline::DoAdd(OptionBase* opt)
{
    assert(!opt->name_.empty());

    SplitString(opt->name_, CharDelimiter('|'), [&](std::string const& name)
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

Cmdline::Result Cmdline::HandlePositional(std::string const& optstr)
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
Cmdline::Result Cmdline::HandleOption(std::string const& optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != std::string::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        bool ambiguous = false;
        if (auto const opt = FindOption(name, ambiguous))
        {
            if (ambiguous)
            {
                EmitDiag(Diagnostic::error, curr_index_, "Option '" + optstr + "' is ambiguous");
                return Result::error;
            }

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

Cmdline::Result Cmdline::HandlePrefix(std::string const& optstr)
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
Cmdline::Result Cmdline::DecomposeGroup(std::string const& optstr, std::vector<OptionBase*>& group)
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
        EmitDiag(Diagnostic::error, curr_index_, std::string("Option '") + optstr[n] + "' must be the last in a group");
        return Result::error;
    }

    return Result::success;
}

Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, std::string const& name, std::string const& arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->num_args_ == Arg::no)
    {
        EmitDiag(Diagnostic::error, curr_index_, "Option '" + name + "' does not accept an argument");
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, std::string const& name, std::string const& arg)
{
    auto Parse1 = [&](std::string const& arg1)
    {
        if (!opt->IsOccurrenceAllowed())
        {
            EmitDiag(Diagnostic::error, curr_index_, "Option '" + name + "' already specified");
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
                EmitDiag(Diagnostic::error, curr_index_, "Invalid argument '" + arg1 + "' for option '" + name + "'");
            }

            return Result::error;
        }

        ++opt->count_;
        return Result::success;
    };

    Result res = Result::success;

    if (opt->comma_separated_arg_ == CommaSeparatedArg::yes)
    {
        SplitString(arg, CharDelimiter(','), [&](std::string const& s)
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

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename ...Match>
static bool IsAnyOf(std::string const& value, Match&&... match)
{
#if __cplusplus >= 201703
    return (... || (value == match));
#else
    bool res = false;
    bool unused[] = { false, (res = res || (value == match))... };
    static_cast<void>(unused);
    return res;
#endif
}

bool cl::ConvertValue<bool>::operator()(std::string const& str, bool& value) const
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
static bool StrToX(std::string const& str, T& value, Fn fn)
{
    if (str.empty())
        return false;

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
static bool StrToLongLong(std::string const& str, long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoll(p, end, 0); });
}

template <typename T>
static bool ParseInt(std::string const& str, T& value)
{
    long long v = 0;
    if (StrToLongLong(str, v) && v >= std::numeric_limits<T>::min() && v <= std::numeric_limits<T>::max())
    {
        value = static_cast<T>(v);
        return true;
    }
    return false;
}

bool ConvertValue<signed char>::operator()(std::string const& str, signed char& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed short>::operator()(std::string const& str, signed short& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed int>::operator()(std::string const& str, signed int& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed long>::operator()(std::string const& str, signed long& value) const
{
    return ParseInt(str, value);
}

bool ConvertValue<signed long long>::operator()(std::string const& str, signed long long& value) const
{
    return StrToLongLong(str, value);
}

// (See above)
static bool StrToUnsignedLongLong(std::string const& str, unsigned long long& value)
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtoull(p, end, 0); });
}

template <typename T>
static bool ParseUnsignedInt(std::string const& str, T& value)
{
    unsigned long long v = 0;
    if (StrToUnsignedLongLong(str, v) && v <= std::numeric_limits<T>::max())
    {
        value = static_cast<T>(v);
        return true;
    }
    return false;
}

bool ConvertValue<unsigned char>::operator()(std::string const& str, unsigned char& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned short>::operator()(std::string const& str, unsigned short& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned int>::operator()(std::string const& str, unsigned int& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned long>::operator()(std::string const& str, unsigned long& value) const
{
    return ParseUnsignedInt(str, value);
}

bool ConvertValue<unsigned long long>::operator()(std::string const& str, unsigned long long& value) const
{
    return StrToUnsignedLongLong(str, value);
}

bool ConvertValue<float>::operator()(std::string const& str, float& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtof(p, end); });
}

bool ConvertValue<double>::operator()(std::string const& str, double& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtod(p, end); });
}

bool ConvertValue<long double>::operator()(std::string const& str, long double& value) const
{
    return StrToX(str, value, [](char const* p, char** end) { return std::strtold(p, end); });
}

bool cl::ConvertValue<std::string>::operator()(std::string const& str, std::string& value) const
{
    value.assign(str.data(), str.size());
    return true;
}
