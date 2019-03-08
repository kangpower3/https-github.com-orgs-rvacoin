// Copyright (c) 2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dividendsdb.h"

static const char SNAPSHOTCHECK_FLAG = 'C'; // Snapshot Check

CDividendsDB::CDividendsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "dividends", nCacheSize, fMemory, fWipe) {
}

bool CDividendsDB::WriteSnapshotCheck(const std::string& asset_name, const int& block_height)
{
    std::set<std::string> setAssetNames;
    ReadSnapshotCheck(block_height, setAssetNames);
    setAssetNames.insert(asset_name);

    return Write(std::make_pair(SNAPSHOTCHECK_FLAG, block_height), setAssetNames);
}

bool CDividendsDB::ReadSnapshotCheck(const int& block_height, std::set<std::string>& setAssetNames)
{
    return Read(std::make_pair(SNAPSHOTCHECK_FLAG, block_height), setAssetNames);
}