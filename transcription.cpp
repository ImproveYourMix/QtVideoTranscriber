#include "transcription.h"

Transcription::Transcription(const QString &file, const QString &outputFolder, int row, const QString &title, const QString &link, QObject *parent)
    : QObject(parent), file(file), outputFolder(outputFolder), row(row), title(title), link(link), abortFlag(false) {
    thread = new QThread;
    transcriber = new Transcriber(&abortFlag); // Pass the abort flag to the transcriber
    transcriber->moveToThread(thread);

    connect(thread, &QThread::started, transcriber, [this, file, outputFolder, title, link]() {
        transcriber->setFileAndOutput(file, outputFolder);
        transcriber->setVideoInfo(title, link);
        transcriber->startTranscription();
    });

    connect(transcriber, &Transcriber::progressUpdated, this, [this, row](int progress) {
        emit progressUpdated(row, progress);
    });

    connect(transcriber, &Transcriber::statusUpdated, this, [this, row](const QString &status) {
        emit statusUpdated(row, status);
    });

    connect(transcriber, &Transcriber::transcriptionFinished, this, &Transcription::onTranscriptionFinished);
    connect(transcriber, &Transcriber::transcriptionFinished, thread, &QThread::quit);
    connect(transcriber, &Transcriber::transcriptionFinished, transcriber, &Transcriber::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

Transcription::~Transcription() {
    //TODO double check if this is necessary
    //delete thread;
    // delete transcriber;
}

void Transcription::start() {
    thread->start();
}

void Transcription::abort() {
    abortFlag.store(true); // Signal abort
    emit statusUpdated(row, "Is Cancelling");
}

int Transcription::getRow() const {
    return row;
}

bool Transcription::isAborted() const {
    return abortFlag.load();
}

void Transcription::onTranscriptionFinished(bool aborted) {
    emit transcriptionFinished(row, aborted);
}
