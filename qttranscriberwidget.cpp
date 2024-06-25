#include "qttranscriberwidget.h"
#include "ui_qttranscriberwidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>


QtTranscriberWidget::QtTranscriberWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::QtTranscriberWidget), model(new TranscriptionModel(this)), threadQueueManager(new TranscriptionQueueManager(this)) {
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &QtTranscriberWidget::selectFiles);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &QtTranscriberWidget::selectOutputFolder);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &QtTranscriberWidget::transcribeFiles);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &QtTranscriberWidget::stopCurrentTranscription);

    model->setColumnCount(5); // Aggiungere colonne per il titolo e il link
    model->setHorizontalHeaderLabels(QStringList() << "File" << "Title" << "Link" << "Progress" << "Status");
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->progressBar->setValue(0);

    elapsedTimeLabel = new QLabel(this);
    elapsedTimeLabel->setText("Total Elapsed: 00:00:00");
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QtTranscriberWidget::updateElapsedTime);
    ui->statusbar->addPermanentWidget(elapsedTimeLabel);
    ui->pushButton_3->setDisabled(true);
    ui->pushButton_4->setDisabled(true);

    connect(threadQueueManager, &TranscriptionQueueManager::allThreadsFinished, this, &QtTranscriberWidget::onAllThreadsFinished);
    connect(threadQueueManager, &TranscriptionQueueManager::progressUpdated, this, &QtTranscriberWidget::onProgressUpdated);
    connect(threadQueueManager, &TranscriptionQueueManager::statusUpdated, this, &QtTranscriberWidget::onStatusUpdated);
    connect(threadQueueManager, &TranscriptionQueueManager::progressUpdated, this, &QtTranscriberWidget::updateTotalProgress);
}

QtTranscriberWidget::~QtTranscriberWidget() {
    delete ui;
}

void QtTranscriberWidget::selectFiles() {
    selectedFiles = QFileDialog::getOpenFileNames(this, tr("Select Audio or Video Files"), "", tr("Media Files (*.wav *.mp4)"));
    model->setRowCount(selectedFiles.size());

    for (int i = 0; i < selectedFiles.size(); ++i) {
        QStandardItem *fileItem = new QStandardItem(selectedFiles.at(i));
        QStandardItem *titleItem = new QStandardItem("");
        QStandardItem *linkItem = new QStandardItem("");
        QStandardItem *progressItem = new QStandardItem("0%");
        QStandardItem *statusItem = new QStandardItem("Waiting");

        model->setItem(i, 0, fileItem);
        model->setItem(i, 1, titleItem);
        model->setItem(i, 2, linkItem);
        model->setItem(i, 3, progressItem);
        model->setItem(i, 4, statusItem);

        progressMap[i] = 0; // Initialize progress map
    }

    if (!outputFolder.isEmpty() && !selectedFiles.isEmpty()) {
        ui->pushButton_3->setDisabled(false);
    }
}

void QtTranscriberWidget::selectOutputFolder() {
    outputFolder = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), "");
    if (!outputFolder.isEmpty()) {
        ui->pushButton_2->setText(outputFolder);
        if (!selectedFiles.isEmpty()) {
            ui->pushButton_3->setDisabled(false);
        }
    }
}

void QtTranscriberWidget::transcribeFiles() {
    if (isTranscribing) {
        stopTranscription();
        return;
    }

    if (selectedFiles.isEmpty() || outputFolder.isEmpty()) {
        return;
    }

    startTime = QTime::currentTime();
    timer->start(1000);

    for (int i = 0; i < selectedFiles.size(); ++i) {
        // Ottieni titolo e link dalla tabella
        QString title = model->item(i, 1)->text();
        QString link = model->item(i, 2)->text();
        threadQueueManager->addTranscription(selectedFiles.at(i), outputFolder, i, title, link);
    }

    threadQueueManager->start();

    ui->pushButton_3->setText("Stop All");
    ui->pushButton_4->setDisabled(false);
    isTranscribing = true;
}

void QtTranscriberWidget::updateElapsedTime() {
    int elapsed = startTime.secsTo(QTime::currentTime());
    QTime elapsedTime((elapsed / 3600), (elapsed / 60) % 60, elapsed % 60);
    elapsedTimeLabel->setText("Total Elapsed: " + elapsedTime.toString("hh:mm:ss"));
}

void QtTranscriberWidget::stopTranscription() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Stop Transcription", "Are you sure you want to stop all transcriptions?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        threadQueueManager->stopAllThreads();
        ui->pushButton_3->setText("Transcribe");
        ui->pushButton_4->setDisabled(true);
        isTranscribing = false;
    }
}

void QtTranscriberWidget::stopCurrentTranscription() {
    threadQueueManager->stopCurrentThread();
}

void QtTranscriberWidget::onAllThreadsFinished() {
    timer->stop();
    ui->pushButton_3->setText("Transcribe");
    ui->pushButton_4->setDisabled(true);
    isTranscribing = false;
}

void QtTranscriberWidget::onProgressUpdated(int row, int progress) {
    model->item(row, 3)->setText(QString::number(progress) + "%");
    progressMap[row] = progress; // Update progress map
}

void QtTranscriberWidget::onStatusUpdated(int row, const QString &status) {
    model->item(row, 4)->setText(status);
}

void QtTranscriberWidget::updateTotalProgress() {
    int totalProgress = 0;
    for (auto progress : progressMap.values()) {
        totalProgress += progress;
    }
    int total = progressMap.size() * 100;
    int overallProgress = (totalProgress * 100) / total;
    ui->progressBar->setValue(overallProgress);
}
