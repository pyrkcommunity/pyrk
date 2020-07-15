#include "tokenselectaddressdialog.h"
#include "ui_tokenselectaddressdialog.h"

#include "addresstablemodel.h"
#include "guiutil.h"
#include "walletmodel.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QSettings>
#include <QTableWidgetItem>

QString TokenSelectAddressDialog::selectedAddress = QString();

TokenSelectAddressDialog::TokenSelectAddressDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TokenSelectAddressDialog),
    platformStyle(platformStyle),
    walletModel(0)
{
    ui->setupUi(this);

    // Address table
    ui->token_address_list->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->token_address_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->token_address_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Right click context menu
    QAction *copyAddressAction = new QAction(tr("&Copy Address"), this);
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    connect(ui->token_address_list, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextualMenu(const QPoint&)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(on_copyAddress_clicked()));

    // Address selection
    connect(ui->token_address_list->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(selectionChanged()));

    // OK clicked
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(buttonBoxClicked()));
}

TokenSelectAddressDialog::~TokenSelectAddressDialog()
{
    delete ui;
}

void TokenSelectAddressDialog::setModel(WalletModel *model)
{
    this->walletModel = model;

    if (model && model->getAddressTableModel()) {
        int token_address_row_count = 0;

        for (int i = 0; i < model->getAddressTableModel()->rowCount(QModelIndex()); ++i) {
            QModelIndex index = model->getAddressTableModel()->index(i, 1, QModelIndex());
            QVariant value = model->getAddressTableModel()->data(index, Qt::EditRole);

            if (model->validateAddress(value.toString())) {
                ui->token_address_list->insertRow(token_address_row_count);
                QTableWidgetItem* item = new QTableWidgetItem(value.toString());
                ui->token_address_list->setItem(token_address_row_count, 0, item);
                ++token_address_row_count;
            }
        }
    }
}

void TokenSelectAddressDialog::buttonBoxClicked()
{
    done(QDialog::Accepted);
}

void TokenSelectAddressDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->token_address_list->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TokenSelectAddressDialog::on_copyAddress_clicked()
{
    if(!ui->token_address_list->selectionModel())
        return;

    QModelIndexList selection = ui->token_address_list->selectionModel()->selectedRows(0);
    GUIUtil::setClipboard(selection.at(0).data(0).toString());
}

void TokenSelectAddressDialog::selectionChanged()
{
    if(!ui->token_address_list->selectionModel())
        return;

    if(ui->token_address_list->selectionModel()->hasSelection())
    {
        QModelIndexList selection = ui->token_address_list->selectionModel()->selectedRows(0);
        QString address = selection.at(0).data(0).toString();
        if (address.toStdString() != "" && walletModel && walletModel->validateAddress(address)) {
            TokenSelectAddressDialog::selectedAddress = address;
        }
    }
}
