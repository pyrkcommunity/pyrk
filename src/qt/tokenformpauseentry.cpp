#include "tokenformpauseentry.h"
#include "ui_tokenformentry.h"

#include "base58.h"
#include "net.h"
#include "pyrktokens.h"
#include "primitives/transaction.h"
#include "qvalidatedlineedit.h"
#include "tokenutil.h"
#include "utilstrencodings.h"
#include "wallet/rpcwallet.h"
#include "walletmodel.h"

#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormPauseEntry::TokenFormPauseEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addPauseEntry();
}

TokenFormPauseEntry::~TokenFormPauseEntry()
{
    delete ui;
}

void TokenFormPauseEntry::addPauseEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    QPushButton* pauseButton = new QPushButton(this);
    pauseButton->setText("Pause");

    QSpacerItem* pauseSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    baseLayout->addWidget(pauseButton);
    baseLayout->addSpacerItem(pauseSpacer);

    connect(pauseButton, SIGNAL(clicked()), this, SLOT(createPauseClicked()));
}

void TokenFormPauseEntry::createPauseClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    if (!g_connman) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Peer-to-peer functionality missing or disabled."));
        return;
    }

    QLabel* tokenidObj = parent->findChild<QLabel*>("label_token_id_value_2");

    std::string tokenid;
    if (tokenidObj) {
        tokenid = tokenidObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    std::string opreturncommand = "34320105";
    std::string opreturndata = opreturncommand + tokenid;

    std::string currenttokenaddress;
    if (!parent->tokenAddress(currenttokenaddress)) {
        // tokenAddress shows errors to user on failure
        return;
    }

    std::set<CBitcoinAddress> setAddress;
    CBitcoinAddress address(currenttokenaddress);
    setAddress.insert(address);

    uint256 inputtxid;
    int inputvout;
    CAmount inputvalue;

    // Require input to cover 1000 fee and 1000 as min to return to sender or we could end up
    // with a less than dust or negative output resulting in a non-standard TX that will be rejected.
    if (!selectInput(setAddress, inputtxid, inputvout, inputvalue, 1000 + 1000)) {
        // Error message sent to user inside selectInput call
        return;
    }

    CMutableTransaction rawTx;
    CTxIn in(COutPoint(inputtxid, inputvout), CScript(), std::numeric_limits<uint32_t>::max());
    rawTx.vin.push_back(in);

    // Return funds to sender
    CScript scriptPubKeySender = GetScriptForDestination(address.Get());
    CAmount valueout = inputvalue - 1000;
    CTxOut outSender(valueout, scriptPubKeySender);
    rawTx.vout.push_back(outSender);

    // Token data
    CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
    rawTx.vout.push_back(out);

    sendTokenTransaction(rawTx);
}
