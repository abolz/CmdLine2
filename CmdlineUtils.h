// Distributed under the MIT license. See the end of the file for details.

#pragma once

#include <iterator>

namespace cl {

// Parses a single argument from a command line string.
// Using Unix-style escaping.
template <typename InpIt, typename Buffer>
InpIt TokenizeSingleArgUnix(InpIt first, InpIt last, Buffer& buffer)
{
    //
    // See:
    //
    // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
    // http://wiki.bash-hackers.org/syntax/quoting
    //

    int quote_char = 0;

    for (/**/; first != last; ++first)
    {
        auto ch = *first;

        // Quoting a single character using the backslash?
        if (quote_char == '\\')
        {
            buffer.push_back(ch);
            quote_char = 0;
        }
        // Currently quoting using ' or "?
        else if (quote_char && ch != quote_char)
        {
            buffer.push_back(ch);
        }
        // Toggle quoting?
        else if (ch == '\'' || ch == '"' || ch == '\\')
        {
            quote_char = quote_char ? 0 : ch;
        }
        // Arguments are separated by whitespace
        else if (ch == ' ' || ch == '\t')
        {
            return ++first;
        }
        else
        {
            buffer.push_back(ch);
        }
    }

    return first;
}

// Parses a command line string and returns a list of command line arguments.
// Using Unix-style escaping.
template <typename InpIt, typename Rhs>
void TokenizeUnix(InpIt first, InpIt last, Rhs& out)
{
    if (first == last)
        return;

    typename Rhs::value_type buffer;

    while (first != last)
    {
        buffer.clear();

        first = TokenizeSingleArgUnix(first, last, buffer);

        if (!buffer.empty())
            out.push_back(buffer);
    }
}

// Parse off the program filename.
// For Windows.
template <typename InpIt, typename Buffer>
InpIt ParseProgramFilenameWindows(InpIt first, InpIt last, Buffer& buffer)
{
    if (first == last)
        return first;

    bool quoting = (*first == '"');

    if (quoting)
        ++first;

    for (/**/; first != last; ++first)
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

        buffer.push_back(ch);
    }

    return first;
}

// Parses a single argument from a command line string.
// Using Windows-style escaping.
template <typename InpIt, typename Buffer>
InpIt TokenizeSingleArgWindows(InpIt first, InpIt last, Buffer& buffer, int& quoting)
{
    int recently_closed = 0;
    int num_backslashes = 0;

    quoting = 0;

    for (/**/; first != last; ++first)
    {
        auto ch = *first;

        if (ch == '"' && recently_closed)
        {
            recently_closed = 0;

            // If a closing " is followed immediately by another ", the 2nd " is
            // accepted literally and added to the parameter.
            //
            // See:
            // http://www.daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC

            buffer.push_back('"');

#if 0 // post 2008
            quoting = 1;
#endif
        }
        else if (ch == '"')
        {
            // If an even number of backslashes is followed by a double
            // quotation mark, one backslash is placed in the argv array for
            // every pair of backslashes, and the double quotation mark is
            // interpreted as a string delimiter.
            //
            // If an odd number of backslashes is followed by a double quotation
            // mark, one backslash is placed in the argv array for every pair of
            // backslashes, and the double quotation mark is "escaped" by the
            // remaining backslash, causing a literal double quotation mark (")
            // to be placed in argv.

            int even = (num_backslashes % 2) == 0;

            num_backslashes /= 2;
            for (/**/; num_backslashes != 0; --num_backslashes)
                buffer.push_back('\\');

            if (even)
            {
                recently_closed = quoting; // Remember if this is a closing "
                quoting = !quoting;
            }
            else
            {
                buffer.push_back('"');
            }
        }
        else if (ch == '\\')
        {
            recently_closed = 0;

            ++num_backslashes;
        }
        else
        {
            recently_closed = 0;

            // Backslashes are interpreted literally, unless they immediately
            // precede a double quotation mark.

            for (/**/; num_backslashes != 0; --num_backslashes)
                buffer.push_back('\\');

            if ((ch == ' ' || ch == '\t') && !quoting)
            {
                // Arguments are delimited by white space, which is either a
                // space or a tab.
                //
                // A string surrounded by double quotation marks ("string") is
                // interpreted as single argument, regardless of white space
                // contained within. A quoted string can be embedded in an
                // argument.

                ++first;
                break;
            }

            buffer.push_back(ch);
        }
    }

    return first;
}

// Parses a command line string and returns a list of command line arguments.
// Using Windows-style escaping.
template <class InpIt, class Rhs>
void TokenizeWindows(InpIt first, InpIt last, Rhs& out, bool parse_program_name = true)
{
    if (first == last)
        return;

    typename Rhs::value_type buffer;

    if (parse_program_name)
    {
        first = ParseProgramFilenameWindows(first, last, buffer);
        out.push_back(buffer);
    }

    while (first != last)
    {
        int quoting = 0;

        first = TokenizeSingleArgWindows(first, last, buffer, quoting);

        if (!buffer.empty() || quoting)
        {
            out.push_back(buffer);
            buffer.clear();
        }
    }
}

// Quote a single command line argument. Using Windows-style escaping.
//
// See:
// http://blogs.msdn.com/b/twistylittlepassagesallalike/archive/2011/04/23/everyone-quotes-arguments-the-wrong-way.aspx
//
// This routine appends the given argument to a command line such that
// CommandLineToArgvW will return the argument string unchanged. Arguments in a
// command line should be separated by spaces; this function does not add these
// spaces.
template <class InpIt, class Buffer>
void QuoteSingleArgWindows(InpIt first, InpIt last, Buffer& buffer)
{
    int num_backslashes = 0;

    buffer.push_back('"');

    for (/**/; first != last; ++first)
    {
        auto ch = *first;

        if (ch == '\\')
        {
            ++num_backslashes;
        }
        else if (ch == '"')
        {
            // Escape all backslashes and the following double quotation mark.

            for (/**/; num_backslashes != 0; --num_backslashes)
                buffer.push_back('\\');

            buffer.push_back('\\');
        }
        else
        {
            num_backslashes = 0; // Backslashes aren't special here.
        }

        buffer.push_back(ch);
    }

    // Escape all backslashes, but let the terminating double quotation mark we
    // add below be interpreted as a metacharacter.

    for (/**/; num_backslashes != 0; --num_backslashes)
        buffer.push_back('\\');

    buffer.push_back('"');
}

// Quote command line arguments. Using Windows-style escaping.
template <class InpIt, class Buffer>
InpIt QuoteArgsWindows(InpIt first, InpIt last, Buffer& buffer)
{
    using std::begin;
    using std::end;

    for (/**/; first != last; ++first)
    {
        auto I = begin(*first);
        auto E = end(*first);

        if (I == E)
        {
            // If a command line string ends while currently quoting,
            // CommandLineToArgvW will add the current argument to the argv
            // array regardless whether the current argument is empty or not.
            //
            // So if we have an empty argument here add an opening " and return.
            // This should be the last argument though...

            buffer.push_back('"');
            break;
        }

        // Append the current argument
        QuoteSingleArgWindows(I, E, buffer);

        buffer.push_back(' ');
    }

    return first;
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
