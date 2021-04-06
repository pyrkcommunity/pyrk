#ifndef TOKENFORMADDMETAENTRY_H
#define TOKENFORMADDMETAENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormAddMetaEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormAddMetaEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormAddMetaEntry();

private:
    void addAddMetaEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createAddMetaClicked();
};

#endif // TOKENFORMADDMETAENTRY_H
