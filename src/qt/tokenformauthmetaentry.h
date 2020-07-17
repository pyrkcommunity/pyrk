#ifndef TOKENFORMAUTHMETAENTRY_H
#define TOKENFORMAUTHMETAENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormAuthMetaEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormAuthMetaEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormAuthMetaEntry();

private:
    void addAuthMetaEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createAuthMetaClicked();
};

#endif // TOKENFORMAUTHMETAENTRY_H
