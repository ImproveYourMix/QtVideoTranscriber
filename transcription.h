#ifndef TRANSCRIPTION_H
#define TRANSCRIPTION_H

#include <QObject>
#include <QThread>
#include <atomic>
#include "transcriber.h"

class Transcription : public QObject {
    Q_OBJECT

public:
    Transcription(const QString &file, const QString &outputFolder, int row, const QString &title, const QString &link, QObject *parent = nullptr);
    ~Transcription();
    void start();
    void abort();
    int getRow() const;
    bool isAborted() const;

signals:
    void progressUpdated(int row, int progress);
    void statusUpdated(int row, const QString &status);
    void transcriptionFinished(int row, bool aborted);

private slots:
    void onTranscriptionFinished(bool aborted);

private:
    QString file;
    QString outputFolder;
    int row;
    QString title;
    QString link;
    QThread *thread;
    Transcriber *transcriber;
    std::atomic<bool> abortFlag; // Use atomic to safely signal abort
};

#endif // TRANSCRIPTION_H
