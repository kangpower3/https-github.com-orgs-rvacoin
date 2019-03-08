// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assets/assets.h"
#include "assets/assetdb.h"
#include "assets/messages.h"
#include "assets/messagedb.h"
#include <map>
#include "tinyformat.h"

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "httpserver.h"
#include "validation.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/fees.h"
#include "policy/policy.h"
#include "policy/rbf.h"
#include "rpc/mining.h"
#include "rpc/safemode.h"
#include "rpc/server.h"
#include "script/sign.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#include "wallet/coincontrol.h"
#include "wallet/feebumper.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "assets/dividendsdb.h"

UniValue createdividendsdatabase(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
                "createdividendsdatabase \n"
                "\nCreates a database entry, were the wallet looks to see if it needs to create a database snapshot of the current assetindex database\n"

                "\nArguments:\n"
                "asset_name:   (string required) The asset name that you want the snapshot to take from\n"
                "block_height: (number required) The block height at which to take the snapshot of the asset database for the given asset_name\n"

                "\nResult:\n"

                "\nExamples:\n"
                + HelpExampleCli("createdividendsdatabase", "\"ASSETNAME\" 400000")
                + HelpExampleRpc("createdividendsdatabase", "\"ASSETNAME\" 400000")
        );

    if (!fAssetIndex) {
        UniValue ret(UniValue::VSTR);
        ret.push_back("Asset Index is required to make a dividend call. To enable assetindex, run the wallet with -assetindex or add assetindex from your raven.conf and perform a -reindex");
        return ret;
    }

    std::string asset_name = request.params[0].get_str();
    int64_t block_height = request.params[1].get_int64();

    AssetType assetType;

    if (!IsAssetNameValid(asset_name, assetType))
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset_name: Please use a valid asset_name"));

    if (assetType == AssetType::UNIQUE || assetType == AssetType::OWNER || assetType == AssetType::MSGCHANNEL)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset_name: OWNER, UNQIUE, MSGCHANNEL assets are not allowed for this call"));

    if (block_height < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid block_height: Block heights must be a positive number"));

    if (block_height <= chainActive.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid block_height: Block heights must be greater than the current height of: ") + std::to_string(chainActive.Height()));

    if (!pdividenddb)
        throw JSONRPCError(RPC_DATABASE_ERROR, std::string("Dividend database is not setup. Please restart wallet to try again"));

    if (pdividenddb->WriteSnapshotCheck(asset_name, block_height))
        return "Dividend Snapshot Check was successfully added to the database";

    throw JSONRPCError(RPC_DATABASE_ERROR, std::string("Failed to add Snapshot Check to database"));
}

UniValue getdividenddatabase(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
                "getdividenddatabase \n"
                "\nGet all database snapshot checks for a certain block height\n"

                "\nArguments:\n"
                "block_height: (number required) The block height at which to take the snapshot of the asset database for the given asset_name\n"

                "\nResult:\n"
                "[\n"
                   "asset_name,   (string)\n"
                "]\n"

                "\nExamples:\n"
                + HelpExampleCli("getdividenddatabase", "400000")
                + HelpExampleRpc("getdividenddatabase", "400000")
        );

    if (!fAssetIndex) {
        UniValue ret(UniValue::VSTR);
        ret.push_back("Asset Index is required to make a dividend call. To enable assetindex, run the wallet with -assetindex or add assetindex from your raven.conf and perform a -reindex");
        return ret;
    }

    int64_t block_height = request.params[0].get_int64();

    if (block_height < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid block_height: Block heights must be a positive number"));

    if (!pdividenddb)
        throw JSONRPCError(RPC_DATABASE_ERROR, std::string("Dividend database is not setup. Please restart wallet to try again"));

    std::set<std::string> setAssetNames;
    pdividenddb->ReadSnapshotCheck(block_height, setAssetNames);

    UniValue ret(UniValue::VARR);
    for (auto str: setAssetNames)
        ret.push_back(str);

    return ret;
}


static const CRPCCommand commands[] =
    {           //  category    name                          actor (function)             argNames
                //  ----------- ------------------------      -----------------------      ----------
            { "dividends",      "createdividendsdatabase",    &createdividendsdatabase,    {"asset_name", "block_height"}},
            { "dividends",      "getdividenddatabase",        &getdividenddatabase,        {"block_height"}},
    };

void RegisterDividendsRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
