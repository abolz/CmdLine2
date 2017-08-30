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

#include "StringSplit.h"

#include <climits>
#include <cstdarg>
#include <cstdio>
#if defined(_WIN32)
#include <windows.h>
#endif

using namespace cl;

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
            EmitDiag(Diagnostic::error, -1, "Option '" + std::string(opt->name_) + "' is missing");
            res = false;
        }
    }

    return res;
}

#if defined(_WIN32)

void Cmdline::PrintErrors() const
{
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

    fprintf(stderr, "\n");
}

#else

#define CL_VT100_RESET      "\x1B[0m"
#define CL_VT100_RED        "\x1B[31;1m"
#define CL_VT100_MAGENTA    "\x1B[36;1m"
#define CL_VT100_CYAN       "\x1B[36;1m"

void Cmdline::PrintErrors() const
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

    fprintf(stderr, "\n");
}

#undef CL_VT100_RESET
#undef CL_VT100_RED
#undef CL_VT100_MAGENTA
#undef CL_VT100_CYAN

#endif

static void AppendAligned(std::string& out, cxx::string_view text, size_t indent, size_t width)
{
    if (text.size() < width && indent < width - text.size())
    {
        out.append(indent, ' ');
        out.append(text.data(), text.size());
        out.append(width - text.size() - indent, ' ');
    }
    else
    {
        out.append(indent, ' ');
        out.append(text.data(), text.size());
        out += '\n';
        out.append(width, ' ');
    }
}

static void AppendWrapped(std::string& out, cxx::string_view text, size_t indent, size_t width)
{
    if (width <= indent)
        width = SIZE_MAX;

    bool first = true;

    // Break the string into paragraphs
    str::Split(text, str::LineDelimiter(), [&](cxx::string_view par)
    {
        // Break the paragraphs at the maximum width into lines
        str::Split(par, str::WrapDelimiter(width), [&](cxx::string_view line)
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

            out.append(line.data(), line.size());
        });
    });
}

static constexpr size_t kMaxWidth    = 0;
static constexpr size_t kOptIndent   = 2;
static constexpr size_t kDescrIndent = 30;

std::string Cmdline::HelpMessage(std::string const& program_name) const
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
            std::string usage;

            usage += '-';
            usage += '-';
            usage.append(opt->name_.data(), opt->name_.size());

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
                sopt.append(usage.data(), usage.size());
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

void Cmdline::PrintHelpMessage(std::string const& program_name) const
{
    auto const msg = HelpMessage(program_name);
    fprintf(stderr, "%s\n", msg.c_str());
}

OptionBase* Cmdline::FindOption(cxx::string_view name) const
{
    for (auto&& p : options_)
    {
        if (p.name == name)
            return p.option;
    }

    return nullptr;
}

OptionBase* Cmdline::FindOption(cxx::string_view name, bool& ambiguous) const
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

void Cmdline::DoAdd(OptionBase* opt)
{
    assert(!opt->name_.empty());

    str::Split(opt->name_, '|', [&](cxx::string_view name)
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

Cmdline::Result Cmdline::HandlePositional(cxx::string_view optstr)
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
Cmdline::Result Cmdline::HandleOption(cxx::string_view optstr)
{
    auto arg_start = optstr.find('=');

    if (arg_start != cxx::string_view::npos)
    {
        // Found an '=' sign. Extract the name of the option.
        auto const name = optstr.substr(0, arg_start);

        bool ambiguous = false;
        if (auto const opt = FindOption(name, ambiguous))
        {
            if (ambiguous)
            {
                EmitDiag(Diagnostic::error, curr_index_, "Option '" + std::string(optstr) + "' is ambiguous");
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

Cmdline::Result Cmdline::HandlePrefix(cxx::string_view optstr)
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
Cmdline::Result Cmdline::DecomposeGroup(cxx::string_view optstr, std::vector<OptionBase*>& group)
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

Cmdline::Result Cmdline::HandleOccurrence(OptionBase* opt, cxx::string_view name, cxx::string_view arg)
{
    // An argument was specified for OPT.

    if (opt->positional_ == Positional::no && opt->num_args_ == Arg::no)
    {
        EmitDiag(Diagnostic::error, curr_index_, "Option '" + std::string(name) + "' does not accept an argument");
        return Result::error;
    }

    return ParseOptionArgument(opt, name, arg);
}

Cmdline::Result Cmdline::ParseOptionArgument(OptionBase* opt, cxx::string_view name, cxx::string_view arg)
{
    auto Parse1 = [&](cxx::string_view arg1)
    {
        if (!opt->IsOccurrenceAllowed())
        {
            EmitDiag(Diagnostic::error, curr_index_, "Option '" + std::string(name) + "' already specified");
            return Result::error;
        }

        ParseContext ctx;

        ctx.name    = name;
        ctx.arg     = arg1;
        ctx.index   = curr_index_;
        ctx.cmdline = this;

        //size_t const num_diagnostics = diag_.size();

        if (!opt->Parse(ctx))
        {
            //bool const diagnostic_emitted = diag_.size() > num_diagnostics;
            //if (!diagnostic_emitted)
            {
                EmitDiag(Diagnostic::error, curr_index_, "Invalid argument '" + std::string(arg1) + "' for option '" + std::string(name) + "'");
            }

            return Result::error;
        }

        ++opt->count_;
        return Result::success;
    };

    Result res = Result::success;

    if (opt->comma_separated_arg_ == CommaSeparatedArg::yes)
    {
        str::Split(arg, ',', [&](cxx::string_view s)
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

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

template <typename T, typename ...Match>
static bool IsAnyOf(T&& value, Match&&... match)
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

bool cl::ParseValue<bool>::operator()(ParseContext const& ctx, bool& value) const
{
    if (ctx.arg.empty() || IsAnyOf(ctx.arg, "1", "true", "True", "yes", "Yes", "on", "On"))
        value = true;
    else if (IsAnyOf(ctx.arg, "0", "false", "False", "no", "No", "off", "Off"))
        value = false;
    else
        return false;

    return true;
}

bool cl::ParseValue<std::string>::operator()(ParseContext const& ctx, std::string& value) const
{
    value.assign(ctx.arg.data(), ctx.arg.size());
    return true;
}
