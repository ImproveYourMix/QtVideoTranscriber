#ifndef QTTRANSCRIBERWIDGET_H
#define QTTRANSCRIBERWIDGET_H

#include "transcriptionmodel.h"
#include "transcriptionqueuemanager.h"

#include <QLabel>
#include <QWidget>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class QtTranscriberWidget; }
QT_END_NAMESPACE


class QtTranscriberWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QtTranscriberWidget(QWidget *parent = nullptr);
    ~QtTranscriberWidget();
private slots:
    void selectFiles();
    void selectOutputFolder();
    void transcribeFiles();
    void updateElapsedTime();
    void stopTranscription();
    void stopCurrentTranscription();
    void onAllThreadsFinished();
    void onProgressUpdated(int row, int progress);
    void onStatusUpdated(int row, const QString &status);
    void updateTotalProgress();

private:
    Ui::QtTranscriberWidget *ui;
    QStringList selectedFiles;
    QString outputFolder;
    TranscriptionModel *model;
    QLabel *elapsedTimeLabel;
    QTimer *timer;
    QTime startTime;
    TranscriptionQueueManager *threadQueueManager;
    bool isTranscribing = false;
    QMap<int, int> progressMap;
};

#endif // QTTRANSCRIBERWIDGET_H
