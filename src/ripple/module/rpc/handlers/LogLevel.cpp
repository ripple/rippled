//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

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


namespace ripple {

Json::Value doLogLevel (RPC::Context& context)
{
    // log_level
    if (!context.params_.isMember ("severity"))
    {
        // get log severities
        Json::Value ret (Json::objectValue);
        Json::Value lev (Json::objectValue);

        lev["base"] = Log::severityToString (LogSink::get()->getMinSeverity ());
        std::vector< std::pair<std::string, std::string> > logTable (
            deprecatedLogs().partition_severities());
        typedef std::map<std::string, std::string>::value_type stringPair;
        BOOST_FOREACH (const stringPair & it, logTable)
            lev[it.first] = it.second;

        ret["levels"] = lev;
        return ret;
    }

    LogSeverity sv = Log::stringToSeverity (context.params_["severity"].asString ());

    if (sv == lsINVALID)
        return rpcError (rpcINVALID_PARAMS);

    // log_level severity
    if (!context.params_.isMember ("partition"))
    {
        // set base log severity
        LogSink::get()->setMinSeverity (sv, true);
        return Json::objectValue;
    }

    // log_level partition severity base?
    if (context.params_.isMember ("partition"))
    {
        // set partition severity
        std::string partition (context.params_["partition"].asString ());

        if (boost::iequals (partition, "base"))
            LogSink::get()->setMinSeverity (sv, false);
        else if (!LogPartition::setSeverity (partition, sv))
            return rpcError (rpcINVALID_PARAMS);

        return Json::objectValue;
    }

    return rpcError (rpcINVALID_PARAMS);
}

} // ripple
