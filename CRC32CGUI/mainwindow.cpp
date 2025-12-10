#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    this->setWindowIcon(QIcon(":/resources/app_icon.ico"));

    ui.comboBoxAlgo->addItem("C++ (Standard Library)", 0);
    ui.comboBoxAlgo->addItem("ASM (Assembly x64 Optimized)", 1);
    this->setWindowIcon(QIcon("app_icon.ico"));
    QList<int> threadCounts = { 1, 2, 4, 8, 16, 32, 64 };
    for (int count : threadCounts) {
        ui.comboBoxThreads->addItem(QString::number(count), count);
    }

    ui.progressBar->setValue(0);

    applyProfessionalStyle();
}

MainWindow::~MainWindow()
{
}

QString MainWindow::formatTimeElapsed(qint64 nanoseconds)
{
    if (nanoseconds < 1000) {
        return QString("%1 ns").arg(nanoseconds);
    }

    double value = nanoseconds;

    // Microseconds (\u00B5 to symbol mikro)
    value /= 1000.0;
    if (value < 1000.0) {
        return QString("%1 \u00B5s").arg(value, 0, 'f', 2);
    }

    // Milliseconds
    value /= 1000.0;
    if (value < 1000.0) {
        return QString("%1 ms").arg(value, 0, 'f', 2);
    }

    // Seconds
    value /= 1000.0;
    return QString("%1 s").arg(value, 0, 'f', 3);
}

void MainWindow::applyProfessionalStyle()
{
    QString style = R"(
        QMainWindow {
            background-color: #2b2b2b;
        }
        QWidget {
            color: #e0e0e0;
            font-family: 'Segoe UI', 'Roboto', sans-serif;
            font-size: 14px;
        }
        QGroupBox {
            border: 1px solid #444;
            border-radius: 6px;
            margin-top: 20px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            color: #00aaff;
        }
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 4px;
            padding: 6px 12px;
            color: #ffffff;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
            border-color: #00aaff;
        }
        QPushButton:pressed {
            background-color: #2a2a2a;
        }
        QPushButton#pushBtnCalculate {
            background-color: #007acc;
            border: none;
            font-weight: bold;
            font-size: 16px;
        }
        QPushButton#pushBtnCalculate:hover {
            background-color: #0099ff;
        }
        QPushButton#pushBtnCalculate:disabled {
            background-color: #555;
            color: #888;
        }
        QLineEdit, QComboBox {
            background-color: #1e1e1e;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 4px;
            color: #ffffff;
        }
        QLineEdit:read-only {
            background-color: #252525;
            color: #aaa;
        }
        QProgressBar {
            border: 1px solid #444;
            border-radius: 4px;
            text-align: center;
            background-color: #1e1e1e;
        }
        QProgressBar::chunk {
            background-color: #00aaff;
            width: 10px;
        }
        QLabel#labelTitle {
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
        }
        QLabel#labelStats {
            color: #aaaaaa;
            font-size: 12px;
        }
    )";
    this->setStyleSheet(style);
}

void MainWindow::on_pushBtnHelp_clicked()
{
    QString helpText =
        "<h3>Program Purpose</h3>"
        "<p>This application calculates the <b>CRC32C checksum</b> of a selected file. It ensures the file is correct and not damaged.</p>"
        "<hr>"
        "<h3>Controls Description</h3>"
        "<ul>"
        "<li><b>Browse:</b> Click this button to select a file from your computer.</li>"
        "<li><b>Algorithm:</b> Choose how to calculate:"
        "<ul>"
        "<li><i>C++:</i> Standard method.</li>"
        "<li><i>ASM:</i> Very fast method (Assembly).</li>"
        "</ul>"
        "</li>"
        "<li><b>Thread Count:</b> Select number of threads to make it faster.</li>"
        "<li><b>Start Calculation:</b> Click to begin the process.</li>"
        "</ul>"
        "<hr>"
        "<h3>How to use</h3>"
        "<ol>"
        "<li>Click <b>Browse</b> and open your file.</li>"
        "<li>Select the <b>Algorithm</b> and <b>Threads</b>.</li>"
        "<li>Click <b>START CALCULATION</b>.</li>"
        "<li>Copy the result from the text box.</li>"
        "</ol>";

    QMessageBox::information(this, "Help / Info", helpText);
}

void MainWindow::on_pushBtnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select file for CRC32C calculation"), "", tr("All files (*.*)"));

    if (fileName.isEmpty())
        return;

    ui.lineEditFilePath->setText(fileName);

    QFileInfo fileInfo(fileName);
    ui.labelStats->setText(QString("Time: - | Size: %1").arg(formatFileSize(fileInfo.size())));

    ui.lineEditResult->clear();
    ui.progressBar->setValue(0);
}

void MainWindow::on_pushBtnCalculate_clicked()
{
    QString filePath = ui.lineEditFilePath->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No file selected!");
        return;
    }

    int algoIndex = ui.comboBoxAlgo->currentData().toInt();
    int threadCount = ui.comboBoxThreads->currentData().toInt();

    ui.pushBtnCalculate->setEnabled(false);
    ui.pushBtnBrowse->setEnabled(false);
    ui.progressBar->setValue(0);
    ui.labelStats->setText("Calculation in progress...");

    QElapsedTimer timer;
    timer.start();

    ui.progressBar->setValue(10);

    ui.progressBar->setValue(50);

    qint64 nsecs = timer.nsecsElapsed();
    ui.progressBar->setValue(100);

    QString fakeResult = "A1B2C3D4";
    ui.lineEditResult->setText(fakeResult);

    QFileInfo fileInfo(filePath);

    QString timeString = formatTimeElapsed(nsecs);

    ui.labelStats->setText(QString("Time: %1 | Size: %2")
        .arg(timeString)
        .arg(formatFileSize(fileInfo.size())));

    ui.pushBtnCalculate->setEnabled(true);
    ui.pushBtnBrowse->setEnabled(true);
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