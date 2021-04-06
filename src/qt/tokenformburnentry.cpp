#include "tokenformburnentry.h"
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

TokenFormBurnEntry::TokenFormBurnEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addCreateEntry();
}

TokenFormBurnEntry::~TokenFormBurnEntry()
{
    delete ui;
}

void TokenFormBurnEntry::addCreateEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    QLabel *labelBurn = new QLabel(this);
    labelBurn->setText("Burn Amount");

    QValidatedLineEdit* burnAmount = new QValidatedLineEdit(this);
    burnAmount->setObjectName("edit_burn_amount");
    burnAmount->setMaxLength(12);
    burnAmount->setFixedWidth(120);
    QDoubleValidator* validator = new QDoubleValidator(0, 184000000000, 0, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    burnAmount->setCheckValidator(validator);

    QSpacerItem *burnSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    QPushButton* burnButton = new QPushButton(this);
    burnButton->setText("Burn Tokens");

    baseLayout->addWidget(labelBurn);
    baseLayout->addWidget(burnAmount);
    baseLayout->addWidget(burnButton);
    baseLayout->addSpacerItem(burnSpacer);

    connect(burnButton, SIGNAL(clicked()), this, SLOT(burnTokenClicked()));
}


void TokenFormBurnEntry::burnTokenClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    QValidatedLineEdit* edit_burn_amount = findChild<QValidatedLineEdit*>("edit_burn_amount");
    if (edit_burn_amount) {
        if (!edit_burn_amount->isValid()) {
            QMessageBox::critical(nullptr, tr("Error"),
                tr("Amount cannot be more than 184,000,000,000."));
            return;
        }
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (!g_connman) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Peer-to-peer functionality missing or disabled."));
        return;
    }

    QLabel* tokenidObj = parent->findChild<QLabel*>("label_token_id_value_2");
    QLineEdit* amountObj = findChild<QLineEdit*>("edit_burn_amount");

    std::string amount, tokenid;

    if (tokenidObj && amountObj) {
        tokenid = tokenidObj->text().toStdString();
        amount = amountObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (tokenid.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token ID empty, select token from token list."));
        return;
    }

    if (amount.empty() || std::stold(amount) <= 0) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Amount must be set to more than 0."));
        return;
    }

    std::string burnamounthex = llint_to_hex((uint64_t)(std::stold(amount)*100000000));

    padTo(burnamounthex, 16);

    std::string opreturncommand = "34320103";
    std::string opreturndata = opreturncommand + tokenid + burnamounthex;

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

    if (sendTokenTransaction(rawTx)) {
        clear();
    }
}

void TokenFormBurnEntry::clear()
{
    QLineEdit* burnObj = findChild<QLineEdit*>("edit_burn_amount");

    if (burnObj) {
        burnObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
