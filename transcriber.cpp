#include "transcriber.h"
#include "dr_wav.h"
#include "common.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>

#include <chrono>
#include <ctime>

Transcriber::Transcriber(std::atomic<bool>* abortFlag, QObject *parent)
    : QObject(parent), abortFlag(abortFlag) {}

void Transcriber::setFileAndOutput(const QString &file, const QString &outputFolder) {
    this->file = file;
    this->outputFolder = outputFolder;
}

void Transcriber::startTranscription() {
    if (abortFlag->load()) return;

    QFileInfo fileInfo(file);
    QString wavFile = outputFolder + "/" + fileInfo.baseName() + ".wav";
    QString outputFile = outputFolder + "/" + fileInfo.baseName() + ".json";

    if (fileInfo.suffix() == "mp4") {
        qInfo() << "Extracting audio";
        emit statusUpdated("Extracting audio");
        extractAudio(file, wavFile);
    } else {
        wavFile = file; // Use the WAV file directly
    }

    emit statusUpdated("Transcribing");
    transcribeFile(wavFile, outputFile);
    emit transcriptionFinished();
}

void Transcriber::abortTranscription() {
    abortFlag->store(true);
}

void Transcriber::extractAudio(const QString &inputFile, const QString &outputFile) {
    QProcess process;
    QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg";
    qInfo() << "ffmpeg path" << ffmpegPath;
    process.start(ffmpegPath, QStringList() << "-y" << "-i" << inputFile << "-ar" << "16000" << "-ac" << "1" << outputFile);
    process.waitForFinished();
    qInfo() << process.readAllStandardError();
    qInfo() << process.readAllStandardOutput();
}

void Transcriber::transcribeFile(const QString &wavFile, const QString &outputFile) {
    whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = params.use_gpu;
    std::string basePath = QCoreApplication::applicationDirPath().toStdString();
    std::string modelPath = basePath + "/" + params.model;
    qInfo() << "model path" << QString::fromStdString(modelPath);
    struct whisper_context *ctx = whisper_init_from_file_with_params(modelPath.c_str(), cparams);

    if (!ctx) {
        emit statusUpdated("Failed to initialize Whisper context");
        return;
    }

    std::vector<float> pcmf32;
    std::vector<std::vector<float>> pcmf32s;
    if (!read_wav(wavFile.toStdString(), pcmf32, pcmf32s, false)) {
        emit statusUpdated("Failed to read WAV file");
        whisper_free(ctx);
        return;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_realtime   = false;
    wparams.print_progress   = params.print_progress;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.print_special    = params.print_special;
    wparams.translate        = params.translate;
    wparams.language         = params.language.c_str();
    wparams.n_threads        = params.n_threads;
    wparams.n_max_text_ctx   = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
    wparams.offset_ms        = params.offset_t_ms;
    wparams.duration_ms      = params.duration_ms;

    wparams.token_timestamps = params.output_wts || params.output_jsn_full || params.max_len > 0;
    wparams.thold_pt         = params.word_thold;
    wparams.max_len          = params.output_wts && params.max_len == 0 ? 60 : params.max_len;
    wparams.audio_ctx        = params.audio_ctx;

    wparams.tdrz_enable      = params.tinydiarize;

    wparams.initial_prompt   = params.prompt.c_str();

    wparams.greedy.best_of        = params.best_of;
    wparams.beam_search.beam_size = params.beam_size;

    wparams.entropy_thold    = params.entropy_thold;
    wparams.logprob_thold    = params.logprob_thold;

    wparams.no_timestamps    = params.no_timestamps;

    whisper_print_user_data user_data = { &params, &pcmf32s, abortFlag, 0, this };

    if (!wparams.print_realtime) {
        wparams.new_segment_callback           = [](struct whisper_context * ctx, struct whisper_state * state, int n_new, void * user_data) {
            auto & transcriber  = *((whisper_print_user_data *) user_data)->transcriber;
            transcriber.whisper_print_segment_callback(ctx, state, n_new, user_data);
        };
        wparams.new_segment_callback_user_data = &user_data;
    }

    if (wparams.print_progress) {
        wparams.progress_callback           = [](struct whisper_context * ctx, struct whisper_state * state, int progress, void * user_data){
            auto & transcriber  = *((whisper_print_user_data *) user_data)->transcriber;
            transcriber.whisper_print_progress_callback(ctx, state, progress, user_data);
        };
        wparams.progress_callback_user_data = &user_data;
    }

    wparams.abort_callback = [](void * user_data) {
        qInfo("Abort callback");
        const auto & is_aborted = *((whisper_print_user_data *) user_data)->is_aborted;
        if(is_aborted) {
            auto & transcriber  = *((whisper_print_user_data *) user_data)->transcriber;
            transcriber.transcriptionFinished(true);
        }
        return is_aborted.load();
    };
    wparams.abort_callback_user_data = &user_data;

    qInfo("Starting transcribe");
    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        emit statusUpdated("Failed to process audio");
        whisper_free(ctx);
        return;
    }
    qInfo("Transcribe finished");

    const auto fname_jsn = outputFile.toStdString();
    output_json(ctx, fname_jsn.c_str(), params, pcmf32s, params.output_jsn_full);

    whisper_free(ctx);

    emit progressUpdated(100);
    emit statusUpdated("Completed");
    emit totalProgressUpdated(100); // Emitting the total progress
}

void Transcriber::whisper_print_progress_callback(struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, int progress, void * user_data) {
    qInfo("whisper_print_progress_callback");
    int progress_step = ((whisper_print_user_data *) user_data)->params->progress_step;
    int * progress_prev  = &(((whisper_print_user_data *) user_data)->progress_prev);
    qInfo("progress: %d - progress_prev: %d - step: %d", progress, *progress_prev, progress_step);
    if (progress >= *progress_prev + progress_step) {
        *progress_prev += progress_step;
        qInfo("progress: %d", progress);
        emit progressUpdated(progress);
        emit totalProgressUpdated(progress); // Emitting the total progress
    }
}

void Transcriber::whisper_print_segment_callback(struct whisper_context * ctx, struct whisper_state * /*state*/, int n_new, void * user_data) {
    qInfo("whisper_print_segment_callback");
    const auto & params  = *((whisper_print_user_data *) user_data)->params;
    const int n_segments = whisper_full_n_segments(ctx);

    std::string speaker = "";

    int64_t t0 = 0;
    int64_t t1 = 0;

    const int s0 = n_segments - n_new;

    if (s0 == 0) {
        printf("\n");
    }

    for (int i = s0; i < n_segments; i++) {
        if (!params.no_timestamps || params.diarize) {
            t0 = whisper_full_get_segment_t0(ctx, i);
            t1 = whisper_full_get_segment_t1(ctx, i);
        }

        if (!params.no_timestamps) {
            printf("[%s --> %s]  ", to_timestamp(t0).c_str(), to_timestamp(t1).c_str());
        }

        const char * text = whisper_full_get_segment_text(ctx, i);

        printf("%s%s", speaker.c_str(), text);

        if (!params.no_timestamps) {
            printf("\n");
        }

        fflush(stdout);
    }
}

char* escape_double_quotes(const char* input) {
    size_t length = strlen(input);
    size_t new_length = length;

    // Calcola la lunghezza del nuovo array di caratteri con l'escape
    for (size_t i = 0; i < length; ++i) {
        if (input[i] == '"') {
            new_length++;
        }
    }

    // Alloca memoria per il nuovo array di caratteri
    char* output = (char*)malloc(new_length + 1);
    if (!output) {
        return nullptr; // Gestione dell'errore di allocazione
    }

    size_t j = 0;
    for (size_t i = 0; i < length; ++i) {
        if (input[i] == '"') {
            output[j++] = '\\';
        }
        output[j++] = input[i];
    }
    output[new_length] = '\0';

    return output;
}

char* remove_double_quotes(const char* input) {
    size_t length = strlen(input);
    size_t new_length = length;

    // Calcola la lunghezza del nuovo array di caratteri senza le virgolette doppie
    for (size_t i = 0; i < length; ++i) {
        if (input[i] == '"') {
            new_length--;
        }
    }

    // Alloca memoria per il nuovo array di caratteri
    char* output = (char*)malloc(new_length + 1);
    if (!output) {
        return nullptr; // Gestione dell'errore di allocazione
    }

    size_t j = 0;
    for (size_t i = 0; i < length; ++i) {
        if (input[i] != '"') {
            output[j++] = input[i];
        }
    }
    output[new_length] = '\0';

    return output;
}

void Transcriber::setVideoInfo(const QString &title, const QString &link) {
    this->videoTitle = title;
    this->videoHrefLink = link;
}

// Funzione per ottenere il timestamp corrente in millisecondi
int64_t Transcriber::get_current_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    return epoch.count();
}


bool Transcriber::output_json(
    struct whisper_context * ctx,
    const char * fname,
    const whisper_params & params,
    std::vector<std::vector<float>> pcmf32s,
    bool full) {
    std::ofstream fout(fname);
    int indent = 0;

    std::ostringstream videoTextStream;

    auto doindent = [&]() {
        for (int i = 0; i < indent; i++) fout << "\t";
    };

    auto start_arr = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": [\n";
        indent++;
    };

    auto end_arr = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "]\n" : "],\n");
    };

    auto start_obj = [&](const char *name) {
        doindent();
        if (name) {
            fout << "\"" << name << "\": {\n";
        } else {
            fout << "{\n";
        }
        indent++;
    };

    auto end_obj = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "}\n" : "},\n");
    };

    auto start_value = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": ";
    };

    auto value_s = [&](const char *name, const char *val, bool end) {
        start_value(name);
        char * val_no_quotes = remove_double_quotes(val);
        fout << "\"" << val_no_quotes << (end ? "\"\n" : "\",\n");
        free(val_no_quotes);
    };

    auto end_value = [&](bool end) {
        fout << (end ? "\n" : ",\n");
    };

    auto value_i = [&](const char *name, const int64_t val, bool end) {
        start_value(name);
        fout << val;
        end_value(end);
    };

    auto value_f = [&](const char *name, const float val, bool end) {
        start_value(name);
        fout << val;
        end_value(end);
    };

    auto value_b = [&](const char *name, const bool val, bool end) {
        start_value(name);
        fout << (val ? "true" : "false");
        end_value(end);
    };

    auto times_o = [&](int64_t t0, int64_t t1, bool end) {
        start_obj("timestamps");
        value_s("from", to_timestamp(t0, true).c_str(), false);
        value_s("to", to_timestamp(t1, true).c_str(), true);
        end_obj(false);
        start_obj("offsets");
        value_i("from", t0 * 10, false);
        value_i("to", t1 * 10, true);
        end_obj(end);
    };

    if (!fout.is_open()) {
        fprintf(stderr, "%s: failed to open '%s' for writing\n", __func__, fname);
        return false;
    }

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);
    start_obj(nullptr);
    value_s("systeminfo", whisper_print_system_info(), false);
    start_obj("model");
    value_s("type", whisper_model_type_readable(ctx), false);
    value_b("multilingual", whisper_is_multilingual(ctx), false);
    value_i("vocab", whisper_model_n_vocab(ctx), false);
    start_obj("audio");
    value_i("ctx", whisper_model_n_audio_ctx(ctx), false);
    value_i("state", whisper_model_n_audio_state(ctx), false);
    value_i("head", whisper_model_n_audio_head(ctx), false);
    value_i("layer", whisper_model_n_audio_layer(ctx), true);
    end_obj(false);
    start_obj("text");
    value_i("ctx", whisper_model_n_text_ctx(ctx), false);
    value_i("state", whisper_model_n_text_state(ctx), false);
    value_i("head", whisper_model_n_text_head(ctx), false);
    value_i("layer", whisper_model_n_text_layer(ctx), true);
    end_obj(false);
    value_i("mels", whisper_model_n_mels(ctx), false);
    value_i("ftype", whisper_model_ftype(ctx), true);
    end_obj(false);
    start_obj("params");
    value_s("model", params.model.c_str(), false);
    value_s("language", params.language.c_str(), false);
    value_b("translate", params.translate, true);
    end_obj(false);
    start_obj("result");
    value_s("language", whisper_lang_str(whisper_full_lang_id(ctx)), true);
    end_obj(false);
    start_arr("transcription");

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);

        // Rimuovi le virgolette doppie nella variabile text
        char* no_quotes_text = remove_double_quotes(text);

        videoTextStream << no_quotes_text << " ";

        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        start_obj(nullptr);
        times_o(t0, t1, false);
        value_s("text", no_quotes_text, !params.diarize && !params.tinydiarize && !full);

        free(no_quotes_text); // Libera la memoria allocata per no_quotes_text

        if (full) {
            start_arr("tokens");
            const int n = whisper_full_n_tokens(ctx, i);
            for (int j = 0; j < n; ++j) {
                auto token = whisper_full_get_token_data(ctx, i, j);
                start_obj(nullptr);
                char* token_text = remove_double_quotes(whisper_token_to_str(ctx, token.id));
                value_s("text", token_text, false);
                free(token_text);
                if (token.t0 > -1 && token.t1 > -1) {
                    times_o(token.t0, token.t1, false);
                }
                value_i("id", token.id, false);
                value_f("p", token.p, false);
                value_f("t_dtw", token.t_dtw, true);
                end_obj(j == (n - 1));
            }
            end_arr(!params.diarize && !params.tinydiarize);
        }

        end_obj(i == (n_segments - 1));
    }

    end_arr(false);

    // Aggiungi videoTitle e videoHrefLink
    start_value("videoTitle");
    fout << "\"" << videoTitle.toStdString() << "\",\n";
    start_value("videoHrefLink");
    fout << "\"" << videoHrefLink.toStdString() << "\",\n";

    start_value("videoText");
    fout << "\"" << remove_double_quotes(videoTextStream.str().c_str()) << "\",\n";

    // Aggiungi il timestamp
    start_value("timestamp");
    fout << "\"" << get_current_timestamp_ms() << "\"\n";

    end_obj(true);
    return true;
}


