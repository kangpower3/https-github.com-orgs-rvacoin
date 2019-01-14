// Copyright (c) 2018-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVENCOIN_IPFS_H
#define RAVENCOIN_IPFS_H


#include <assets/ipfs/client.h>

#define MAX_IPFS_DATA_SIZE 16000

using namespace ipfs;

enum IPFS_STATE {
    NOT_STARTED,
    VERSION_CHECKED,
    CLIENT_CHECKED,
    DAEMON_THREAD_STARTED,
    DAEMON_CLIENT_CONNECTED,
    DAEMON_CLIENT_FAILED_CONNECT,
    FAILED_VERSION,
    FAILED_DAEMON_START
};


std::string IPFSStateToString(IPFS_STATE& state);


extern CCriticalSection cs_ipfs;

extern bool fIPFSThreadStarted;
extern bool fIPFSForceStop;

extern bool fIpfsInit;
extern bool fFoundVersion;
extern IPFS_STATE globalIpfsState;

struct AddResult {
    int size;
    std::string ipfsHash;
};

bool ThreadDaemonStart();
bool ThreadDaemonStop();

bool InitIpfs();
void StartIpfsDaemon();
bool ShutdownIpfsDaemon();
bool GetIpfs(const std::string& ipfshash, std::stringstream& contents);
bool AddIpfsData(const std::string& data, AddResult& result, bool fAlsoPin = true);
bool AddIpfsFile(const std::string& pathtofile, AddResult& result, bool fAlsoPin = true);
bool PinIpfsFile(const std::string& ipfs_id);
bool GetObjectStats(const std::string& ipfshash, int& cumulitiveSize);

bool ParseIPFSJson(ipfs::Json& json, AddResult& result);

void StartUpLocalIpfsNode();
void StopLocalIpfsNode();
void ThreadIpfsStartUp();

bool CheckIPFSHash(const std::string hash, std::string& error);


#endif //RAVENCOIN_IPFS_H
