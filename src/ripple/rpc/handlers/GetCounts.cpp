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

#include <BeastConfig.h>
#include <ripple/app/data/DatabaseCon.h>
#include <ripple/app/data/SqliteDatabase.h>
#include <ripple/app/ledger/AcceptedLedger.h>
#include <ripple/basics/UptimeTimer.h>
#include <ripple/nodestore/Database.h>

namespace ripple {

// {
//   min_count: <number>  // optional, defaults to 10
// }
Json::Value doGetCounts (RPC::Context& context)
{
    auto lock = getApp().masterLock();

    int minCount = 10;

    if (context.params.isMember ("min_count"))
        minCount = context.params["min_count"].asUInt ();

    auto objectCounts = CountedObjects::getInstance ().getCounts (minCount);

    Json::Value ret (Json::objectValue);

    for (auto const& it : objectCounts)
    {
        ret [it.first] = it.second;
    }

    Application& app = getApp();

    int dbKB = app.getLedgerDB ().getDB ()->getKBUsedAll ();

    if (dbKB > 0)
        ret["dbKBTotal"] = dbKB;

    dbKB = app.getLedgerDB ().getDB ()->getKBUsedDB ();

    if (dbKB > 0)
        ret["dbKBLedger"] = dbKB;

    dbKB = app.getTxnDB ().getDB ()->getKBUsedDB ();

    if (dbKB > 0)
        ret["dbKBTransaction"] = dbKB;

    {
        std::size_t c = app.getOPs().getLocalTxCount ();
        if (c > 0)
            ret["local_txs"] = static_cast<Json::UInt> (c);
    }

    ret["write_load"] = app.getNodeStore ().getWriteLoad ();

    ret["SLE_hit_rate"] = app.getSLECache ().getHitRate ();
    ret["node_hit_rate"] = app.getNodeStore ().getCacheHitRate ();
    ret["ledger_hit_rate"] = app.getLedgerMaster ().getCacheHitRate ();
    ret["AL_hit_rate"] = AcceptedLedger::getCacheHitRate ();

    ret["fullbelow_size"] = static_cast<int>(app.getFullBelowCache().size());
    ret["treenode_cache_size"] = app.getTreeNodeCache().getCacheSize();
    ret["treenode_track_size"] = app.getTreeNodeCache().getTrackSize();

    std::string uptime;
    int s = UptimeTimer::getInstance ().getElapsedSeconds ();
    textTime (uptime, s, "year", 365 * 24 * 60 * 60);
    textTime (uptime, s, "day", 24 * 60 * 60);
    textTime (uptime, s, "hour", 60 * 60);
    textTime (uptime, s, "minute", 60);
    textTime (uptime, s, "second", 1);
    ret["uptime"] = uptime;

    auto counters = app.getNodeStore().counters();
    std::uint32_t stores[2];
    *stores = counters.stores.load();
    ret["node_writes_h"] = stores[0];
    ret["node_writes_l"] = stores[1];
    std::uint32_t fetches[2];
    *fetches = counters.fetches.load();
    ret["node_reads_total_h"] = fetches[0];
    ret["node_reads_total_l"] = fetches[1];
    std::uint32_t fetchHits[2];
    *fetchHits = counters.fetchHits.load();
    ret["node_reads_hit_h"] = fetchHits[0];
    ret["node_reads_hit_l"] = fetchHits[1];
    std::uint32_t storeBytes[2];
    *storeBytes = counters.storeBytes.load();
    ret["node_written_bytes_h"] = storeBytes[0];
    ret["node_written_bytes_l"] = storeBytes[1];
    std::uint32_t fetchBytes[2];
    *fetchBytes = counters.fetchBytes.load();
    ret["node_read_bytes_h"] = fetchBytes[0];
    ret["node_read_bytes_l"] = fetchBytes[1];
    std::uint32_t fetchTime[2];
    *fetchTime = counters.fetchTime.load();
    ret["node_read_time_h"] = fetchTime[0];
    ret["node_read_time_l"] = fetchTime[1];

    return ret;
}

} // ripple
