// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <qt/quarlcoinunits.h>

#include <consensus/amount.h>

#include <QStringList>

#include <cassert>

static constexpr auto MAX_DIGITS_QRL = 16;

QuarlcoinUnits::QuarlcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<QuarlcoinUnit> QuarlcoinUnits::availableUnits()
{
    QList<QuarlcoinUnit> unitlist;
    unitlist.append(Unit::QRL);
    unitlist.append(Unit::mQRL);
    unitlist.append(Unit::uQRL);
    unitlist.append(Unit::QUARK);
    return unitlist;
}

QString QuarlcoinUnits::longName(Unit unit)
{
    switch (unit) {
    case Unit::QRL: return QString("QRL");
    case Unit::mQRL: return QString("mQRL");
    case Unit::uQRL: return QString::fromUtf8("µQRL");
    case Unit::QUARK: return QString("quark");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString QuarlcoinUnits::shortName(Unit unit)
{
    switch (unit) {
    case Unit::QRL: return longName(unit);
    case Unit::mQRL: return longName(unit);
    case Unit::uQRL: return QString::fromUtf8("µQRL");
    case Unit::QUARK: return QString("quark");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString QuarlcoinUnits::description(Unit unit)
{
    switch (unit) {
    case Unit::QRL: return QString("Quarlcoin coins");
    case Unit::mQRL: return QString("Milli-QRL (1 / 1" THIN_SP_UTF8 "000)");
    case Unit::uQRL: return QString("Micro-QRL (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case Unit::QUARK: return QString("quark (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

qint64 QuarlcoinUnits::factor(Unit unit)
{
    switch (unit) {
    case Unit::QRL: return 100'000'000;
    case Unit::mQRL: return 100'000;
    case Unit::uQRL: return 100;
    case Unit::QUARK: return 1;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

int QuarlcoinUnits::decimals(Unit unit)
{
    switch (unit) {
    case Unit::QRL: return 8;
    case Unit::mQRL: return 5;
    case Unit::uQRL: return 2;
    case Unit::QUARK: return 0;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString QuarlcoinUnits::format(Unit unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool justify)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    QString quotient_str = QString::number(quotient);
    if (justify) {
        quotient_str = quotient_str.rightJustified(MAX_DIGITS_QRL - num_decimals, ' ');
    }

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == SeparatorStyle::ALWAYS || (separators == SeparatorStyle::STANDARD && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    if (num_decimals > 0) {
        qint64 remainder = n_abs % coin;
        QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');
        return quotient_str + QString(".") + remainder_str;
    } else {
        return quotient_str;
    }
}


// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString QuarlcoinUnits::formatWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}

QString QuarlcoinUnits::formatHtmlWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString QuarlcoinUnits::formatWithPrivacy(Unit unit, const CAmount& amount, SeparatorStyle separators, bool privacy)
{
    assert(amount >= 0);
    QString value;
    if (privacy) {
        value = format(unit, 0, false, separators, true).replace('0', '#');
    } else {
        value = format(unit, amount, false, separators, true);
    }
    return value + QString(" ") + shortName(unit);
}

bool QuarlcoinUnits::parse(Unit unit, const QString& value, CAmount* val_out)
{
    if (value.isEmpty()) {
        return false; // Refuse to parse invalid unit or empty string
    }
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    const QString& whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString QuarlcoinUnits::getAmountColumnTitle(Unit unit)
{
    return QObject::tr("Amount") + " (" + shortName(unit) + ")";
}

int QuarlcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant QuarlcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(longName(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant::fromValue(unit);
        }
    }
    return QVariant();
}

CAmount QuarlcoinUnits::maxMoney()
{
    return MAX_MONEY;
}

namespace {
qint8 ToQint8(QuarlcoinUnit unit)
{
    switch (unit) {
    case QuarlcoinUnit::QRL: return 0;
    case QuarlcoinUnit::mQRL: return 1;
    case QuarlcoinUnit::uQRL: return 2;
    case QuarlcoinUnit::QUARK: return 3;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QuarlcoinUnit FromQint8(qint8 num)
{
    switch (num) {
    case 0: return QuarlcoinUnit::QRL;
    case 1: return QuarlcoinUnit::mQRL;
    case 2: return QuarlcoinUnit::uQRL;
    case 3: return QuarlcoinUnit::QUARK;
    }
    assert(false);
}
} // namespace

QDataStream& operator<<(QDataStream& out, const QuarlcoinUnit& unit)
{
    return out << ToQint8(unit);
}

QDataStream& operator>>(QDataStream& in, QuarlcoinUnit& unit)
{
    qint8 input;
    in >> input;
    unit = FromQint8(input);
    return in;
}
