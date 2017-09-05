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

#ifndef STR_STRING_SPLIT_H
#define STR_STRING_SPLIT_H 1

#include "cxx_string_view.h"

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

#define STR_STRING_SPLIT_STD 1 // https://isocpp.org/files/papers/n3593.html

namespace str {

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

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

// Return type for tokenizers.
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

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

//
// N3593:
//
// A string delimiter. This is the default delimiter used if a string is given
// as the delimiter argument to std::split().
//
struct StringLiteral
{
    cxx::string_view const delim;

    explicit StringLiteral(char const* delim_) : delim(delim_) {}

    DelimiterResult operator()(cxx::string_view str) const
    {
        if (delim.empty())
        {
#if STR_STRING_SPLIT_STD
            if (str.size() <= 1)
                return { cxx::string_view::npos, 0 };
            return { 1, 0 };
#else
            // Return the whole string as a token.
            // Makes StringLiteral("") behave exactly as AnyOf("").
            return { cxx::string_view::npos, 0 };
#endif
        }

        return { str.find(delim), delim.size() };
    }
};

//
// A character delimiter.
//
// This is slightly faster than StringLiteral if you want to break the string at
// a specific character.
//
struct CharLiteral
{
    char const ch;

    explicit CharLiteral(char ch_) : ch(ch_) {}

    DelimiterResult operator()(cxx::string_view const& str) const
    {
        return { str.find(ch), 1 };
    }
};

//
// N3593:
//
// Each character in the given string is a delimiter. A std::any_of_delimiter
// with string of length 1 behaves the same as a std::literal_delimiter with the
// same string of length 1.
//
struct AnyOf
{
    cxx::string_view const chars;

    explicit AnyOf(char const* chars_) : chars(chars_) {}

    DelimiterResult operator()(cxx::string_view str) const
    {
#if STR_STRING_SPLIT_STD
        if (chars.empty())
        {
            if (str.size() <= 1)
                return { cxx::string_view::npos, 0 };
            return { 1, 0 };
        }
#endif

        return { str.find_first_of(chars), 1 };
    }
};

//
// Breaks a stringn into lines, i.e. searches for "\n" or "\r" or "\r\n".
//
struct LineDelimiter
{
    DelimiterResult operator()(cxx::string_view str) const;
};

//
// Breaks a string into words, i.e. searches for the first whitespace preceding
// the given length. If there is no whitespace, breaks a single word at length
// characters.
//
struct WrapDelimiter
{
    size_t const length;

    explicit WrapDelimiter(size_t length_) : length(length_)
    {
        assert(length != 0 && "invalid parameter");
    }

    DelimiterResult operator()(cxx::string_view str) const;
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

namespace impl {

#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17) // yes, this is wrong...

template <typename R, typename T, typename ...Args>
using IsInvocableR = typename std::is_invocable_r<R, T, Args...>::type;

#else // WORKAROUND (incomplete)

template <typename T>
using Void_t = void;

template <typename T, typename Sig, typename = void>
struct IsInvocableImpl : std::false_type
{
};

template <typename T, typename Ret, typename ...Args>
struct IsInvocableImpl<T, Ret (Args...), Void_t< std::result_of<T&& (Args&&...)> >>
    : std::is_convertible< std::result_of_t<T&& (Args&&...)>, Ret >
{
};

template <typename R, typename T, typename ...Args>
using IsInvocableR = typename IsInvocableImpl<T, R(Args...)>::type;

#endif

struct DoSplitResult
{
    cxx::string_view tok; // The current token.
    cxx::string_view str; // The rest of the string.
    bool last = false;
};

bool DoSplit(DoSplitResult& res, cxx::string_view str, DelimiterResult del);
bool DoSplit(DoSplitResult& res, cxx::string_view str, TokenizerResult tok);

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

template <typename String, typename Splitter, typename Function>
bool ForEach(String&& str, Splitter&& split, Function&& fn)
{
    DoSplitResult curr;

    curr.tok = {};
    curr.str = cxx::string_view(str);
    curr.last = false;

    for (;;)
    {
        if (!::str::impl::DoSplit(curr, curr.str, split(curr.str)))
            return true;
        if (!::str::impl::ApplyFunction(curr.tok, /*no forward here!*/ fn, IsInvocableR<bool, Function, cxx::string_view>{}))
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
        std::is_constructible<CharLiteral, T>::value,
        CharLiteral,
        std::conditional_t<
            std::is_constructible<StringLiteral, T>::value,
            StringLiteral,
            U
        >
    >;

} // namespace impl

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

//
// Split the string STR into substrings using the Delimiter (or Tokenizer) SPLIT
// and call FN for each substring.
//
// FN must return void or bool. If FN returns false, this method stops splitting
// the input string and returns false, too. Otherwise, returns true.
//
template <typename String, typename Splitter, typename Function>
inline bool Split(String&& str, Splitter&& split, Function&& fn)
{
    using S = impl::DefaultDelimiter<Splitter, Splitter&&>;

    static_assert(
        std::is_constructible<cxx::string_view, String&&>::value,
        "The string type must be explicitly convertible to 'cxx::string_view'");

    static_assert(
        impl::IsInvocableR< DelimiterResult, S, cxx::string_view >::value ||
        impl::IsInvocableR< TokenizerResult, S, cxx::string_view >::value,
        "The delimiter must be invocable with an argument of type 'cxx::string_view' and "
        "its return type must be convertible to either 'DelimiterResult' or 'TokenizerResult'");

    static_assert(
        impl::IsInvocableR< bool, Function, cxx::string_view >::value ||
        impl::IsInvocableR< void, Function, cxx::string_view >::value,
        "The callback function must be invocable with an argument of type 'cxx::string_view' and "
        "must return either a bool or void");

    return ::str::impl::ForEach<String, S>(
        std::forward<String>(str),
            static_cast<S>(split), // forward or construct
                std::forward<Function>(fn));
}

} // namespace str

#endif // STR_STRING_SPLIT_H
