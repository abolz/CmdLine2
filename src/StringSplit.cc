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

#define STR_STRING_SPLIT_USE_SSE 0

#include <cassert>

#if STR_STRING_SPLIT_USE_SSE
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <emmintrin.h>
#endif

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

static bool IsLineBreak(char ch)
{
    return ch == '\n' || ch == '\r';
}

#if STR_STRING_SPLIT_USE_SSE
static int CountTrailingZeros(unsigned x) // PRE: x != 0
{
#ifdef _MSC_VER
    unsigned long index = 0;
    _BitScanForward(&index, x);
    return static_cast<int>(index);
#else
    return __builtin_ctz(x);
#endif
}
#endif

// Returns the position of the first character in [FIRST, LAST) which is equal to CR or LF.
static char const* FindLineBreak(char const* first, char const* last)
{
    assert(first <= last);

#if STR_STRING_SPLIT_USE_SSE
    if (last - first >= 16)
    {
        __m128i const CR = _mm_set1_epi8('\r');
        __m128i const LF = _mm_set1_epi8('\n');

        uintptr_t blocks = static_cast<uintptr_t>((last - first) / 16);
        while (blocks-- != 0)
        {
            __m128i const data = _mm_loadu_si128(reinterpret_cast<__m128i const*>(first));
            __m128i const mask = _mm_or_si128(_mm_cmpeq_epi8(data, CR),
                                              _mm_cmpeq_epi8(data, LF));

            unsigned const m = static_cast<unsigned>(_mm_movemask_epi8(mask));
            if (m != 0)
                return first + CountTrailingZeros(m);

            first += 16;
        }
    }
#endif

    while (first != last && !IsLineBreak(*first))
    {
        ++first;
    }

    return first;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

str::DelimiterResult str::LineDelimiter::operator()(cxx::string_view str) const
{
    auto const first = str.data();
    auto const last  = str.data() + str.size();

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
            return {index, 2};
    }

    return {index, 1};
}

str::DelimiterResult str::WrapDelimiter::operator()(cxx::string_view str) const
{
    // If the string fits into the current line, just return this last line.
    if (str.size() <= length)
    {
        return {cxx::string_view::npos, 0};
    }

    // Otherwise, search for the first space preceding the line length.
    auto I = str.find_last_of(" \t", length);

    if (I != cxx::string_view::npos) // There is a space.
    {
        return {I, 1};
    }

    return {length, 0}; // No space in current line, break at length.
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

bool str::impl::DoSplit(DoSplitResult& res, cxx::string_view str, DelimiterResult del)
{
    //
    // +-----+-----+------------+
    // ^ tok ^     ^    rest    ^
    //       f     f+c
    //

    if (del.first == cxx::string_view::npos)
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

    res.tok = { str.data(), del.first };
    res.str = { str.data() + off, str.size() - off };
    return true;
}

bool str::impl::DoSplit(DoSplitResult& res, cxx::string_view str, TokenizerResult tok)
{
    //
    // +-----+-----+------------+
    // ^     ^ tok ^    rest    ^
    //       f     f+c
    //

    if (tok.first == cxx::string_view::npos)
    {
//      res.tok = {};
//      res.str = {};
        res.last = true;
        return false;
    }

    assert(tok.first + tok.count >= tok.first);
    assert(tok.first + tok.count <= str.size());

    size_t const off = tok.first + tok.count;
    assert(off > 0 && "invalid tokenizer result");

    res.tok = { str.data() + tok.first, tok.count };
    res.str = { str.data() + off, str.size() - off };
    return true;
}
