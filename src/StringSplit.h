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

#pragma once

#include "cxx_string_view.h"

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

#define STR_SPLIT_STD 1 // https://isocpp.org/files/papers/n3593.html

namespace str {

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

// Return type for tokenizers. Returned by the ToX delimiters
//
// +-----+-----+------------+
// ^     ^ tok ^    rest    ^
//       f     f+c
//
// Either FIRST or COUNT must be non-zero.
struct TokenizerResult
{
    size_t first;
    size_t count;
};

//
// N3593:
//
// A string delimiter. This is the default delimiter used if a string is given
// as the delimiter argument to std::split().
//
struct ByString
{
    cxx::string_view const delim;

    explicit ByString(char const* delim_) : delim(delim_) {}

    DelimiterResult operator()(cxx::string_view str) const
    {
        if (delim.empty())
        {
#if STR_SPLIT_STD
            if (str.size() <= 1)
                return {cxx::string_view::npos, 0};

            return {1, 0};
#else
            // Return the whole string as a token.
            // Makes StringLiteral("") behave exactly as AnyOf("").
            return {cxx::string_view::npos, 0};
#endif
        }

        return {str.find(delim), delim.size()};
    }
};

//
// A character delimiter.
//
// This is slightly faster than StringLiteral if you want to break the string at
// a specific character.
//
struct ByChar
{
    char const ch;

    explicit ByChar(char ch_) : ch(ch_) {}

    DelimiterResult operator()(cxx::string_view const& str) const {
        return {str.find(ch), 1};
    }
};

//
// N3593:
//
// Each character in the given string is a delimiter. A std::any_of_delimiter
// with string of length 1 behaves the same as a std::literal_delimiter with the
// same string of length 1.
//
struct ByAnyOf
{
    cxx::string_view const chars;

    explicit ByAnyOf(char const* chars_) : chars(chars_) {}

    DelimiterResult operator()(cxx::string_view str) const
    {
#if STR_SPLIT_STD
        if (chars.empty())
        {
            if (str.size() <= 1)
                return {cxx::string_view::npos, 0};
            return {1, 0};
        }
#endif

        return {str.find_first_of(chars), 1};
    }
};

//
// Breaks a stringn into lines, i.e. searches for "\n" or "\r" or "\r\n".
//
struct ByLine
{
    DelimiterResult operator()(cxx::string_view str) const;
};

//
// Breaks a string into words, i.e. searches for the first whitespace preceding
// the given length. If there is no whitespace, breaks a single word at length
// characters.
//
struct ByMaxLength
{
    size_t const length;

    explicit ByMaxLength(size_t length_) : length(length_)
    {
        assert(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(cxx::string_view str) const;
};

namespace impl {

struct DoSplitResult
{
    cxx::string_view tok; // The current token.
    cxx::string_view str; // The rest of the string.
    bool             last = false;
};

inline bool DoSplit(DoSplitResult& res, cxx::string_view str, DelimiterResult del)
{
    //
    // +-----+-----+------------+
    // ^ tok ^     ^    rest    ^
    //       f     f+c
    //

    if (del.first == cxx::string_view::npos)
    {
        res.tok = str;
        //res.str = {};
        res.last = true;
        return true;
    }

    assert(del.first + del.count >= del.first);
    assert(del.first + del.count <= str.size());

    size_t const off = del.first + del.count;
    assert(off > 0 && "invalid delimiter result");

    res.tok = cxx::string_view(str.data(), del.first);
    res.str = cxx::string_view(str.data() + off, str.size() - off);
    return true;
}

inline bool DoSplit(DoSplitResult& res, cxx::string_view str, TokenizerResult tok)
{
    //
    // +-----+-----+------------+
    // ^     ^ tok ^    rest    ^
    //       f     f+c
    //

    if (tok.first == cxx::string_view::npos)
    {
        //res.tok = {};
        //res.str = {};
        res.last = true;
        return false;
    }

    assert(tok.first + tok.count >= tok.first);
    assert(tok.first + tok.count <= str.size());

    size_t const off = tok.first + tok.count;
    assert(off > 0 && "invalid tokenizer result");

    res.tok = cxx::string_view(str.data() + tok.first, tok.count);
    res.str = cxx::string_view(str.data() + off, str.size() - off);
    return true;
}

template <typename Function>
bool ApplyFunction(cxx::string_view str, Function&& fn, /* returns bool */ std::true_type)
{
    return fn(str);
}

template <typename Function>
bool ApplyFunction(cxx::string_view str, Function&& fn, /* returns bool */ std::false_type)
{
    fn(str);
    return true;
}

template <typename Splitter, typename Function>
bool ForEach(cxx::string_view str, Splitter&& split, Function&& fn)
{
    using FnReturnsBool = std::is_convertible<decltype(fn(cxx::string_view{})), bool>;

    DoSplitResult curr;

    curr.tok = cxx::string_view();
    curr.str = str;
    curr.last = false;

    for (;;)
    {
        if (!::str::impl::DoSplit(curr, curr.str, split(curr.str)))
            return true;
        if (!::str::impl::ApplyFunction(curr.tok, /*no forward here!*/ fn, FnReturnsBool{}))
            return false;
        if (curr.last)
            return true;
    }
}

//
// N3593:
//
// The default delimiter when not explicitly specified is
// std::literal_delimiter.
//
template <typename T, typename U = std::decay_t<T>>
using DefaultDelimiter =
    std::conditional_t<
        std::is_constructible<ByChar, T>::value,
        ByChar,
        std::conditional_t<
            std::is_constructible<ByString, T>::value,
            ByString,
            U
        >
    >;

} // namespace impl

//
// Split the string STR into substrings using the Delimiter (or Tokenizer) SPLIT
// and call FN for each substring.
//
// FN must return void or bool. If FN returns false, this method stops splitting
// the input string and returns false, too. Otherwise, returns true.
//
template <typename Splitter, typename Function>
bool Split(cxx::string_view str, Splitter&& split, Function&& fn)
{
    using S = impl::DefaultDelimiter<Splitter, Splitter&&>;

#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
    static_assert(
        std::is_invocable_r<DelimiterResult, S, cxx::string_view>::value ||
        std::is_invocable_r<TokenizerResult, S, cxx::string_view>::value,
        "The delimiter must be invocable with an argument of type 'cxx::string_view' and "
        "its return type must be convertible to either 'DelimiterResult' or 'TokenizerResult'");

    static_assert(
        std::is_invocable_r<bool, Function, cxx::string_view>::value ||
        std::is_invocable_r<void, Function, cxx::string_view>::value,
        "The callback function must be invocable with an argument of type 'cxx::string_view' and "
        "must return either a bool or void");
#endif

    return ::str::impl::ForEach<S>(
        str,
        static_cast<S>(split), // forward or construct
        std::forward<Function>(fn));
}

} // namespace str
