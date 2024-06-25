#ifndef TRANSCRIBER_H
#define TRANSCRIBER_H

#include <QObject>
#include <QString>
#include <vector>
#include <atomic>
#include <fstream>
#include <sstream>
#include "whisper.h"

class Transcriber : public QObject {
    Q_OBJECT

public:
    explicit Transcriber(std::atomic<bool>* abortFlag, QObject *parent = nullptr);
    void setFileAndOutput(const QString &file, const QString &outputFolder);
    void startTranscription();
    void abortTranscription();
    void setVideoInfo(const QString &title, const QString &link);

signals:
    void progressUpdated(int progress);
    void statusUpdated(const QString &status);
    void transcriptionFinished(bool aborted = false);
    void error(QString err);
    void totalProgressUpdated(int progress);

private:
    QString file;
    QString outputFolder;
    QString videoTitle;
    QString videoHrefLink;
    whisper_params params;
    std::atomic<bool>* abortFlag;

    void whisper_print_progress_callback(struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, int progress, void * user_data);
    void whisper_print_segment_callback(struct whisper_context * ctx, struct whisper_state * /*state*/, int n_new, void * user_data);
    void extractAudio(const QString &inputFile, const QString &outputFile);
    void transcribeFile(const QString &wavFile, const QString &outputFile);
    bool output_json(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s, bool full);
    void updateTotalProgress();
};

struct whisper_print_user_data {
    const whisper_params * params;
    const std::vector<std::vector<float>> * pcmf32s;
    const std::atomic<bool>* is_aborted;
    int progress_prev;
    Transcriber* transcriber;
};

#endif // TRANSCRIBER_H
