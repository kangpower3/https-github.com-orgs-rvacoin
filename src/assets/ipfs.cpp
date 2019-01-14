// Copyright (c) 2018-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <tinyformat.h>
#include <regex>
#include "ipfs.h"

#include "assets/ipfs/client.h"

#include <boost/thread.hpp>



std::string IPFS_GET = "ipfs get /ipfs/";
std::string IPFS_ADD = "ipfs add ";
std::string IPFS_CAT = "ipfs cat ";
std::string IPFS_OBJECT_STAT = "ipfs object stat ";
std::string IPFS_VERSION = "ipfs version";
std::string IPFS_INIT = "ipfs init";
std::string IPFS_DAEMON = std::string("ipfs daemon &");
std::string IPFS_DAEMON_CHECK = std::string("ipfs daemon");
std::string IPFS_SHUTDOWN = "ipfs shutdown";

CCriticalSection cs_ipfs;

std::regex re("\\d+\\.\\d+\\.\\d+");
std::regex RegexDaemonRunning("ipfs daemon is running");




bool fIPFSThreadStarted = false;
bool fIPFSForceStop = false;
bool fInitIpfs = false;
bool fIPFSDaemonStarted = false;

IPFS_STATE globalIpfsState = IPFS_STATE::NOT_STARTED;


std::string IPFSStateToString(IPFS_STATE& state)
{
    switch (state)
    {
        case IPFS_STATE::NOT_STARTED:                   return "NOT_STARTED";
        case IPFS_STATE::VERSION_CHECKED:               return "VERSION_CHECKED";
        case IPFS_STATE::CLIENT_CHECKED:                return "CLIENT_CHECKED";
        case IPFS_STATE::DAEMON_THREAD_STARTED:         return "DAEMON_THREAD_STARTED";
        case IPFS_STATE::DAEMON_CLIENT_CONNECTED:       return "DAEMON_CLIENT_CONNECTED";
        case IPFS_STATE::DAEMON_CLIENT_FAILED_CONNECT:  return "DAEMON_CLIENT_FAILED_CONNECT";
        case IPFS_STATE::FAILED_VERSION:                return "FAILED_VERSION";
        case IPFS_STATE::FAILED_DAEMON_START:           return "FAILED_DAEMON_START";
        default:                                        return "UNKNOWN";
    }
}

bool ThreadDaemonStart()
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    static boost::thread_group* ipfsThreads = new boost::thread_group();

    ipfsThreads->create_thread(boost::bind(&StartIpfsDaemon));

    return true;
}

bool ThreadDaemonStop()
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    static boost::thread_group* ipfsThreads = new boost::thread_group();

    ipfsThreads->create_thread(boost::bind(&ShutdownIpfsDaemon));

    return true;
}

bool InitIpfs()
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    LOCK(cs_ipfs);
    FILE* version;

    bool fVersionFound = false;
    // Check the IPFS Version
    version = popen(IPFS_VERSION.data(), "r");
    while (!feof(version))
    {
        char buffer[100];
        if (fgets(buffer, sizeof(buffer), version) != NULL)
        {
            fputs(buffer, version);
        }

        std::string bufferStr(buffer);
        if (std::regex_search(bufferStr, re)) {
            fVersionFound = true;
            LogPrintf("%s : Found %s\n", __func__, buffer);
            break;
        } else {
            LogPrintf("%s : IPFS version not found\n", __func__);
            break;
        }
    }
    fclose(version);

    if (!fVersionFound)
        return false;

    // TODO if we want to support of a certain verison or greater. Make the check here.

    // Init IPFS
    int init = system(IPFS_INIT.data());
    if (init == 0){
        LogPrintf("%s : Init Successful\n", __func__);
    } else {
        LogPrintf("%s : Init Failed (Ipfs could already be initialized, or an issue has occured during ititialization.\n", __func__);
    }

    return true;
}

// Only call on seperate thread
void StartIpfsDaemon()
{
    LOCK(cs_ipfs);
    // Init IPFS
    int daemon = system(IPFS_DAEMON.data());
    if (daemon == 0){
        LogPrintf("%s : Daemon started Successful\n", __func__);
    } else {
        LogPrintf("%s : Daemon already started, or failed to start\n", __func__);
    }
}

// Only call on seperate thread
bool ShutdownIpfsDaemon()
{
    LOCK(cs_ipfs);
    // Init IPFS
    int init = system(IPFS_SHUTDOWN.data());
    if (init == 0) {
        LogPrintf("%s : Daemon shutdown successful\n", __func__);
    } else {
        LogPrintf("%s : Daemon failed to shutdown, or daemon not running\n", __func__);
    }

    globalIpfsState = IPFS_STATE::NOT_STARTED;

    return true;
}

bool GetIpfs(const std::string& ipfshash, std::stringstream& contents)
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    try {
        ipfs::Client client("localhost", 5001);
        client.FilesGet(std::string("/ipfs/" + ipfshash), &contents);
        return true;
    } catch (...) {
        return false;
    }
}

// fAlsoPin defaulted to true. Don't want users not having the ipfs data pinned if they created it
bool AddIpfsData(const std::string& data, AddResult& result, bool fAlsoPin)
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    // Create file to upload
    ipfs::http::FileUpload file{"", ipfs::http::FileUpload::Type::kFileContents, data};
    std::vector<ipfs::http::FileUpload> vec;
    vec.emplace_back(file);

    try {
        // Create client connection to ipfs node
        ipfs::Client client("localhost", 5001);
        ipfs::Json json;
        // Make the file add call
        client.FilesAdd(vec, &json);
        bool ret = ParseIPFSJson(json, result);
        // Pin the file to the local ipfs node
        if (ret && fAlsoPin)
            client.PinAdd(result.ipfsHash);

        return ret;
    } catch (...) {
        return false;
    }
}

bool AddIpfsFile(const std::string& pathtofile, AddResult& result, bool fAlsoPin)
{
// Example on how to use this command
//    AddResult result2;
//    if (AddIpfsFile("/Users/dirtory/Documents/Ravencoin/tmp_file.txt", result2))
//        std::cout << result2.ipfsHash << std::to_string(result2.size) << std::endl;

    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    // Create file to upload
    ipfs::http::FileUpload file{"", ipfs::http::FileUpload::Type::kFileName, pathtofile};
    std::vector<ipfs::http::FileUpload> vec;
    vec.emplace_back(file);

    // Create client connection to ipfs node
    ipfs::Client client("localhost", 5001);
    ipfs::Json json;

    try {
        // Make the file add call
        client.FilesAdd(vec, &json);

        bool ret = ParseIPFSJson(json, result);

        // Pin the file to the local ipfs node
        if (ret && fAlsoPin)
            client.PinAdd(result.ipfsHash);

        return ret;
    } catch (...) {
        return false;
    }
}

bool PinIpfsFile(const std::string& ipfs_id)
{
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;
    // Create client connection to ipfs node
    ipfs::Client client("localhost", 5001);
    try {
        client.PinAdd(ipfs_id);
    } catch (...) {
        return false;
    }

    return true;
}

bool GetObjectStats(const std::string& ipfshash, int& cumulitiveSize)
{

    std::cout << IPFS_DAEMON << std::endl;
    if (!fIPFS) // Stop if ipfs is turned off on the client
        return false;

    try {
        // Create client connection to ipfs node
        ipfs::Client client("localhost", 5001);
        ipfs::Json json;
        client.ObjectStat(ipfshash, &json);

        if (json.find("CumulativeSize") == json.end())
            return false;

        cumulitiveSize = json.at("CumulativeSize");
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseIPFSJson(ipfs::Json& json, AddResult& result)
{
    // Get the ipfs hash returned from the ipfs call
    if (json.is_array()) {
        for (auto item : json) {
            if (item.is_object()) {
                if (item.count("hash"))
                    result.ipfsHash = item.at("hash");
                if (item.count("size"))
                    result.size = item.at("size");
            }
        }
    } else {
        return false;
    }

    // Check to make sure we got an ipfs hash, and a valid size
    if (result.ipfsHash.empty() || result.size == 0)
        return false;

    return true;
}

void StartUpLocalIpfsNode()
{
    if (fIPFSThreadStarted) {
        LogPrintf("Ipfs Thread already started: Skipping ipfs thread start\n");
        return;
    }

    static boost::thread_group* ipfsThreads = new boost::thread_group();
    ipfsThreads->create_thread(boost::bind(&ThreadIpfsStartUp));
}

void StopLocalIpfsNode()
{
    {
        LOCK(cs_ipfs);
        LogPrintf("Ipfs stopping daemon\n");
        fIPFSForceStop = true;
        globalIpfsState = IPFS_STATE::NOT_STARTED;
        ThreadDaemonStop();
    }
}

void ThreadIpfsStartUp()
{
    if (!fIPFS)
        return;

    LogPrintf("Ipfs Local Node Thread starting\n");
    globalIpfsState = IPFS_STATE::NOT_STARTED;

    int c = 0;

    while(true) {
        fIPFSThreadStarted = true;
        c++;
        if (c > 100)
            c = 0;

        // Check to see if thread is being forcefully stopped (before sleep)
        if (fIPFSForceStop) {
            break;
        }

        boost::this_thread::interruption_point();
        MilliSleep(30000);
        boost::this_thread::interruption_point();

        // Check to see if thread is being forcefully stopped (after sleep)
        if (fIPFSForceStop) {
            break;
        }

        {
            LOCK(cs_ipfs);

            // Check if the state hasn't started yet.
            if (globalIpfsState == IPFS_STATE::NOT_STARTED) {
                if (InitIpfs()) {
                    globalIpfsState = IPFS_STATE::VERSION_CHECKED;
                    continue;
                } else {
                    globalIpfsState = IPFS_STATE::FAILED_VERSION;
                }
            }

            // Check to see if the version check passed, and start the ipfs daemon
            if (globalIpfsState == IPFS_STATE::VERSION_CHECKED) {
                ThreadDaemonStart();
                globalIpfsState = IPFS_STATE::DAEMON_THREAD_STARTED;
                continue;
            }

            // If the ipfs daemon was started, check the client connect call
            if (globalIpfsState == IPFS_STATE::DAEMON_THREAD_STARTED) {
                std::stringstream ss;
                if (!GetIpfs("QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG/readme", ss))
                    globalIpfsState = IPFS_STATE::DAEMON_CLIENT_FAILED_CONNECT;
                else
                    globalIpfsState = IPFS_STATE::DAEMON_CLIENT_CONNECTED;
                continue;
            }

            // If the client has been connected for 10 rounds of 30 second checking, make sure they are still connected
            if (globalIpfsState == DAEMON_CLIENT_CONNECTED) {
                if (c % 10) {
                    std::stringstream ss;
                    if (!GetIpfs("QmYwAPJzv5CZsnA625s3Xf2nemtYgPpHdWEz79ojWnPbdG/readme", ss))
                        globalIpfsState = IPFS_STATE::DAEMON_CLIENT_FAILED_CONNECT;
                    else
                        globalIpfsState = IPFS_STATE::DAEMON_CLIENT_CONNECTED;
                    continue;
                }
            }

            if (globalIpfsState == IPFS_STATE::FAILED_VERSION || globalIpfsState == DAEMON_CLIENT_FAILED_CONNECT) {
                break;
            }
        }
    }

    fIPFSThreadStarted = false;
    ThreadDaemonStop();
    LogPrintf("Ipfs Local Node Thread exiting\n");
}

bool CheckIPFSHash(const std::string hash, std::string& error) {
    if (!CheckEncodedIPFS(hash, error)) {
        return false;
    }
    else if (hash.size() != 46) {
        error = _("IPFS Hash must have size of 46 characters");
        return false;
    } else if (DecodeIPFS(hash).empty()) {
        error = _("IPFS hash is not valid. Please use a valid IPFS hash");
        return false;
    }

    return true;
}