#include "transcriptionqueuemanager.h"

TranscriptionQueueManager::TranscriptionQueueManager(QObject *parent)
    : QObject(parent) {}

void TranscriptionQueueManager::addTranscription(const QString &file, const QString &outputFolder, int row, const QString &title, const QString &link) {
    Transcription *transcription = new Transcription(file, outputFolder, row, title, link, this);
    connect(transcription, &Transcription::progressUpdated, this, &TranscriptionQueueManager::progressUpdated);
    connect(transcription, &Transcription::statusUpdated, this, &TranscriptionQueueManager::statusUpdated);
    connect(transcription, &Transcription::transcriptionFinished, this, &TranscriptionQueueManager::onTranscriptionFinished);
    queue.enqueue(transcription);
}

void TranscriptionQueueManager::start() {
    if (!queue.isEmpty()) {
        startNextTranscription();
    }
}

void TranscriptionQueueManager::stopAllThreads() {
    while (!activeTranscriptions.isEmpty()) {
        auto transcription = activeTranscriptions.begin().value();
        transcription->abort();
        activeTranscriptions.erase(activeTranscriptions.begin());
    }
    queue.clear();
}

void TranscriptionQueueManager::stopCurrentThread() {
    if (!activeTranscriptions.isEmpty()) {
        auto transcription = activeTranscriptions.begin().value();
        transcription->abort();
        activeTranscriptions.erase(activeTranscriptions.begin());
        startNextTranscription();
    }
}

void TranscriptionQueueManager::onTranscriptionFinished(int row, bool aborted) {
    auto transcription = activeTranscriptions.take(row);
    transcription->deleteLater();

    if (queue.isEmpty() && activeTranscriptions.isEmpty()) {
        emit allThreadsFinished();
    } else {
        startNextTranscription();
    }
}

void TranscriptionQueueManager::startNextTranscription() {
    if (!queue.isEmpty()) {
        Transcription *transcription = queue.dequeue();
        int row = transcription->getRow();
        activeTranscriptions.insert(row, transcription);
        transcription->start();
    }
}
