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

public slots:
    void on_calculateCppButton_clicked();
    void on_calculateAsmButton_clicked();

private:
    Ui::MainWindow ui;
};