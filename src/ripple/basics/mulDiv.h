//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2015 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_BASICS_MULDIV_H_INCLUDED
#define RIPPLE_BASICS_MULDIV_H_INCLUDED

#include <cstdint>

namespace ripple
{

/**
    A utility function to compute (value)*(mul)/(div) while avoiding
    overflow but keeping precision.
*/
std::uint64_t
mulDiv(std::uint64_t value, std::uint64_t mul, std::uint64_t div);

/**
    A utility function to compute (value)*(mul)/(div) while avoiding
    overflow but keeping precision. Will return the max uint64_t
    value if mulDiv would overflow anyway.
*/
std::uint64_t
mulDivNoThrow(std::uint64_t value, std::uint64_t mul, std::uint64_t div);

template <class T1, class T2>
void lowestTerms(T1& a,  T2& b)
{
    std::uint64_t x = a, y = b;
    while (y != 0)
    {
        auto t = x % y;
        x = y;
        y = t;
    }
    a /= x;
    b /= x;
}

} // ripple

#endif
