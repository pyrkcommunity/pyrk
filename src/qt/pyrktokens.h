#ifndef PYRKTOKENS_H
#define PYRKTOKENS_H

#include "platformstyle.h"

#include <QMenu>
#include <QWidget>

namespace Ui {
    class PyrkTokens;
}

class ClientModel;
class WalletModel;
class QItemSelection;

/** Payrk Tokens page widget */
class PyrkTokens : public QWidget
{
    Q_OBJECT

public:
    explicit PyrkTokens(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~PyrkTokens();

    void setWalletModel(WalletModel *walletModel);
    bool tokenAddress(std::string &currentTokenAddress);

protected:
    bool event(QEvent* pEvent) override;

private:
    QMenu *contextTokenMenu;
    QMenu *contextTXMenu;
    const PlatformStyle *platformStyle;

private:
    void getTokenAddress();
    void deleteForm();
    void populateTokenList();
    void populateTransactionList();
    void resetTokenInfo();

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

    Ui::PyrkTokens *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void contextualTokenMenu(const QPoint &point);
    void contextualTXMenu(const QPoint &point);
    void copyAddressTokenTable();
    void copyAddressTXTable();
    void createNewForm();
    void populateTokenInfo();
    void setAddressClicked();
    void updateTables();
    void viewOnExplorerClicked();

};
#endif // PYRKTOKENS_H
