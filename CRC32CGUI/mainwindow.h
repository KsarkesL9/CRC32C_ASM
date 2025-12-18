#pragma once

#include <QtWidgets/QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <vector>
#include <future>
#include "ui_CRC32CGUI.h" 
#include "../DllCpp/dll_cpp.h" 

extern "C" uint32_t AsmCrc32cUpdate(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" uint32_t AsmCrc32cUpdate3Way(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" DLL_API uint32_t AsmCrc32cUpdateFusion(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" uint32_t AsmCrc32cUpdateReport(uint32_t currentCrc, const uint8_t* data, size_t length);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushBtnBrowse_clicked();
    void on_pushBtnCalculate_clicked();
    void on_pushBtnHelp_clicked();
    void on_pushBtnLoadRam_clicked();
    void on_comboBoxAlgo_currentIndexChanged(int index); 

private:
    Ui::MainWindow ui;

    QLabel* labelMemoryMode;
    QComboBox* comboBoxMemoryMode;

    QPushButton* pushBtnLoadRam;
    QByteArray m_ramBuffer;
    bool m_isRamLoaded;

    QString formatFileSize(qint64 size);
    QString formatTimeElapsed(qint64 nanoseconds);
    void applyProfessionalStyle();
    void setupMemoryControls();
    void setupRamControl();
    void runDiagnostics();

    uint32_t calculateChunk(const uint8_t* data, size_t length, int algoIndex);
};