#ifndef TRANSCRIPTIONQUEUEMANAGER_H
#define TRANSCRIPTIONQUEUEMANAGER_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include "transcription.h"

class TranscriptionQueueManager : public QObject
{
    Q_OBJECT

public:
    explicit TranscriptionQueueManager(QObject *parent = nullptr);
    void addTranscription(const QString &file, const QString &outputFolder, int row, const QString &title, const QString &link);
    void start();
    void stopAllThreads();
    void stopCurrentThread();

signals:
    void allThreadsFinished();
    void progressUpdated(int row, int progress);
    void statusUpdated(int row, const QString &status);

private slots:
    void onTranscriptionFinished(int row, bool aborted);

private:
    void startNextTranscription();

    QQueue<Transcription*> queue;
    QMap<int, Transcription*> activeTranscriptions;
};

#endif // TRANSCRIPTIONQUEUEMANAGER_H
