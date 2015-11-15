//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#include <BeastConfig.h>
#include <ripple/protocol/XRPAmount.h>
#include <beast/unit_test/suite.h>

namespace ripple {

class XRPAmount_test : public beast::unit_test::suite
{
public:
    void testSigNum ()
    {
        testcase ("signum");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x(i);

            if (i < 0)
                expect (x.signum () < 0);
            else if (i > 0)
                expect (x.signum () > 0);
            else
                expect (x.signum () == 0);
        }
    }

    void testBeastZero ()
    {
        testcase ("beast::Zero Comparisons");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            expect ((i == 0) == (x == zero));
            expect ((i != 0) == (x != zero));
            expect ((i < 0) == (x < zero));
            expect ((i > 0) == (x > zero));
            expect ((i <= 0) == (x <= zero));
            expect ((i >= 0) == (x >= zero));

            expect ((0 == i) == (zero == x));
            expect ((0 != i) == (zero != x));
            expect ((0 < i) == (zero < x));
            expect ((0 > i) == (zero > x));
            expect ((0 <= i) == (zero <= x));
            expect ((0 >= i) == (zero >= x));
        }
    }

    void testComparisons ()
    {
        testcase ("XRP Comparisons");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                expect ((i == j) == (x == y));
                expect ((i != j) == (x != y));
                expect ((i < j) == (x < y));
                expect ((i > j) == (x > y));
                expect ((i <= j) == (x <= y));
                expect ((i >= j) == (x >= y));
            }
        }
    }

    void testAddSub ()
    {
        testcase ("Addition & Subtraction");

        for (auto i : { -1, 0, 1})
        {
            XRPAmount const x (i);

            for (auto j : { -1, 0, 1})
            {
                XRPAmount const y (j);

                expect (XRPAmount(i + j) == (x + y));
                expect (XRPAmount(i - j) == (x - y));

                expect ((x + y) == (y + x));   // addition is commutative
            }
        }
    }

    void testMulRatio()
    {
        testcase ("mulRatio");

        constexpr auto maxUInt32 = std::numeric_limits<std::uint32_t>::max ();
        constexpr auto maxUInt64 = std::numeric_limits<std::uint64_t>::max ();

        {
            // multiply by a number that would overflow then divide by the same
            // number, and check we didn't lose any value
            XRPAmount big (maxUInt64);
            expect (big == mulRatio (big, maxUInt32, maxUInt32, true));
            // rounding mode shouldn't matter as the result is exact
            expect (big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
            // Similar test as above, but for neative values
            XRPAmount big (maxUInt64);
            expect (big == mulRatio (big, maxUInt32, maxUInt32, true));
            // rounding mode shouldn't matter as the result is exact
            expect (big == mulRatio (big, maxUInt32, maxUInt32, false));
        }

        {
            // small amounts
            XRPAmount tiny (1);
            // Round up should give the smallest allowable number
            expect (tiny == mulRatio (tiny, 1, maxUInt32, true));
            // rounding down should be zero
            expect (beast::zero == mulRatio (tiny, 1, maxUInt32, false));
            expect (beast::zero ==
                mulRatio (tiny, maxUInt32 - 1, maxUInt32, false));

            // tiny negative numbers
            XRPAmount tinyNeg (-1);
            // Round up should give zero
            expect (zero == mulRatio (tinyNeg, 1, maxUInt32, true));
            expect (zero == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, true));
            // rounding down should be tiny
            expect (tinyNeg == mulRatio (tinyNeg, maxUInt32 - 1, maxUInt32, false));
        }

        {
            // rounding
            {
                XRPAmount one (1);
                auto const rup = mulRatio (one, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (one, maxUInt32 - 1, maxUInt32, false);
                expect (rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount big (maxUInt64);
                auto const rup = mulRatio (big, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (big, maxUInt32 - 1, maxUInt32, false);
                expect (rup.drops () - rdown.drops () == 1);
            }

            {
                XRPAmount negOne (-1);
                auto const rup = mulRatio (negOne, maxUInt32 - 1, maxUInt32, true);
                auto const rdown = mulRatio (negOne, maxUInt32 - 1, maxUInt32, false);
                expect (rup.drops () - rdown.drops () == 1);
            }
        }

        {
            // division by zero
            XRPAmount one (1);
            except ([&] {mulRatio (one, 1, 0, true);});
        }

        {
            // overflow
            XRPAmount big (maxUInt64);
            except ([&] {mulRatio (big, 2, 0, true);});
        }
    }

    //--------------------------------------------------------------------------

    void run ()
    {
        testSigNum ();
        testBeastZero ();
        testComparisons ();
        testAddSub ();
        testMulRatio ();
    }
};

BEAST_DEFINE_TESTSUITE(XRPAmount,protocol,ripple);

} // ripple
