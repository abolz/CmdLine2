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

#ifndef CL_CMDLINE_UTILS_H
#define CL_CMDLINE_UTILS_H 1

#include <cassert>
#include <iterator>
#include <string>
#include <type_traits>

namespace cl {

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

// Parse arguments from a command line string.
// Using Unix-style escaping.
struct GenArgsUnix
{
    template <typename InpIt, typename EndIt, typename YieldChar>
    InpIt operator()(InpIt next, EndIt last, YieldChar yield_char) const
    {
        //using value_type = typename std::iterator_traits<InpIt>::value_type;
        using value_type = std::remove_const_t<std::remove_reference_t<decltype(*next)>>;

        //
        // See:
        //
        // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
        // http://wiki.bash-hackers.org/syntax/quoting
        //

        value_type quote_char = '\0';

        // Skip leading space
        for ( ; next != last; ++next)
        {
            auto ch = *next;

            if (ch != ' ' && ch != '\t')
                break;
        }

        for ( ; next != last; ++next)
        {
            auto ch = *next;

            //
            // Quoting a single character using the backslash?
            //
            if (quote_char == '\\')
            {
                yield_char(ch);
                quote_char = '\0';
            }
            //
            // Currently quoting using ' or "?
            //
            else if (quote_char != '\0' && ch != quote_char)
            {
                yield_char(ch);
            }
            //
            // Toggle quoting?
            //
            else if (ch == '\'' || ch == '"' || ch == '\\')
            {
                quote_char = (quote_char != '\0') ? '\0' : ch;
            }
            //
            // Arguments are separated by whitespace
            //
            else if (ch == ' ' || ch == '\t')
            {
                ++next;
                break;
            }
            else
            {
                yield_char(ch);
            }
        }

        return next;
    }
};

// Parse off the program filename.
// For Windows.
struct ParseProgramNameWindows
{
    template <typename InpIt, typename EndIt, typename YieldChar>
    InpIt operator()(InpIt first, EndIt last, YieldChar yield_char) const
    {
        if (first == last)
            return first;

        bool quoting = (*first == '"');

        if (quoting)
            ++first;

        for ( ; first != last; ++first)
        {
            auto ch = *first;

            if (ch == '"' && quoting)
            {
                ++first;
                break;
            }

            if ((ch == ' ' || ch == '\t') && !quoting)
            {
                ++first;
                break;
            }

            yield_char(ch);
        }

        return first;
    }
};

// Parses a single argument from a command line string.
// Using Windows-style escaping.
struct GenArgsWindows
{
    template <typename InpIt, typename EndIt, typename YieldChar>
    InpIt operator()(InpIt next, EndIt last, YieldChar yield_char) const
    {
        bool   quoting         = false;
        bool   recently_closed = false;
        size_t num_backslashes = 0;

        // Skip leading space
        for ( ; next != last; ++next)
        {
            auto ch = *next;

            if (ch != ' ' && ch != '\t')
                break;
        }

        for ( ; next != last; ++next)
        {
            auto ch = *next;

            if (ch == '"' && recently_closed)
            {
                recently_closed = false;

                //
                // If a closing " is followed immediately by another ", the 2nd
                // " is accepted literally and added to the parameter.
                //
                // See:
                // http://www.daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC
                //

                yield_char('"');
            }
            else if (ch == '"')
            {
                //
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
                //

                bool even = (num_backslashes % 2) == 0;

                num_backslashes /= 2;
                for (/**/; num_backslashes != 0; --num_backslashes)
                {
                    yield_char('\\');
                }

                if (even)
                {
                    recently_closed = quoting; // Remember if this is a closing "
                    quoting = !quoting;
                }
                else
                {
                    yield_char('"');
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

                //
                // Backslashes are interpreted literally, unless they
                // immediately precede a double quotation mark.
                //

                for (/**/; num_backslashes != 0; --num_backslashes)
                {
                    yield_char('\\');
                }

                if ((ch == ' ' || ch == '\t') && !quoting)
                {
                    //
                    // Arguments are delimited by white space, which is either a
                    // space or a tab.
                    //
                    // A string surrounded by double quotation marks ("string")
                    // is interpreted as single argument, regardless of white
                    // space contained within. A quoted string can be embedded
                    // in an argument.
                    //

                    ++next;
                    break;
                }

                yield_char(ch);
            }
        }

        return next;
    }
};

// Quote a single command line argument.
// Using Windows-style escaping.
//
// See:
// http://blogs.msdn.com/b/twistylittlepassagesallalike/archive/2011/04/23/everyone-quotes-arguments-the-wrong-way.aspx
//
// This routine appends the given argument to a command line such that
// CommandLineToArgvW will return the argument string unchanged. Arguments in a
// command line should be separated by spaces; this function does not add these
// spaces.
struct QuoteArgsWindows
{
    template <typename InpIt, typename EndIt, typename YieldChar>
    void operator()(InpIt first, EndIt last, YieldChar yield_char)
    {
        if (first == last)
            return;

        auto yield_bs = [&](size_t count) // Append COUNT backslashes
        {
            for ( ; count != 0; --count)
            {
                yield_char('\\');
            }
        };

        yield_char('"');

        for ( ; ; ++first)
        {
            size_t num_backslashes = 0;

            for ( ; first != last; ++first)
            {
                auto ch = *first;

                if (ch != '\\')
                    break;

                ++num_backslashes;
            }

            if (first == last)
            {
                //
                // Escape all backslashes, but let the terminating double
                // quotation mark we add below be interpreted as a
                // metacharacter.
                //

                yield_bs(num_backslashes * 2);
                break;
            }
            else if (*first == '"')
            {
                //
                // Escape all backslashes and the following double quotation
                // mark.
                //

                yield_bs(num_backslashes * 2 + 1);
                yield_char('"');
            }
            else
            {
                //
                // Backslashes aren't special here.
                //

                yield_bs(num_backslashes);
                yield_char(*first);
            }
        }

        yield_char('"');
    }
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#if 0

class ArgvEnd {
    //bool operator==(ArgvEnd) const { return true; }
    //bool operator!=(ArgvEnd) const { return false; }
};

inline ArgvEnd argv_end()
{
    return {};
}

template <typename InpIt, typename EndIt = InpIt, typename Generator = cl::GenArgsUnix>
class ArgvIterator
{
    static_assert(
        std::is_convertible<decltype(*std::declval<InpIt>()), char>::value,
        "Incompatible iterator value_type");

    using Buffer = std::string;

    Buffer      arg;
    InpIt       first;
    InpIt       next;
    EndIt const last;

public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = Buffer;
    using reference         = Buffer /*const*/&;
    using pointer           = Buffer /*const*/*;
    using difference_type   = ptrdiff_t;

public:
    ArgvIterator(InpIt f, EndIt l)
        : first(f)
        , next(f)
        , last(l)
    {
    }

    reference operator*()
    {
        assert(first != last);
        if (first == next)
        {
            arg.clear();
            next = Generator{}(first, last, [&](auto ch) { arg.push_back(ch); });
        }

        return arg;
    }

    pointer operator->()
    {
        return &(operator*());
    }

    ArgvIterator& operator++()
    {
        first = next;
        return *this;
    }

    bool operator==(ArgvEnd) const { return first == last; }
    bool operator!=(ArgvEnd) const { return first != last; }
};

template <typename InpIt, typename EndIt>
ArgvIterator<InpIt, EndIt> argv_begin(InpIt first, EndIt last)
{
    return ArgvIterator<InpIt, EndIt>(first, last);
}

template <typename InpIt, typename EndIt, typename Generator>
ArgvIterator<InpIt, EndIt, Generator> argv_begin(InpIt first, EndIt last, Generator)
{
    return ArgvIterator<InpIt, EndIt, Generator>(first, last);
}

#endif

} // namespace cl

#endif // CL_CMDLINE_UTILS_H
