#ifndef TOKENFORMNEWOWNERENTRY_H
#define TOKENFORMNEWOWNERENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormNewOwnerEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormNewOwnerEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormNewOwnerEntry();

private:
    void addNewOwnerEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createNewOwnerClicked();
};

#endif // TOKENFORMNEWOWNERENTRY_H
