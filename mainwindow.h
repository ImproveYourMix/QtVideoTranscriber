#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "transcriber.h"
#include "transcriptionqueuemanager.h"
#include "transcriptionmodel.h"

#include <QMainWindow>
#include <QStringList>
#include <QStandardItemModel>
#include <QLabel>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    Ui::MainWindow *ui;
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

#endif // MAINWINDOW_H
