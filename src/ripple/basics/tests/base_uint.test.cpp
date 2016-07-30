//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2016 Ripple Labs Inc.

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
#include <ripple/basics/base_uint.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/digest.h>

namespace ripple {
namespace test {

struct base_uint_test : beast::unit_test::suite
{
    using test96 = base_uint<96>;

    template<class bt>
    void checkHash(bt const& t, std::string const& hash)
    {
        auto h = sha512Half(t);
        BEAST_EXPECT(to_string(h) == hash);
    }

    void run()
    {
        Blob raw{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        BEAST_EXPECT(test96::bytes == raw.size());

        test96 u(raw);
        BEAST_EXPECT(raw.size() == u.size());
        BEAST_EXPECT(to_string(u) == "0102030405060708090A0B0C");
        BEAST_EXPECT(*u.data() == 1);
        BEAST_EXPECT(u.signum() == 1);
        BEAST_EXPECT(!!u);
        BEAST_EXPECT(!u.isZero());
        BEAST_EXPECT(u.isNonZero());
        unsigned char t = 0;
        for (auto& d : u)
        {
            BEAST_EXPECT(d == ++t);
        }
        checkHash(u, "25996DBE31A04FF03E4E63C9C647AC6D459475E2845529238F3D08F4A37EBC54");

        test96 v(~u);
        BEAST_EXPECT(to_string(v) == "FEFDFCFBFAF9F8F7F6F5F4F3");
        BEAST_EXPECT(*v.data() == 0xfe);
        BEAST_EXPECT(v.signum() == 1);
        BEAST_EXPECT(!!v);
        BEAST_EXPECT(!v.isZero());
        BEAST_EXPECT(v.isNonZero());
        t = 0xff;
        for (auto& d : v)
        {
            BEAST_EXPECT(d == --t);
        }
        checkHash(v, "019A8C0B8307FD822A1346546200328DB40B4565793FD4799B83D780BDB634CE");

        BEAST_EXPECT(compare(u, v) < 0);
        BEAST_EXPECT(compare(v, u) > 0);

        v = u;
        BEAST_EXPECT(v == u);

        test96 z(beast::zero);
        BEAST_EXPECT(to_string(z) == "000000000000000000000000");
        BEAST_EXPECT(*z.data() == 0);
        BEAST_EXPECT(*z.begin() == 0);
        BEAST_EXPECT(*std::prev(z.end(), 1) == 0);
        BEAST_EXPECT(z.signum() == 0);
        BEAST_EXPECT(!z);
        BEAST_EXPECT(z.isZero());
        BEAST_EXPECT(!z.isNonZero());
        for (auto& d : z)
        {
            BEAST_EXPECT(d == 0);
        }
        checkHash(z, "666A9A1A7542E895A9F447D1C3E0FFD679BBF6346E0C43F5C7A733C46F5E56C2");

        test96 n(z);
        n++;
        BEAST_EXPECT(n == test96(1));
        n--;
        BEAST_EXPECT(n == beast::zero);
        BEAST_EXPECT(n == z);
        n--;
        BEAST_EXPECT(to_string(n) == "FFFFFFFFFFFFFFFFFFFFFFFF");
        n = beast::zero;
        BEAST_EXPECT(n == z);

        auto hash = sha512Half(u, v, z, n);
        BEAST_EXPECT(to_string(hash) ==
            "55CAB39C72F2572057114B4732210605AA22C6E89243FBB5F9943130D813F902");
    }
};

BEAST_DEFINE_TESTSUITE(base_uint, ripple_basics, ripple);

}  // namespace test
}  // namespace ripple
