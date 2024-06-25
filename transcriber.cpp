#include "transcriber.h"
#include "dr_wav.h"
#include "common.h"

Transcriber::Transcriber(std::atomic<bool>* abortFlag, QObject *parent) :
    QObject(parent), abortFlag(abortFlag) {}

void Transcriber::setFileAndOutput(const QString &file, const QString &outputFolder) {
    this->file = file;
    this->outputFolder = outputFolder;
}

void Transcriber::setVideoInfo(const QString &title, const QString &link) {
    this->videoTitle = title;
    this->videoHrefLink = link;
}

void Transcriber::startTranscription() {
    // Implementation of the transcription process
}

void Transcriber::abortTranscription() {
    // Implementation of the abort transcription process
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
        char * val_escaped = escape_double_quotes(val);
        fout << "\"" << val_escaped << (end ? "\"\n" : "\",\n");
        free(val_escaped);
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
        
        // Esegui l'escape delle doppie virgolette nella variabile text
        char* escaped_text = escape_double_quotes(text);
        
        videoTextStream << escaped_text << " ";

        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        start_obj(nullptr);
        times_o(t0, t1, false);
        value_s("text", escaped_text, !params.diarize && !params.tinydiarize && !full);

        free(escaped_text); // Libera la memoria allocata per escaped_text

        if (full) {
            start_arr("tokens");
            const int n = whisper_full_n_tokens(ctx, i);
            for (int j = 0; j < n; ++j) {
                auto token = whisper_full_get_token_data(ctx, i, j);
                start_obj(nullptr);
                char* token_text = escape_double_quotes(whisper_token_to_str(ctx, token.id));
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

    end_arr(true);

    // Aggiungi videoTitle e videoHrefLink
    start_value("videoTitle");
    fout << "\"" << videoTitle.toStdString() << "\",\n";
    start_value("videoHrefLink");
    fout << "\"" << videoHrefLink.toStdString() << "\",\n";

    start_value("videoText");
    fout << "\"" << escape_double_quotes(videoTextStream.str().c_str()) << "\"\n";

    end_obj(true);
    return true;
}
