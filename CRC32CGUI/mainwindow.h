#pragma once

#include <QtWidgets/QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "ui_CRC32CGUI.h" 
#include "CrcCalculator.h"

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

    CrcCalculator m_calculator;

    QString formatFileSize(qint64 size);
    QString formatTimeElapsed(qint64 nanoseconds);
    void applyProfessionalStyle();
    void setupMemoryControls();
    void setupRamControl();
};