// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

class CBitcoinAddress;
class CRPCTable;
class UniValue;

#include <curl/curl.h>

#include <string>

std::string AccountFromValue(const UniValue& value);
CBitcoinAddress GetAccountAddress(std::string strAccount, bool bForceNew=false);
void RegisterWalletRPCCommands(CRPCTable &t);

CURLcode curl_read(const std::string& url, std::ostream& os, long timeout = 30);
std::string llint_to_hex(uint64_t intValue);
void padTo(std::string &str, const size_t num, const char paddingChar = '0');
std::string string_to_hex(const std::string& in);

#endif //BITCOIN_WALLET_RPCWALLET_H
