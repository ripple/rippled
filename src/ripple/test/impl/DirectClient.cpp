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

#include <BeastConfig.h>
#include <ripple/test/DirectClient.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/RPCHandler.h>

namespace ripple {
namespace test {

class DirectClient : public AbstractClient
{
    Application& app_;

public:
    DirectClient(Application& app)
        : app_(app)
    {
    }

    Json::Value
    rpc(Json::Value const& jv) override
    {
        auto loadType = Resource::feeReferenceRPC;
        RPC::Context ctx { app_.journal ("RPCHandler"), jv, app_,
            loadType, app_.getOPs (), app_.getLedgerMaster(), Role::ADMIN };
        Json::Value jr;
        RPC::doCommand(ctx, jr);
        return jr;
    }
};

std::unique_ptr<AbstractClient>
makeDirectClient(Application& app)
{
    return std::make_unique<DirectClient>(app);
}

} // test
} // ripple
