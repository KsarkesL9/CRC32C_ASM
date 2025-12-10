#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CRC32CGUI.h" 
#include "../DllCpp/dll_cpp.h" 


extern "C" int AsmAdd(int a, int b);


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushBtnBrowse_clicked();
    void on_pushBtnCalculate_clicked();
    void on_pushBtnHelp_clicked(); // Nowy slot dla pomocy

private:
    Ui::MainWindow ui;

    QString formatFileSize(qint64 size);
    QString formatTimeElapsed(qint64 nanoseconds);
    void applyProfessionalStyle(); // Metoda stylizuj¹ca

};