#include "transcriptionmodel.h"

TranscriptionModel::TranscriptionModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

Qt::ItemFlags TranscriptionModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);
    if (index.column() == 1 || index.column() == 2) // Make Title and Link columns editable
    {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

QVariant TranscriptionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return QStandardItemModel::data(index, role);
    }
    return QStandardItemModel::data(index, role);
}

bool TranscriptionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        return QStandardItemModel::setData(index, value, role);
    }
    return QStandardItemModel::setData(index, value, role);
}
