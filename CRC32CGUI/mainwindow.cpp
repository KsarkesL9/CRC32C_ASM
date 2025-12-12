#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_isRamLoaded(false)
{
    ui.setupUi(this);

    this->setWindowIcon(QIcon(":/CRC32CGUI/app_icon.ico"));

    ui.comboBoxAlgo->addItem("C++ Bit-wise (Very Slow)", 0);
    ui.comboBoxAlgo->addItem("C++ Slicing-by-1 (Standard)", 1);
    ui.comboBoxAlgo->addItem("C++ Slicing-by-8 (High Performance)", 2);
    ui.comboBoxAlgo->addItem("ASM (Not connected)", 3);

    QList<int> threadCounts = { 1, 2, 4, 8, 16, 32, 64 };
    for (int count : threadCounts) {
        ui.comboBoxThreads->addItem(QString::number(count), count);
    }

    ui.progressBar->setValue(0);

    setupMemoryControls();
    setupRamControl();

    connect(ui.comboBoxAlgo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::on_comboBoxAlgo_currentIndexChanged);

    on_comboBoxAlgo_currentIndexChanged(ui.comboBoxAlgo->currentIndex());

    applyProfessionalStyle();
}

MainWindow::~MainWindow()
{
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
        ui.comboBoxThreads->setToolTip("Multi-threading disabled for Bit-wise algorithm.");
    }
    else {
        ui.comboBoxThreads->setEnabled(true);
        ui.comboBoxThreads->setToolTip("");
    }
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

        ui.labelStats->setText("Loading into RAM... please wait.");
        QApplication::processEvents();

        m_ramBuffer = file.readAll();
        file.close();

        if (m_ramBuffer.isEmpty() && file.size() > 0) {
            QMessageBox::critical(this, "Error", "Failed to load file to RAM (Out of Memory?)");
            return;
        }

        m_isRamLoaded = true;

        pushBtnLoadRam->setText("Unload from RAM");
        ui.pushBtnBrowse->setEnabled(false);
        ui.lineEditFilePath->setEnabled(false);
        comboBoxMemoryMode->setEnabled(false);

        ui.labelStats->setText(QString("Loaded %1 into RAM. Ready to calculate.").arg(formatFileSize(m_ramBuffer.size())));
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
        QPushButton:pressed { background-color: #2a2a2a; }
        
        QPushButton#pushBtnCalculate { background-color: #007acc; border: none; font-weight: bold; font-size: 16px; }
        QPushButton#pushBtnCalculate:hover { background-color: #0099ff; }
        QPushButton#pushBtnCalculate:disabled { background-color: #555; color: #888; }

        QLineEdit, QComboBox { background-color: #1e1e1e; border: 1px solid #444; border-radius: 4px; padding: 4px; color: #ffffff; }
        QLineEdit:read-only { background-color: #252525; color: #aaa; }
        QLineEdit:disabled { color: #666; background-color: #1a1a1a; }
        
        QProgressBar { border: 1px solid #444; border-radius: 4px; text-align: center; background-color: #1e1e1e; }
        QProgressBar::chunk { background-color: #00aaff; width: 10px; }
        
        QLabel#labelTitle { color: #ffffff; font-size: 24px; font-weight: bold; }
        QLabel#labelStats { color: #aaaaaa; font-size: 12px; }
        
        QComboBox::drop-down { border: 0px; }
        QComboBox QAbstractItemView { background-color: #2b2b2b; color: #ffffff; selection-background-color: #007acc; }
    )";

    QString ramButtonStyle;
    if (m_isRamLoaded) {
        ramButtonStyle = R"(
            QPushButton#pushBtnLoadRam { background-color: #a32a2a; border: 1px solid #d44; }
            QPushButton#pushBtnLoadRam:hover { background-color: #c93030; }
        )";
    }
    else {
        ramButtonStyle = R"(
            QPushButton#pushBtnLoadRam { background-color: #5a3a7a; border: 1px solid #7a5a9a; }
            QPushButton#pushBtnLoadRam:hover { background-color: #6a4a8a; }
        )";
    }

    this->setStyleSheet(baseStyle + ramButtonStyle);
}

void MainWindow::on_pushBtnHelp_clicked()
{
    QString helpText = R"(
        <h2 style="color: #007acc;">CRC32C Performance Calculator</h2>
        <p>This application allows high-performance CRC32C checksum calculation using various software algorithms, multi-threading, and memory management techniques.</p>
        <hr>

        <h3 style="color: #00aaff;">1. Algorithms</h3>
        <ul>
            <li><b>C++ Bit-wise:</b> Reference implementation. Processes data bit-by-bit. Extremely slow, intended for educational comparison. <i>(Single-threaded only)</i>.</li>
            <li><b>C++ Slicing-by-1:</b> Standard implementation using a single lookup table (256 entries).</li>
            <li><b>C++ Slicing-by-8:</b> High-performance implementation processing 8 bytes at a time using 8 parallel lookup tables. <b>Recommended.</b></li>
        </ul>

        <h3 style="color: #00aaff;">2. Multi-threading</h3>
        <p>You can select up to 64 threads. The data is split into logical chunks and processed in parallel using <code>std::async</code>.</p>
        <p>Partial results from threads are combined mathematically using <b>GF(2) matrix exponentiation</b> (shift-and-multiply) to ensure the final checksum is identical to a single-threaded run.</p>
        <p><i>Note: Multi-threading is disabled for the Bit-wise algorithm due to performance constraints.</i></p>

        <h3 style="color: #00aaff;">3. Memory Modes & Disk I/O</h3>
        <p>Configure how files are read from the disk to balance RAM usage vs. speed:</p>
        <ul>
            <li><b>Load Entire File:</b> Reads the whole file into a buffer. Fastest for I/O, but uses RAM proportional to file size.</li>
            <li><b>Chunk (64MB - 4MB):</b> Reads the file sequentially in blocks. Safe for huge files on systems with low RAM.</li>
        </ul>

        <h3 style="color: #00aaff;">4. Benchmarking (Preload to RAM)</h3>
        <p>Use the <b>Preload to RAM</b> button to load the file into memory <i>before</i> starting the timer.</p>
        <ul>
            <li><b>When active:</b> The timer measures pure Algorithm + CPU performance (Disk I/O is excluded).</li>
            <li><b>When inactive:</b> The timer includes Disk reading speed + Calculation time.</li>
        </ul>
    )";
    QMessageBox::information(this, "User Guide", helpText);
}

void MainWindow::on_pushBtnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file", "", "All files (*.*)");
    if (fileName.isEmpty()) return;

    ui.lineEditFilePath->setText(fileName);
    QFileInfo fileInfo(fileName);
    ui.labelStats->setText(QString("Time: - | Size: %1").arg(formatFileSize(fileInfo.size())));
    ui.lineEditResult->clear();
    ui.progressBar->setValue(0);
}


uint32_t MainWindow::calculateChunk(const uint8_t* data, size_t length, int algoIndex)
{
    uint32_t crc = 0; 

    if (algoIndex == 0) {
        crc = CppCrc32cUpdateBitwise(crc, data, length);
    }
    else if (algoIndex == 1) {
        crc = CppCrc32cUpdateSlicing1(crc, data, length);
    }
    else {
        crc = CppCrc32cUpdateSlicing8(crc, data, length);
    }
    return crc;
}

void MainWindow::on_pushBtnCalculate_clicked()
{
    if (!m_isRamLoaded && ui.lineEditFilePath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "No file selected!");
        return;
    }

    int algoIndex = ui.comboBoxAlgo->currentData().toInt();
    int threadCount = ui.comboBoxThreads->currentData().toInt();

    if (algoIndex == 0) threadCount = 1;

    ui.pushBtnCalculate->setEnabled(false);
    ui.comboBoxAlgo->setEnabled(false);
    ui.comboBoxThreads->setEnabled(false);

    if (!m_isRamLoaded) {
        ui.pushBtnBrowse->setEnabled(false);
        pushBtnLoadRam->setEnabled(false);
        comboBoxMemoryMode->setEnabled(false);
    }

    ui.progressBar->setValue(0);
    ui.labelStats->setText("Calculation in progress...");
    QApplication::processEvents();

    uint32_t finalCrc = 0;

    qint64 fileSize = 0;
    if (m_isRamLoaded) fileSize = m_ramBuffer.size();
    else fileSize = QFileInfo(ui.lineEditFilePath->text()).size();

    QElapsedTimer timer;
    timer.start();

    auto processBufferParallel = [&](const uint8_t* buffer, size_t totalSize) -> uint32_t {
        if (threadCount == 1) {
            uint32_t crc = CppCrc32cInit();
            if (algoIndex == 0) crc = CppCrc32cUpdateBitwise(crc, buffer, totalSize);
            else if (algoIndex == 1) crc = CppCrc32cUpdateSlicing1(crc, buffer, totalSize);
            else crc = CppCrc32cUpdateSlicing8(crc, buffer, totalSize);
            return CppCrc32cFinalize(crc);
        }

        std::vector<std::future<uint32_t>> futures;
        size_t chunkSize = totalSize / threadCount;
        size_t remainder = totalSize % threadCount;
        size_t currentOffset = 0;

        for (int i = 0; i < threadCount; ++i) {
            size_t size = chunkSize + (i < remainder ? 1 : 0);
            if (size == 0) continue;

            const uint8_t* chunkPtr = buffer + currentOffset;

            futures.push_back(std::async(std::launch::async, [=]() {
                return calculateChunk(chunkPtr, size, algoIndex);
                }));

            currentOffset += size;
        }

        uint32_t runningCrc = CppCrc32cInit(); 

        currentOffset = 0;
        chunkSize = totalSize / threadCount;
        remainder = totalSize % threadCount;

        for (int i = 0; i < threadCount; ++i) {
            size_t size = chunkSize + (i < remainder ? 1 : 0);
            if (size == 0) continue;

            uint32_t chunkCrc = futures[i].get();
            runningCrc = CppCrc32cCombine(runningCrc, chunkCrc, size);

            currentOffset += size;
        }

        return CppCrc32cFinalize(runningCrc);
        };

    if (m_isRamLoaded) {
        const uint8_t* rawData = reinterpret_cast<const uint8_t*>(m_ramBuffer.constData());
        finalCrc = processBufferParallel(rawData, m_ramBuffer.size());
        ui.progressBar->setValue(100);
    }
    else {
        QString filePath = ui.lineEditFilePath->text();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", "Cannot open file!");
            ui.pushBtnCalculate->setEnabled(true);
            ui.comboBoxAlgo->setEnabled(true);
            ui.comboBoxThreads->setEnabled(true);
            ui.pushBtnBrowse->setEnabled(true);
            pushBtnLoadRam->setEnabled(true);
            comboBoxMemoryMode->setEnabled(true);
            return;
        }

        qint64 chunkSize = comboBoxMemoryMode->currentData().toLongLong();

        if (chunkSize == -1) {
            QByteArray allData = file.readAll();
            finalCrc = processBufferParallel(reinterpret_cast<const uint8_t*>(allData.constData()), allData.size());
            ui.progressBar->setValue(100);
        }
        else {
 
            uint32_t runningCrc = CppCrc32cInit();
            qint64 bytesReadTotal = 0;

            while (!file.atEnd()) {
                QByteArray chunk = file.read(chunkSize);
                if (chunk.isEmpty()) break;

                const uint8_t* rawChunk = reinterpret_cast<const uint8_t*>(chunk.constData());

                if (threadCount == 1) {
                    if (algoIndex == 0) runningCrc = CppCrc32cUpdateBitwise(runningCrc, rawChunk, chunk.size());
                    else if (algoIndex == 1) runningCrc = CppCrc32cUpdateSlicing1(runningCrc, rawChunk, chunk.size());
                    else runningCrc = CppCrc32cUpdateSlicing8(runningCrc, rawChunk, chunk.size());
                }
                else {
                    std::vector<std::future<uint32_t>> futures;
                    size_t currentChunkSize = chunk.size();
                    size_t subChunkSize = currentChunkSize / threadCount;
                    size_t rem = currentChunkSize % threadCount;
                    size_t offset = 0;

                    for (int i = 0; i < threadCount; ++i) {
                        size_t sz = subChunkSize + (i < rem ? 1 : 0);
                        if (sz == 0) continue;
                        const uint8_t* ptr = rawChunk + offset;
                        futures.push_back(std::async(std::launch::async, [=]() {
                            return calculateChunk(ptr, sz, algoIndex);
                            }));
                        offset += sz;
                    }

                    offset = 0;
                    for (int i = 0; i < threadCount; ++i) {
                        size_t sz = subChunkSize + (i < rem ? 1 : 0);
                        if (sz == 0) continue;
                        uint32_t subRes = futures[i].get();
                        runningCrc = CppCrc32cCombine(runningCrc, subRes, sz);
                        offset += sz;
                    }
                }

                bytesReadTotal += chunk.size();
                if (fileSize > 0) {
                    ui.progressBar->setValue(static_cast<int>((bytesReadTotal * 100) / fileSize));
                }
                QApplication::processEvents();
            }
            finalCrc = CppCrc32cFinalize(runningCrc);
        }
        file.close();
    }

    qint64 nsecs = timer.nsecsElapsed();

    QString resultHex = QString("%1").arg(finalCrc, 8, 16, QChar('0')).toUpper();
    ui.lineEditResult->setText(resultHex);

    QString timeString = formatTimeElapsed(nsecs);
    ui.labelStats->setText(QString("Time: %1 | Size: %2")
        .arg(timeString)
        .arg(formatFileSize(fileSize)));

    ui.pushBtnCalculate->setEnabled(true);
    ui.comboBoxAlgo->setEnabled(true);

    on_comboBoxAlgo_currentIndexChanged(algoIndex);

    if (!m_isRamLoaded) {
        ui.pushBtnBrowse->setEnabled(true);
        pushBtnLoadRam->setEnabled(true);
        comboBoxMemoryMode->setEnabled(true);
    }
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