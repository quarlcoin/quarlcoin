// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_QT_PEERTABLESORTPROXY_H
#define QUARLCOIN_QT_PEERTABLESORTPROXY_H

#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class PeerTableSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PeerTableSortProxy(QObject* parent = nullptr);

protected:
    bool lessThan(const QModelIndex& left_index, const QModelIndex& right_index) const override;
};

#endif // QUARLCOIN_QT_PEERTABLESORTPROXY_H
