#ifndef TRANSCRIBER_H
#define TRANSCRIBER_H

#include <QObject>
#include <QString>
#include <vector>
#include <atomic>
#include <fstream>
#include <sstream>
#include <thread>
#include "whisper.h"

// command-line parameters
struct whisper_params {
    int32_t n_threads     = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors  = 1;
    int32_t offset_t_ms   = 0;
    int32_t offset_n      = 0;
    int32_t duration_ms   = 0;
    int32_t progress_step = 5;
    int32_t max_context   = -1;
    int32_t max_len       = 0;
    int32_t best_of       = whisper_full_default_params(WHISPER_SAMPLING_GREEDY).greedy.best_of;
    int32_t beam_size     = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH).beam_search.beam_size;
    int32_t audio_ctx     = 0;

    float word_thold      =  0.01f;
    float entropy_thold   =  2.40f;
    float logprob_thold   = -1.00f;
    float grammar_penalty = 100.0f;
    float temperature     = 0.0f;
    float temperature_inc = 0.2f;

    bool debug_mode      = false;
    bool translate       = false;
    bool detect_language = false;
    bool diarize         = false;
    bool tinydiarize     = false;
    bool split_on_word   = false;
    bool no_fallback     = false;
    bool output_txt      = false;
    bool output_vtt      = false;
    bool output_srt      = false;
    bool output_wts      = false;
    bool output_csv      = false;
    bool output_jsn      = true;
    bool output_jsn_full = true;
    bool output_lrc      = false;
    bool no_prints       = false;
    bool print_special   = false;
    bool print_colors    = false;
    bool print_progress  = false;
    bool no_timestamps   = false;
    bool log_score       = false;
    bool use_gpu         = true;
    bool flash_attn      = false;

    std::string language  = "it";
    std::string prompt;
    std::string model     = "./models/ggml-medium.bin";
    std::string grammar;
    std::string grammar_rule;

    // [TDRZ] speaker turn string
    std::string tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    // A regular expression that matches tokens to suppress
    std::string suppress_regex;

    std::string openvino_encode_device = "CPU";

    std::string dtw = "";

    std::vector<std::string> fname_inp = {};
    std::vector<std::string> fname_out = {};

};

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
    int64_t get_current_timestamp_ms();
};

struct whisper_print_user_data {
    const whisper_params * params;
    const std::vector<std::vector<float>> * pcmf32s;
    const std::atomic<bool>* is_aborted;
    int progress_prev;
    Transcriber* transcriber;
};


#endif // TRANSCRIBER_H
