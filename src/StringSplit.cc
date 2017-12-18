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

#include "StringSplit.h"

#include <cassert>

static bool IsLineBreak(char ch)
{
    return ch == '\n' || ch == '\r';
}

// Returns the position of the first character in [FIRST, LAST) which is equal to CR or LF.
static char const* FindLineBreak(char const* first, char const* last)
{
    assert(first <= last);

    for (; first != last && !IsLineBreak(*first); ++first)
    {
    }

    return first;
}

str::DelimiterResult str::ByLine::operator()(cxx::string_view str) const
{
    auto const first = str.data();
    auto const last = str.data() + str.size();

    // Find the position of the first CR or LF
    auto const p = FindLineBreak(first, last);
    if (p == last)
    {
        return {cxx::string_view::npos, 0};
    }

    auto const index = static_cast<size_t>(p - first);

    // If this is CRLF, skip the other half.
    if (p + 1 != last)
    {
        if (p[0] == '\r' && p[1] == '\n')
        {
            return {index, 2};
        }
    }

    return {index, 1};
}

str::DelimiterResult str::ByMaxLength::operator()(cxx::string_view str) const
{
    // If the string fits into the current line, just return this last line.
    if (str.size() <= length)
    {
        return {cxx::string_view::npos, 0};
    }

    // Otherwise, search for the first space preceding the line length.
    auto I = str.find_last_of(" \t", length);

    if (I != cxx::string_view::npos)
    {
        // There is a space.
        return {I, 1};
    }

    return {length, 0}; // No space in current line, break at length.
}