#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_isRamLoaded(false)
{
    ui.setupUi(this);

    this->setWindowIcon(QIcon(":/CRC32CGUI/app_icon.ico"));

    ui.comboBoxAlgo->addItem("C++ Bit-wise (Very Slow)", 0);
    ui.comboBoxAlgo->addItem("C++ Slicing-by-1 (Standard)", 1);
    ui.comboBoxAlgo->addItem("C++ Slicing-by-8 (High Performance)", 2);
    ui.comboBoxAlgo->addItem("ASM (Hardware CRC32)", 3);
    ui.comboBoxAlgo->addItem("ASM (Hardware CRC32 with pipelaning)", 4);

    QList<int> threadCounts = { 1, 2, 4, 8, 16, 32, 64 };
    for (int count : threadCounts) {
        ui.comboBoxThreads->addItem(QString::number(count), count);
    }

    ui.progressBar->setValue(0);

    setupMemoryControls();
    setupRamControl();

    connect(ui.comboBoxAlgo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::on_comboBoxAlgo_currentIndexChanged);

    connect(&m_watcher, &QFutureWatcher<CrcCalculator::Result>::finished, this, &MainWindow::on_calculationFinished);

    on_comboBoxAlgo_currentIndexChanged(ui.comboBoxAlgo->currentIndex());

    applyProfessionalStyle();
}

MainWindow::~MainWindow()
{
    m_watcher.waitForFinished();
}

void MainWindow::setupMemoryControls()
{
    labelMemoryMode = new QLabel("Memory Mode:", ui.groupBoxSettings);
    labelMemoryMode->setObjectName("labelMemoryMode");

    comboBoxMemoryMode = new QComboBox(ui.groupBoxSettings);
    comboBoxMemoryMode->setCursor(Qt::PointingHandCursor);

    comboBoxMemoryMode->addItem("Load Entire File (Fastest - High RAM)", -1);
    comboBoxMemoryMode->addItem("Chunk: 64 MB (Balanced)", 64 * 1024 * 1024);
    comboBoxMemoryMode->addItem("Chunk: 16 MB", 16 * 1024 * 1024);
    comboBoxMemoryMode->addItem("Chunk: 4 MB (Low RAM)", 4 * 1024 * 1024);

    ui.gridLayoutSettings->addWidget(labelMemoryMode, 1, 0);
    ui.gridLayoutSettings->addWidget(comboBoxMemoryMode, 1, 1, 1, 3);
}

void MainWindow::setupRamControl()
{
    pushBtnLoadRam = new QPushButton("Preload to RAM", ui.groupBoxFile);
    pushBtnLoadRam->setObjectName("pushBtnLoadRam");
    pushBtnLoadRam->setCursor(Qt::PointingHandCursor);
    pushBtnLoadRam->setToolTip("Loads the file into memory to measure pure algorithm speed without disk I/O.");

    ui.horizontalLayoutFile->addWidget(pushBtnLoadRam);

    connect(pushBtnLoadRam, &QPushButton::clicked, this, &MainWindow::on_pushBtnLoadRam_clicked);
}

void MainWindow::on_comboBoxAlgo_currentIndexChanged(int index)
{
    if (index == 0) {
        ui.comboBoxThreads->setCurrentIndex(0);
        ui.comboBoxThreads->setEnabled(false);
    }
    else {
        ui.comboBoxThreads->setEnabled(true);
    }
}

void MainWindow::toggleControls(bool enabled)
{
    ui.pushBtnCalculate->setEnabled(enabled);
    ui.comboBoxAlgo->setEnabled(enabled);
    ui.comboBoxThreads->setEnabled(enabled && ui.comboBoxAlgo->currentIndex() != 0);

    if (!m_isRamLoaded) {
        ui.pushBtnBrowse->setEnabled(enabled);
        pushBtnLoadRam->setEnabled(enabled);
        comboBoxMemoryMode->setEnabled(enabled);
    }
}

void MainWindow::on_pushBtnCalculate_clicked()
{
    if (!m_isRamLoaded && ui.lineEditFilePath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "No file selected!");
        return;
    }

    CrcCalculator::Settings settings;
    settings.algorithm = static_cast<CrcCalculator::Algorithm>(ui.comboBoxAlgo->currentData().toInt());
    settings.threadCount = ui.comboBoxThreads->currentData().toInt();

    toggleControls(false);
    ui.progressBar->setValue(0);
    ui.labelStats->setText("Calculation in progress...");

    auto progressCallback = [this](int progress) {
        QMetaObject::invokeMethod(ui.progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
        };

    QFuture<CrcCalculator::Result> future;
    if (m_isRamLoaded) {
        future = QtConcurrent::run([this, settings, progressCallback]() {
            return m_calculator.calculateFromBuffer(m_ramBuffer, settings, progressCallback);
            });
    }
    else {
        QString filePath = ui.lineEditFilePath->text();
        qint64 chunkSize = comboBoxMemoryMode->currentData().toLongLong();
        future = QtConcurrent::run([this, filePath, settings, chunkSize, progressCallback]() {
            return m_calculator.calculateFromFile(filePath, settings, chunkSize, progressCallback);
            });
    }

    m_watcher.setFuture(future);
}

void MainWindow::on_calculationFinished()
{
    CrcCalculator::Result res = m_watcher.result();

    QString resultHex = QString("%1").arg(res.crc, 8, 16, QChar('0')).toUpper();
    ui.lineEditResult->setText(resultHex);

    qint64 fileSize = m_isRamLoaded ? m_ramBuffer.size() : QFileInfo(ui.lineEditFilePath->text()).size();

    ui.labelStats->setText(QString("Time: %1 | Size: %2")
        .arg(formatTimeElapsed(res.nanoseconds))
        .arg(formatFileSize(fileSize)));

    toggleControls(true);
}

void MainWindow::on_pushBtnLoadRam_clicked()
{
    if (m_isRamLoaded) {
        m_ramBuffer.clear();
        m_ramBuffer.squeeze();
        m_isRamLoaded = false;
        pushBtnLoadRam->setText("Preload to RAM");
        ui.pushBtnBrowse->setEnabled(true);
        ui.lineEditFilePath->setEnabled(true);
        comboBoxMemoryMode->setEnabled(true);
        ui.labelStats->setText("RAM cleared.");
    }
    else {
        QString filePath = ui.lineEditFilePath->text();
        if (filePath.isEmpty()) {
            QMessageBox::warning(this, "Error", "Select a file first!");
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", "Cannot open file!");
            return;
        }

        ui.labelStats->setText("Loading into RAM...");
        QApplication::processEvents();

        m_ramBuffer = file.readAll();
        file.close();

        if (m_ramBuffer.isEmpty() && file.size() > 0) {
            QMessageBox::critical(this, "Error", "Out of Memory");
            return;
        }

        m_isRamLoaded = true;
        pushBtnLoadRam->setText("Unload from RAM");
        ui.pushBtnBrowse->setEnabled(false);
        ui.lineEditFilePath->setEnabled(false);
        comboBoxMemoryMode->setEnabled(false);
        ui.labelStats->setText(QString("Loaded %1 into RAM.").arg(formatFileSize(m_ramBuffer.size())));
    }
    applyProfessionalStyle();
}

QString MainWindow::formatTimeElapsed(qint64 nanoseconds)
{
    if (nanoseconds < 1000) return QString("%1 ns").arg(nanoseconds);
    double value = nanoseconds / 1000.0;
    if (value < 1000.0) return QString("%1 \u00B5s").arg(value, 0, 'f', 2);
    value /= 1000.0;
    if (value < 1000.0) return QString("%1 ms").arg(value, 0, 'f', 2);
    value /= 1000.0;
    return QString("%1 s").arg(value, 0, 'f', 3);
}

void MainWindow::applyProfessionalStyle()
{
    QString baseStyle = R"(
        QMainWindow { background-color: #2b2b2b; }
        QWidget { color: #e0e0e0; font-family: 'Segoe UI', 'Roboto', sans-serif; font-size: 14px; }
        QGroupBox { border: 1px solid #444; border-radius: 6px; margin-top: 20px; font-weight: bold; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: #00aaff; }
        QPushButton { background-color: #3a3a3a; border: 1px solid #555; border-radius: 4px; padding: 6px 12px; color: #ffffff; }
        QPushButton:hover { background-color: #4a4a4a; border-color: #00aaff; }
        QPushButton#pushBtnCalculate { background-color: #007acc; border: none; font-weight: bold; font-size: 16px; }
        QPushButton#pushBtnCalculate:hover { background-color: #0099ff; }
        QPushButton#pushBtnCalculate:disabled { background-color: #555; color: #888; }
        QLineEdit, QComboBox { background-color: #1e1e1e; border: 1px solid #444; border-radius: 4px; padding: 4px; color: #ffffff; }
        QProgressBar { border: 1px solid #444; border-radius: 4px; text-align: center; background-color: #1e1e1e; }
        QProgressBar::chunk { background-color: #00aaff; width: 10px; }
        QLabel#labelTitle { color: #ffffff; font-size: 24px; font-weight: bold; }
        QLabel#labelStats { color: #aaaaaa; font-size: 12px; }
        QComboBox QAbstractItemView { background-color: #2b2b2b; color: #ffffff; selection-background-color: #007acc; }
    )";

    QString ramButtonStyle = m_isRamLoaded ?
        "QPushButton#pushBtnLoadRam { background-color: #a32a2a; border: 1px solid #d44; }" :
        "QPushButton#pushBtnLoadRam { background-color: #5a3a7a; border: 1px solid #7a5a9a; }";

    this->setStyleSheet(baseStyle + ramButtonStyle);
}

void MainWindow::on_pushBtnHelp_clicked()
{
    QString helpText = "<h2>CRC32C Performance Calculator</h2><p>Multi-threaded high-performance tool.</p>";
    QMessageBox::information(this, "User Guide", helpText);
}

void MainWindow::on_pushBtnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file", "", "All files (*.*)");
    if (fileName.isEmpty()) return;
    ui.lineEditFilePath->setText(fileName);
    QFileInfo fileInfo(fileName);
    ui.labelStats->setText(QString("Size: %1").arg(formatFileSize(fileInfo.size())));
}

QString MainWindow::formatFileSize(qint64 size)
{
    QStringList units = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double readableSize = size;
    while (readableSize >= 1024.0 && unitIndex < units.size() - 1) {
        readableSize /= 1024.0;
        unitIndex++;
    }
    return QString::number(readableSize, 'f', 2) + " " + units[unitIndex];
}