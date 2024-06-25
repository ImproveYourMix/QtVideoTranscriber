#ifndef TRANSCRIPTIONMODEL_H
#define TRANSCRIPTIONMODEL_H

#include <QStandardItemModel>

class TranscriptionModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit TranscriptionModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};

#endif // TRANSCRIPTIONMODEL_H
