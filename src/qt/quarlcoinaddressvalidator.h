// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_QT_QUARLCOINADDRESSVALIDATOR_H
#define QUARLCOIN_QT_QUARLCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class QuarlcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit QuarlcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Quarlcoin address widget validator, checks for a valid quarlcoin address.
 */
class QuarlcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit QuarlcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // QUARLCOIN_QT_QUARLCOINADDRESSVALIDATOR_H
