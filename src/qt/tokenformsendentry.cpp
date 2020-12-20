#include "tokenformsendentry.h"
#include "ui_tokenformentry.h"

#include "base58.h"
#include "guiutil.h"
#include "net.h"
#include "pyrktokens.h"
#include "primitives/transaction.h"
#include "qvalidatedlineedit.h"
#include "tokenutil.h"
#include "utilstrencodings.h"
#include "wallet/rpcwallet.h"
#include "walletmodel.h"

#include <nlohmann/json.hpp>

#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormSendEntry::TokenFormSendEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addSendEntry();
}

TokenFormSendEntry::~TokenFormSendEntry()
{
    delete ui;
}

void TokenFormSendEntry::addSendEntry() {
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    // Add titles for first line of each row, looks tidier!
    QVBoxLayout* titlesLayout = new QVBoxLayout();

    QLabel *labelAddress = new QLabel(this);
    labelAddress->setText("Recipient");

    QLabel *labelID = new QLabel(this);
    labelID->setText("Payment ID");

    titlesLayout->addWidget(labelAddress);
    titlesLayout->addWidget(labelID);

    // Rest of the form goes here
    QVBoxLayout* formLayout = new QVBoxLayout();

    // ID / Address
    QHBoxLayout* idLayout = new QHBoxLayout();
    idLayout->setSpacing(5);

    QValidatedLineEdit* editAddress = new QValidatedLineEdit(this);
    editAddress->setObjectName("edit_send_address");
    GUIUtil::setupAddressWidget(editAddress, this);
    editAddress->setMaxLength(34);
    editAddress->setFixedWidth(240);

    QLabel *labelAmount = new QLabel(this);
    labelAmount->setText("Amount");

    QValidatedLineEdit* editAmount = new QValidatedLineEdit(this);
    editAmount->setObjectName("edit_send_amount");
    editAmount->setMaxLength(12);
    editAmount->setFixedWidth(120);
    QDoubleValidator* validator = new QDoubleValidator(0, 184000000000, 0, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    editAmount->setCheckValidator(validator);

    idLayout->addWidget(editAddress);
    idLayout->addWidget(labelAmount);
    idLayout->addWidget(editAmount);
    idLayout->addStretch();

    // Payment ID / Send button
    QHBoxLayout* amountLayout = new QHBoxLayout();
    amountLayout->setSpacing(5);

    QLineEdit* editPaymentID = new QLineEdit(this);
    editPaymentID->setObjectName("edit_send_payment_id");
    editPaymentID->setMaxLength(20);

    QSpacerItem *sendSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    QPushButton* sendButton = new QPushButton(this);
    sendButton->setText("Send");
    sendButton->setStyleSheet(QString::fromUtf8("QPushButton{border:1px solid rgb(255,255,255); border-radius:4px;}"));

    amountLayout->addWidget(editPaymentID);
    amountLayout->addSpacerItem(sendSpacer);
    amountLayout->addWidget(sendButton);

    formLayout->addLayout(idLayout);
    formLayout->addLayout(amountLayout);

    baseLayout->addLayout(titlesLayout);
    baseLayout->addLayout(formLayout);

    connect(sendButton, SIGNAL(clicked()), this, SLOT(createSendClicked()));
}

void TokenFormSendEntry::createSendClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    QValidatedLineEdit* edit_send_amount = findChild<QValidatedLineEdit*>("edit_send_amount");
    if (edit_send_amount) {
        if (!edit_send_amount->isValid()) {
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
    QLineEdit* addressObj = findChild<QLineEdit*>("edit_send_address");
    QLineEdit* amountObj = findChild<QLineEdit*>("edit_send_amount");
    QLineEdit* paymentidObj = findChild<QLineEdit*>("edit_send_payment_id");

    std::string tokenid, raddress, amount, paymentid;

    if (tokenidObj && addressObj && amountObj && paymentidObj) {
        tokenid = tokenidObj->text().toStdString();
        raddress = addressObj->text().toStdString();
        amount = amountObj->text().toStdString();
        paymentid = paymentidObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (tokenid.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token ID empty, select token from token list."));
        return;
    }

    CBitcoinAddress addressRecipient(raddress);
    if (raddress.empty() || !addressRecipient.IsValid()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Address empty or invalid."));
        return;
    }

    if (amount.empty() || std::stold(amount) <= 0) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Amount must be set to more than 0."));
        return;
    }

    std::string addresshex = string_to_hex(raddress);
    std::string gintvalhex = llint_to_hex((uint64_t)(std::stold(amount)*100000000));
    std::string paymentidhex = string_to_hex(paymentid);
    std::string opreturncommand = "34320104";

    padTo(addresshex, 34);
    padTo(gintvalhex, 16);

    std::string opreturndata = opreturncommand + tokenid + gintvalhex + addresshex + paymentidhex;

    std::string currenttokenaddress;
    if (!parent->tokenAddress(currenttokenaddress)) {
        // tokenAddress shows errors to user on failure
        return;
    }

    if (currenttokenaddress == addressRecipient.ToString()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Destination token address cannot be the same as the source token address."));
        return;
    }

    std::ostringstream oss;
    if(CURLE_OK == curl_read(GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "tokenaddress/" + tokenid + "/" + currenttokenaddress, oss))
    {
        auto json = nlohmann::json::parse(oss.str());
        std::string tokenbalance = "0";

        if (!json["tokenBalance"].empty()) {
            tokenbalance = json["tokenBalance"];
        }

        if (std::stold(amount) > std::stold(tokenbalance)) {
            QMessageBox::critical(nullptr, tr("Error"),
                tr("Insufficient token balance for send."));
            return;
        }

        std::set<CBitcoinAddress> setAddress;
        CBitcoinAddress address(currenttokenaddress);
        setAddress.insert(address);

        uint256 inputtxid;
        int inputvout;
        CAmount inputvalue;
        CAmount valueoutdust = 1000;

        // Require input to cover 1000 fee and 1000 as min to return to sender or we could end up
        // with a less than dust or negative output resulting in a non-standard TX that will be rejected.
        if (!selectInput(setAddress, inputtxid, inputvout, inputvalue, valueoutdust + 1000 + 1000)) {
            // Error message sent to user inside selectInput call
            return;
        }

        CMutableTransaction rawTx;
        CTxIn in(COutPoint(inputtxid, inputvout), CScript(), std::numeric_limits<uint32_t>::max());
        rawTx.vin.push_back(in);

        // Return funds to sender
        CScript scriptPubKeySender = GetScriptForDestination(address.Get());
        CAmount valueout = inputvalue - valueoutdust - 1000; // 1000 Sat fee
        CTxOut outSender(valueout, scriptPubKeySender);
        rawTx.vout.push_back(outSender);

        // Token data
        CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
        rawTx.vout.push_back(out);

        // Dust to recipient
        CScript scriptPubKeyRecipient = GetScriptForDestination(addressRecipient.Get());
        CTxOut outRecipient(valueoutdust, scriptPubKeyRecipient);
        rawTx.vout.push_back(outRecipient);

        if (sendTokenTransaction(rawTx)) {
            clear();
        }
    }
}

void TokenFormSendEntry::clear()
{
    QLineEdit* addressObj = findChild<QLineEdit*>("edit_send_address");
    QLineEdit* amountObj = findChild<QLineEdit*>("edit_send_amount");
    QLineEdit* paymentidObj = findChild<QLineEdit*>("edit_send_payment_id");

    if (addressObj && amountObj && paymentidObj) {
        addressObj->setText("");
        amountObj->setText("");
        paymentidObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
