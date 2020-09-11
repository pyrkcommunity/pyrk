#include "tokenutil.h"

#include "base58.h"
#include "coins.h"
#include "consensus/validation.h"
#include "policy/policy.h"
#include "net.h"
#include "script/interpreter.h"
#include "script/sign.h"
#include "txmempool.h"
#include "validation.h"
#include "wallet/wallet.h"

#include <QMessageBox>
#include <QString>

bool selectInput(std::set<CBitcoinAddress>& setAddress, uint256& inputtxid, int& inputvout, CAmount& inputvalue, CAmount selectvalue)
{
    std::vector<COutput> vecOutputs;
    pwalletMain->AvailableCoins(vecOutputs, false, nullptr, true);

    for (const COutput& out : vecOutputs) {
        if (out.nDepth < 1 || out.nDepth > 9999999)
            continue;

        CTxDestination dest;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, dest);

        if (setAddress.size() && (!fValidAddress || !setAddress.count(dest)))
            continue;

        if (out.tx->tx->vout[out.i].nValue < selectvalue)
            continue;

        inputtxid = out.tx->GetHash();
        inputvout = out.i;
        inputvalue = out.tx->tx->vout[out.i].nValue;

        // We only use the first result so break here
        break;
    }

    if (inputtxid == uint256()) {
        QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
            QMessageBox::tr("No confirmed inputs available. Add some funds to "
                            "your token addresss or wait for transactions to confirm."));
        return false;
    }

    return true;
}

bool sendTokenTransaction(CMutableTransaction& rawTx)
{
    // Fetch input
    CCoinsView viewDummy;
    CCoinsViewCache viewd(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        viewd.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view
        viewd.AccessCoin(rawTx.vin[0].prevout); // Load entries from viewChain into view; can fail.
        viewd.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(rawTx);

    // Sign what we can:
    CTxIn& txin = rawTx.vin[0];
    const Coin& coin = viewd.AccessCoin(txin.prevout);
    const CScript& prevPubKey = coin.out.scriptPubKey;

    txin.scriptSig.clear();
    SignSignature(*pwalletMain, prevPubKey, rawTx, 0, SIGHASH_ALL);

    ScriptError serror = SCRIPT_ERR_OK;
    if (!VerifyScript(txin.scriptSig, prevPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, 0), &serror)) {
        QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
            QMessageBox::tr("Script verification error."));
        return false;
    }

    // Send Raw Transaction to Network
    CTransactionRef tx(MakeTransactionRef(std::move(rawTx)));
    const uint256& hashTx = tx->GetHash();

    CAmount nMaxRawTxFee = 0;
    bool fBypassLimits = true;
    CCoinsViewCache &view = *pcoinsTip;
    bool fHaveChain = false;
    for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        fHaveChain = !existingCoin.IsSpent();
    }
    bool fHaveMempool = mempool.exists(hashTx);
    if (!fHaveMempool && !fHaveChain) {
        // push to local node and sync with wallets

        CValidationState state;
        bool fMissingInputs;
        if (!AcceptToMemoryPool(mempool, state, tx, !fBypassLimits, &fMissingInputs, false, nMaxRawTxFee)) {
            if (state.IsInvalid()) {
                QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
                    QMessageBox::tr("%1").arg(QString::fromStdString(strprintf("%i: %s", state.GetRejectCode(), state.GetRejectReason()))));
                return false;
            } else {
                if (fMissingInputs) {
                    QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
                        QMessageBox::tr("Missing inputs."));
                    return false;
                }
                QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
                    QMessageBox::tr("%1").arg(QString::fromStdString(state.GetRejectReason())));
                return false;
            }
        }
    } else if (fHaveChain) {
        QMessageBox::critical(nullptr, QMessageBox::tr("Error"),
            QMessageBox::tr("Transaction already in block chain."));
        return false;
    }

    g_connman->RelayTransaction(*tx);

    QMessageBox::information(nullptr, QMessageBox::tr("Token Transaction Successful"),
                         QMessageBox::tr("%1").arg(QString::fromStdString(hashTx.GetHex())));

    return true;
}
