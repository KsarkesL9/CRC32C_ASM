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

    setupMemoryControls(); // Combo od chunków
    setupRamControl();     // Nowy przycisk Preload

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
    // Tworzymy przycisk i dodajemy go obok przycisku Browse
    pushBtnLoadRam = new QPushButton("Preload to RAM", ui.groupBoxFile);
    pushBtnLoadRam->setObjectName("pushBtnLoadRam"); // Dla CSS
    pushBtnLoadRam->setCursor(Qt::PointingHandCursor);
    pushBtnLoadRam->setToolTip("Loads the file into memory to measure pure algorithm speed without disk I/O.");

    // Dodajemy do layoutu, w którym jest Browse (horizontalLayoutFile)
    ui.horizontalLayoutFile->addWidget(pushBtnLoadRam);

    connect(pushBtnLoadRam, &QPushButton::clicked, this, &MainWindow::on_pushBtnLoadRam_clicked);
}

void MainWindow::on_pushBtnLoadRam_clicked()
{
    if (m_isRamLoaded) {
        // --- UNLOAD ---
        m_ramBuffer.clear();
        m_ramBuffer.squeeze(); // Zwolnij pamiêæ
        m_isRamLoaded = false;

        // Przywróæ UI
        pushBtnLoadRam->setText("Preload to RAM");
        ui.pushBtnBrowse->setEnabled(true);
        ui.lineEditFilePath->setEnabled(true);
        comboBoxMemoryMode->setEnabled(true);
        ui.labelStats->setText("RAM cleared.");
    }
    else {
        // --- LOAD ---
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

        // Wczytanie ca³oœci
        m_ramBuffer = file.readAll();
        file.close();

        if (m_ramBuffer.isEmpty() && file.size() > 0) {
            QMessageBox::critical(this, "Error", "Failed to load file to RAM (Out of Memory?)");
            return;
        }

        m_isRamLoaded = true;

        // Zablokuj zmianê pliku i trybu pamiêci
        pushBtnLoadRam->setText("Unload from RAM");
        ui.pushBtnBrowse->setEnabled(false);
        ui.lineEditFilePath->setEnabled(false);
        comboBoxMemoryMode->setEnabled(false); // Tryb chunkowania nie ma sensu, gdy dane s¹ w RAM

        ui.labelStats->setText(QString("Loaded %1 into RAM. Ready to calculate.").arg(formatFileSize(m_ramBuffer.size())));
    }

    // Odœwie¿ styl (¿eby przycisk zmieni³ kolor jeœli zdefiniujemy to w CSS)
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

    // Specyficzny styl dla przycisku RAM (zmienny w zale¿noœci od stanu)
    QString ramButtonStyle;
    if (m_isRamLoaded) {
        // Stan za³adowany: Czerwony/Ostrzegawczy (aby roz³adowaæ)
        ramButtonStyle = R"(
            QPushButton#pushBtnLoadRam { background-color: #a32a2a; border: 1px solid #d44; }
            QPushButton#pushBtnLoadRam:hover { background-color: #c93030; }
        )";
    }
    else {
        // Stan domyœlny: Fioletowy (dla odró¿nienia od Browse)
        ramButtonStyle = R"(
            QPushButton#pushBtnLoadRam { background-color: #5a3a7a; border: 1px solid #7a5a9a; }
            QPushButton#pushBtnLoadRam:hover { background-color: #6a4a8a; }
        )";
    }

    this->setStyleSheet(baseStyle + ramButtonStyle);
}

void MainWindow::on_pushBtnHelp_clicked()
{
    QString helpText = "<h3>RAM Mode</h3>"
        "<p><b>Preload to RAM</b> allows you to load the file into memory <i>before</i> starting the timer.</p>"
        "<p>This way, when you click <b>Start Calculation</b>, the time shown reflects <b>only</b> the CRC32C algorithm speed (CPU/RAM), excluding Disk I/O.</p>";
    QMessageBox::information(this, "Help / Info", helpText);
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

void MainWindow::on_pushBtnCalculate_clicked()
{
    // 1. Walidacja: Czy wybrano plik lub za³adowano go do RAM?
    if (!m_isRamLoaded && ui.lineEditFilePath->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "No file selected!");
        return;
    }

    // 2. Pobierz wybrany algorytm
    int algoIndex = ui.comboBoxAlgo->currentData().toInt();

    // 3. Zablokuj interfejs na czas obliczeñ
    ui.pushBtnCalculate->setEnabled(false);
    ui.comboBoxAlgo->setEnabled(false);

    // Jeœli pracujemy na pliku z dysku (nie RAM), blokujemy te¿ kontrolki wyboru pliku
    if (!m_isRamLoaded) {
        ui.pushBtnBrowse->setEnabled(false);
        pushBtnLoadRam->setEnabled(false);
        comboBoxMemoryMode->setEnabled(false);
    }

    ui.progressBar->setValue(0);
    ui.labelStats->setText("Calculation in progress...");
    QApplication::processEvents(); // Odœwie¿ UI przed startem pêtli

    // 4. Przygotowanie zmiennych
    uint32_t currentCrc = CppCrc32cInit(); // Inicjalizacja (0xFFFFFFFF)
    qint64 fileSize = 0;

    if (m_isRamLoaded) {
        fileSize = m_ramBuffer.size();
    }
    else {
        QFileInfo fi(ui.lineEditFilePath->text());
        fileSize = fi.size();
    }

    QElapsedTimer timer;
    timer.start(); // START POMIARU CZASU

    // ---------------------------------------------------------
    // SCENARIUSZ A: DANE W RAM (Benchmark samego CPU)
    // ---------------------------------------------------------
    if (m_isRamLoaded) {
        const uint8_t* rawData = reinterpret_cast<const uint8_t*>(m_ramBuffer.constData());
        size_t len = m_ramBuffer.size();

        if (algoIndex == 0) {
            currentCrc = CppCrc32cUpdateBitwise(currentCrc, rawData, len);
        }
        else if (algoIndex == 1) {
            currentCrc = CppCrc32cUpdateSlicing1(currentCrc, rawData, len);
        }
        else if (algoIndex == 2) {
            currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);
        }
        else {
            // ASM Fallback (na razie Slicing-8, dopóki nie pod³¹czymy ASM)
            currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);
        }
        ui.progressBar->setValue(100);
    }
    // ---------------------------------------------------------
    // SCENARIUSZ B: CZYTANIE Z DYSKU (I/O + CPU)
    // ---------------------------------------------------------
    else {
        QString filePath = ui.lineEditFilePath->text();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", "Cannot open file!");
            // Odblokuj UI w razie b³êdu
            ui.pushBtnCalculate->setEnabled(true);
            ui.comboBoxAlgo->setEnabled(true);
            ui.pushBtnBrowse->setEnabled(true);
            pushBtnLoadRam->setEnabled(true);
            comboBoxMemoryMode->setEnabled(true);
            return;
        }

        qint64 chunkSize = comboBoxMemoryMode->currentData().toLongLong();

        // --- Podscenariusz B1: Ca³y plik naraz (du¿e zu¿ycie RAM, szybkie I/O) ---
        if (chunkSize == -1) {
            QByteArray allData = file.readAll();
            const uint8_t* rawData = reinterpret_cast<const uint8_t*>(allData.constData());
            size_t len = allData.size();

            if (algoIndex == 0)      currentCrc = CppCrc32cUpdateBitwise(currentCrc, rawData, len);
            else if (algoIndex == 1) currentCrc = CppCrc32cUpdateSlicing1(currentCrc, rawData, len);
            else if (algoIndex == 2) currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);
            else                     currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);

            ui.progressBar->setValue(100);
        }
        // --- Podscenariusz B2: Czytanie porcjami (bezpieczne dla RAM) ---
        else {
            qint64 bytesReadTotal = 0;
            if (fileSize == 0) ui.progressBar->setValue(100);

            while (!file.atEnd()) {
                QByteArray chunk = file.read(chunkSize);
                if (chunk.isEmpty()) break;

                const uint8_t* rawData = reinterpret_cast<const uint8_t*>(chunk.constData());
                size_t len = chunk.size();

                if (algoIndex == 0)      currentCrc = CppCrc32cUpdateBitwise(currentCrc, rawData, len);
                else if (algoIndex == 1) currentCrc = CppCrc32cUpdateSlicing1(currentCrc, rawData, len);
                else if (algoIndex == 2) currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);
                else                     currentCrc = CppCrc32cUpdateSlicing8(currentCrc, rawData, len);

                bytesReadTotal += len;

                // Aktualizacja paska postêpu
                if (fileSize > 0) {
                    ui.progressBar->setValue(static_cast<int>((bytesReadTotal * 100) / fileSize));
                }
                QApplication::processEvents(); // Responsywnoœæ UI
            }
        }
        file.close();
    }

    // 5. Finalizacja wyniku (XOR Out)
    uint32_t finalResult = CppCrc32cFinalize(currentCrc);
    qint64 nsecs = timer.nsecsElapsed();

    // 6. Wyœwietlenie wyników
    QString resultHex = QString("%1").arg(finalResult, 8, 16, QChar('0')).toUpper();
    ui.lineEditResult->setText(resultHex);

    QString timeString = formatTimeElapsed(nsecs);
    ui.labelStats->setText(QString("Time: %1 | Size: %2")
        .arg(timeString)
        .arg(formatFileSize(fileSize)));

    // 7. Przywrócenie stanu kontrolek
    ui.pushBtnCalculate->setEnabled(true);
    ui.comboBoxAlgo->setEnabled(true);

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