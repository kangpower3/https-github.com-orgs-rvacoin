// Copyright (c) 2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVENCOIN_DIVIDENDS_H
#define RAVENCOIN_DIVIDENDS_H

#include <dbwrapper.h>

class CDividendsDB  : public CDBWrapper {
public:
    explicit CDividendsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CDividendsDB(const CDividendsDB&) = delete;
    CDividendsDB& operator=(const CDividendsDB&) = delete;

    // My snapshot checks
    bool WriteSnapshotCheck(const std::string& asset_name, const int& block_height);
    bool ReadSnapshotCheck(const int& block_height, std::set<std::string>& setAssetNames);


//    bool WriteMyMessageChannel(const std::string& channelname);
//    bool ReadMyMessageChannel(const std::string& channelname);
//    bool EraseMyMessageChannel(const std::string& channelname);
//    bool LoadMyMessageChannels(std::set<std::string>& setChannels);
//
//    bool WriteUsedAddress(const std::string& address);
//    bool ReadUsedAddress(const std::string& address);
//    bool EraseUsedAddress(const std::string& address);

//    // Write / Read Database flags
//    bool WriteFlag(const std::string &name, bool fValue);
//    bool ReadFlag(const std::string &name, bool &fValue);

    bool Flush();
};


#endif //RAVENCOIN_DIVIDENDS_H
