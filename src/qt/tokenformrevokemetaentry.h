#ifndef TOKENFORMREVOKEMETA_H
#define TOKENFORMREVOKEMETA_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormRevokeMetaEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormRevokeMetaEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormRevokeMetaEntry();

private:
    void addRevokeMetaEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createRevokeMetaClicked();
};

#endif // TOKENFORMREVOKEMETA_H
