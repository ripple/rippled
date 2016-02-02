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
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/core/LoadFeeTrack.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/test/WSClient.h>
#include <ripple/test/jtx.h>
#include <beast/unit_test/suite.h>

namespace ripple {
namespace test {

class Subscribe_test : public beast::unit_test::suite
{
public:
    void testServer()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("server");
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // Raise fee to cause an update
            for(int i = 0; i < 5; ++i)
                env.app().getFeeTrack().raiseLocalFee();
            env.app().getOPs().reportFeeChange();
            auto jv = wsc->getMsg(5s);
            expect((*jv)[jss::type] == "serverStatus");
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("server");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }
    }

    void testLedger()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("ledger");
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::result][jss::ledger_index] == 2);
        }

        {
            // Accept a ledger
            env.close();
            auto jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::ledger_index] == 3);
        }

        {
            // Accept another ledger
            env.close();
            auto jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::ledger_index] == 4);
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("ledger");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }
    }

    void testTransactions()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // Transaction to create account
            env.fund(XRP(10000), "alice");
            env.close();
            auto jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][1u]["CreatedNode"]
                ["NewFields"][jss::Account] == Account("alice").human());

            // Transaction to fund account
            jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][0u]["ModifiedNode"]
                ["FinalFields"][jss::Account] == Account("alice").human());

            // Transaction to create account
            env.fund(XRP(10000), "bob");
            env.close();
            jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][1u]["CreatedNode"]
                ["NewFields"][jss::Account] == Account("bob").human());

            // Transaction to fund account
            jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][0u]["ModifiedNode"]
                ["FinalFields"][jss::Account] == Account("bob").human());
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::accounts] = Json::arrayValue;
            jv[jss::accounts].append(Account("alice").human());
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // Transaction to create account
            env.fund(XRP(10000), "carol");
            env.close();
            auto jv = wsc->getMsg(10ms);
            expect(! jv);

            // Transactions concerning alice
            env.trust(Account("bob")["USD"](100), "alice");
            env.close();
            jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][1u]["ModifiedNode"]
                ["FinalFields"][jss::Account] == Account("alice").human());
            jv = wsc->getMsg(5s);
            expect(jv);
            expect((*jv)[jss::meta]["AffectedNodes"][1u]["CreatedNode"]
                ["NewFields"]["LowLimit"][jss::issuer] ==
                    Account("alice").human());
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }
    }

    void testManifests()
    {
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("manifests");
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("manifests");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }
    }

    void testValidations()
    {
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("validations");
            jv = wsc->invoke("subscribe", jv);
            expect(jv[jss::status] == "success");
        }

        {
            // RPC command
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("validations");
            jv = wsc->invoke("unsubscribe", jv);
            expect(jv[jss::status] == "success");
        }
    }

    void run() override
    {
        testServer();
        testLedger();
        testTransactions();
        testManifests();
        testValidations();
    }
};

BEAST_DEFINE_TESTSUITE(Subscribe,app,ripple);

} // test
} // ripple
