#pragma once

#include <QtWidgets/QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "ui_CRC32CGUI.h" 
#include "../DllCpp/dll_cpp.h" 

extern "C" uint32_t CppCrc32cInit();
extern "C" uint32_t CppCrc32cUpdate(uint32_t currentCrc, const uint8_t* data, size_t length);
extern "C" uint32_t CppCrc32cFinalize(uint32_t currentCrc);
extern "C" uint32_t CppCrc32c(const uint8_t* data, size_t length);

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
    void on_pushBtnLoadRam_clicked(); // Nowy slot

private:
    Ui::MainWindow ui;

    // Elementy dynamiczne
    QLabel* labelMemoryMode;
    QComboBox* comboBoxMemoryMode;

    // Nowe elementy dla trybu RAM
    QPushButton* pushBtnLoadRam;
    QByteArray m_ramBuffer;
    bool m_isRamLoaded;

    QString formatFileSize(qint64 size);
    QString formatTimeElapsed(qint64 nanoseconds);
    void applyProfessionalStyle();
    void setupMemoryControls();
    void setupRamControl(); // Nowa funkcja inicjalizuj¹ca
};