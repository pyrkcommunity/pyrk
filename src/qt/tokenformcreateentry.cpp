#include "tokenformcreateentry.h"
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
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormCreateEntry::TokenFormCreateEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addCreateEntry();
}

TokenFormCreateEntry::~TokenFormCreateEntry()
{
    delete ui;
}

void TokenFormCreateEntry::addCreateEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    // Add titles for first line of each row, looks tidier!
    QVBoxLayout* titlesLayout = new QVBoxLayout();

    QLabel *labelTicker = new QLabel(this);
    labelTicker->setText("Ticker");

    QLabel *labelAmount = new QLabel(this);
    labelAmount->setText("Amount");

    QLabel *labelLogo = new QLabel(this);
    labelLogo->setText("Logo URI");

    titlesLayout->addWidget(labelTicker);
    titlesLayout->addWidget(labelAmount);
    titlesLayout->addWidget(labelLogo);

    // Rest of the form goes here
    QVBoxLayout* formLayout = new QVBoxLayout();

    // Ticker / Token name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSpacing(5);

    QLineEdit* editTicker = new QLineEdit(this);
    editTicker->setObjectName("edit_create_ticker");
    editTicker->setMaxLength(5);
    editTicker->setFixedWidth(60);
    //editTicker->setStyleSheet("QLineEdit { background : white; }");

    QLabel *labelName = new QLabel(this);
    labelName->setText("Name");

    QLineEdit* editName = new QLineEdit(this);
    editName->setObjectName("edit_create_name");
    editName->setMaxLength(20);
    editName->setFixedWidth(240);
    //editName->setStyleSheet("QLineEdit { background : white; }");

    nameLayout->addWidget(editTicker);
    nameLayout->addWidget(labelName);
    nameLayout->addWidget(editName);
    nameLayout->addStretch();

    // Amount / Doc URI
    QHBoxLayout* amountLayout = new QHBoxLayout();
    amountLayout->setSpacing(5);

    QValidatedLineEdit* editAmount = new QValidatedLineEdit(this);
    editAmount->setObjectName("edit_create_amount");
    editAmount->setMaxLength(12);
    editAmount->setFixedWidth(120);
    QDoubleValidator* validator = new QDoubleValidator(0, 184000000000, 0, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    editAmount->setCheckValidator(validator);
    //editAmount->setStyleSheet("QLineEdit { background : white; }");

    QLabel *labelDocURI = new QLabel(this);
    labelDocURI->setText("Document URI");

    QLineEdit* editDocURI = new QLineEdit(this);
    editDocURI->setObjectName("edit_create_doc_uri");
    editDocURI->setMaxLength(32);
    //editDocURI->setStyleSheet("QLineEdit { background : white; }");

    amountLayout->addWidget(editAmount);
    amountLayout->addWidget(labelDocURI);
    amountLayout->addWidget(editDocURI);

    // Logo URI / Create button
    QHBoxLayout* logoLayout = new QHBoxLayout();

    QLineEdit* editLogoURI = new QLineEdit(this);
    editLogoURI->setObjectName("edit_create_logo_uri");
    editLogoURI->setMaxLength(80);
    //editLogoURI->setStyleSheet("QLineEdit { background : white; }");

    QPushButton* createButton = new QPushButton(this);
    createButton->setText("Create Token");

    createButton->setStyleSheet(QString::fromUtf8("QPushButton{border:1px solid rgb(255,255,255); border-radius:4px;}"));

    logoLayout->addWidget(editLogoURI);
    logoLayout->addWidget(createButton);

    formLayout->addLayout(nameLayout);
    formLayout->addLayout(amountLayout);
    formLayout->addLayout(logoLayout);

    baseLayout->addLayout(titlesLayout);
    baseLayout->addLayout(formLayout);

    connect(createButton, SIGNAL(clicked()), this, SLOT(createTokenClicked()));
}

void TokenFormCreateEntry::createTokenClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    QValidatedLineEdit* edit_create_amount = findChild<QValidatedLineEdit*>("edit_create_amount");
    if (edit_create_amount) {
        if (!edit_create_amount->isValid()) {
            QMessageBox::critical(nullptr, tr("Error"),
                tr("Amount cannot be more than 184,000,000,000."));
            return;
        }
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }

    if (!g_connman) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Peer-to-peer functionality missing or disabled."));
        return;
    }

    QLineEdit* tickerObj = findChild<QLineEdit*>("edit_create_ticker");
    QLineEdit* nameObj = findChild<QLineEdit*>("edit_create_name");
    QLineEdit* genesisamountObj = findChild<QLineEdit*>("edit_create_amount");
    QLineEdit* documenturiObj = findChild<QLineEdit*>("edit_create_doc_uri");
    QLineEdit* logouriObj = findChild<QLineEdit*>("edit_create_logo_uri");

    std::string ticker, name, genesisamount, documenturi, logouri;

    if (tickerObj && nameObj && genesisamountObj && documenturiObj && logouriObj) {
        ticker = tickerObj->text().toStdString();
        name = nameObj->text().toStdString();
        genesisamount = genesisamountObj->text().toStdString();
        documenturi = documenturiObj->text().toStdString();
        logouri = logouriObj->text().toStdString();
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }

    if (ticker.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Ticker cannot be empty."));
        return;
    }

    if (name.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token name cannot be empty."));
        return;
    }

    if (genesisamount.empty() || std::stold(genesisamount) <= 0) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Amount must be set to more than 0."));
        return;
    }

    if (documenturi.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Document URI cannot be empty."));
        return;
    }

    if (logouri.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Logo URI cannot be empty."));
        return;
    }

    std::string tickerhex = string_to_hex(ticker);
    std::string namehex = string_to_hex(name);
    std::string gintvalhex = llint_to_hex((uint64_t)(std::stold(genesisamount)*100000000));
    std::string documenturihex = string_to_hex(documenturi);
    std::string logourihex = string_to_hex(logouri);

    padTo(tickerhex, 10);
    padTo(namehex, 40);
    padTo(gintvalhex, 16);
    padTo(documenturihex, 70);

    std::string opreturncommand = "34320101";
    std::string opreturndata = opreturncommand + tickerhex + namehex + gintvalhex + documenturihex + logourihex;

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

    // Require input to cover 500000000 fee and 1000 as min to return to sender or we could end up
    // with a less than dust or negative output resulting in a non-standard TX that will be rejected.
    if (!selectInput(setAddress, inputtxid, inputvout, inputvalue, 500000000 + 1000)) {
        // Error message sent to user inside selectInput call
        return;
    }

    CMutableTransaction rawTx;
    CTxIn in(COutPoint(inputtxid, inputvout), CScript(), std::numeric_limits<uint32_t>::max());
    rawTx.vin.push_back(in);

    // Return funds to sender
    CScript scriptPubKeySender = GetScriptForDestination(address.Get());
    CAmount valueout = inputvalue - 500000000;
    CTxOut outSender(valueout, scriptPubKeySender);
    rawTx.vout.push_back(outSender);

    // Token data
    CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
    rawTx.vout.push_back(out);

    if (sendTokenTransaction(rawTx)) {
        clear();
    }
}

void TokenFormCreateEntry::clear()
{
    QLineEdit* tickerObj = findChild<QLineEdit*>("edit_create_ticker");
    QLineEdit* nameObj = findChild<QLineEdit*>("edit_create_name");
    QLineEdit* genesisamountObj = findChild<QLineEdit*>("edit_create_amount");
    QLineEdit* documenturiObj = findChild<QLineEdit*>("edit_create_doc_uri");
    QLineEdit* logouriObj = findChild<QLineEdit*>("edit_create_logo_uri");

    if (tickerObj && nameObj && genesisamountObj && documenturiObj && logouriObj) {
        tickerObj->setText("");
        nameObj->setText("");
        genesisamountObj->setText("");
        documenturiObj->setText("");
        logouriObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
