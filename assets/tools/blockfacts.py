#Shows data from the first 1000 blocks

import random
import os
import subprocess
import json


#Set this to your raven-cli program
cli = "raven-cli"

#mode = "-testnet"
mode = ""
rpc_port = 18766
#Set this information in your raven.conf file (in datadir, not testnet3)
rpc_user = 'rpcuser'
rpc_pass = 'rpcpass555'

def get_rpc_connection():
    from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
    connection = "http://%s:%s@127.0.0.1:%s"%(rpc_user, rpc_pass, rpc_port)
    rpc_conn = AuthServiceProxy(connection)
    return(rpc_conn)

rpc_connection = get_rpc_connection()

def rpc_call(params):
    process = subprocess.Popen([cli, mode, params], stdout=subprocess.PIPE)
    out, err = process.communicate()
    return(out)

def get_blockinfo(num):
    rpc_connection = get_rpc_connection()
    hash = rpc_connection.getblockhash(num)
    blockinfo = rpc_connection.getblock(hash)
    return(blockinfo)

def get_bci():
    bci = rpc_connection.getblockchaininfo()
    return(bci)    

def get_rpc_connection():
    from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
    connection = "http://%s:%s@127.0.0.1:%s"%(rpc_user, rpc_pass, rpc_port)
    #print("Connection: " + connection)
    rpc_connection = AuthServiceProxy(connection)
    return(rpc_connection)

#Get the blockheight of the chain
blockheight = get_bci().get('blocks')

max_sz = 0
for i in range(1,blockheight):
    dta = get_blockinfo(i)
    print("Block #" + str(i))
    print(dta.get('hash'))
    print(dta.get('difficulty'))
    print(dta.get('time'))
    sz = dta.get('size')
    if (sz > max_sz):
        max_sz = sz
    print(str(sz))
    print("")

print('Max size: ' + str(max_sz))
