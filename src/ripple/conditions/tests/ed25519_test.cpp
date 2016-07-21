//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2016 Ripple Labs Inc.

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

#include <ripple/basics/strHex.h>
#include <ripple/beast/unit_test.h>
#include <ripple/conditions/condition.h>
#include <ripple/conditions/fulfillment.h>
#include <ripple/conditions/ed25519.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <vector>

namespace ripple {
namespace cryptoconditions {

class ed25519_test : public beast::unit_test::suite
{
    void
    check (
        std::array<std::uint8_t, 32> const& secretKey,
        std::vector<std::uint8_t> const& payload,
        std::string const& fulfillment,
        std::string const& condition)
    {
        SecretKey const sk { makeSlice (secretKey) };
        PublicKey const pk = derivePublicKey (KeyType::ed25519, sk);

        auto f = load_fulfillment (fulfillment);
        auto c = load_condition (condition);

        expect (f);
        expect (c);

        if (f && c)
        {
            // Ensure that loading works correctly
            expect (to_string (*f) == fulfillment);
            expect (to_string (*c) == condition);

            // Ensures that the fulfillment generates
            // the condition correctly:
            expect (f->condition() == c);

            // Check fulfillment
            expect (validate (*f, *c, makeSlice(payload)));
            expect (validate (*f, makeSlice(payload)));

            // Check correct creation of fulfillment
            expect (*f == ed25519_t (sk, pk, makeSlice(payload)));
        }
    }

    void testKnownVectors ()
    {
        testcase ("Known Vectors");

        std::array<std::uint8_t, 32> sk =
        {{
            0x50, 0xd8, 0x58, 0xe0, 0x98, 0x5e, 0xcc, 0x7f,
            0x60, 0x41, 0x8a, 0xaf, 0x0c, 0xc5, 0xab, 0x58,
            0x7f, 0x42, 0xc2, 0x57, 0x0a, 0x88, 0x40, 0x95,
            0xa9, 0xe8, 0xcc, 0xac, 0xd0, 0xf6, 0x54, 0x5c
        }};

        std::vector<std::uint8_t> const payload (512, 0x21);

        check (sk, payload,
            "cf:4:RCmTBlAEqh5MSPTdAVgZTAI0m8xmTNluQA6iaZGKjVGfTbzglso5Uo3i2O2WVP6abH1dz5k0H5DLylizTeL5UC0VSptUN4VCkhtbwx3B00pCeWNy1H78rq6OTXzok-EH",
            "cc:4:20:RCmTBlAEqh5MSPTdAVgZTAI0m8xmTNluQA6iaZGKjVE:96");

        sk.fill (0x00);
        check (sk, hexblob (""),
            "cf:4:O2onvM62pC1io6jQKm8Nc2UyFXcd4kOmOsBIoYtZ2imPiVs8r-LJUGA50OKmY4JWgARnT-jSN3hQkuQNaq9IPk_GAWhwXzHxAVlhOM4hqjV8DTKgZPQj3D7kqjq_U_gD",
            "cc:4:20:O2onvM62pC1io6jQKm8Nc2UyFXcd4kOmOsBIoYtZ2ik:96");

        sk.fill (0xff);
        check (sk, hexblob ("616263"),
            "cf:4:dqFZIESm5PURJlvKc6YE2QsFKdHfYCvjChmpJXZg0fWuxqtqkSKv8PfcuWZ_9hMTaJRzK254wm9bZzEB4mf-Litl-k1T2tR4oa2mTVD9Hf232Ukg3D4aVkpkexy6NWAB",
            "cc:4:20:dqFZIESm5PURJlvKc6YE2QsFKdHfYCvjChmpJXZg0fU:96");
    }

    void testFulfillment ()
    {
        testcase ("Fulfillment");

        std::array<std::uint8_t, 32> sk =
        {{
            0x50, 0xd8, 0x58, 0xe0, 0x98, 0x5e, 0xcc, 0x7f,
            0x60, 0x41, 0x8a, 0xaf, 0x0c, 0xc5, 0xab, 0x58,
            0x7f, 0x42, 0xc2, 0x57, 0x0a, 0x88, 0x40, 0x95,
            0xa9, 0xe8, 0xcc, 0xac, 0xd0, 0xf6, 0x54, 0x5c
        }};

        std::vector<std::uint8_t> const v1 (512, 0x21);
        std::vector<std::uint8_t> const v2 (512, 0x22);

        ed25519_t const f ({ sk }, makeSlice(v1));

        // First check against incorrect conditions:
        char const* const ccs[] =
        {
            "cc:0:3:PWh2oBRt6FdusjlahY3hIT0bksZbd53zozHP1aRYRUY:256",
            "cc:1:25:XkflBmyISKuevH8-850LuMrzN-HT1Ds9zKUEzaZ2Wk0:103",
            "cc:2:2b:d3O4epRCo_3rj17Bf3v8hp5ig7vq84ivPok07T9Rdl0:146",
            "cc:3:11:uKkFs6dhGZCwD51c69vVvHYSp25cRi9IlvXfFaxhMjo:518",
            "cc:4:20:O2onvM62pC1io6jQKm8Nc2UyFXcd4kOmOsBIoYtZ2ik:96"
        };

        for (auto cc : ccs)
        {
            auto c = load_condition (cc);

            if (expect (c))
            {
                expect (! validate (f, c.get(), makeSlice(v1)));
                expect (! validate (f, c.get(), makeSlice(v2)));
            }
        }

        // Now, finally, check the correct condition:
        auto c = load_condition (
            "cc:4:20:RCmTBlAEqh5MSPTdAVgZTAI0m8xmTNluQA6iaZGKjVE:96");

        if (expect (c, "000"))
        {
            expect (validate (f, c.get(), makeSlice(v1)));
            expect (validate (f, makeSlice(v1)));

            expect (! validate (f, c.get(), makeSlice(v2)));
            expect (! validate (f, makeSlice(v2)));
        }
    }

    void run ()
    {
        testKnownVectors ();
        testFulfillment ();
    }
};

BEAST_DEFINE_TESTSUITE (ed25519, conditions, ripple);

}

}
