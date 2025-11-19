#include "mainwindow.h"
#include <QDebug>
#include <QString>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_calculateCppButton_clicked()
{
    int liczba1 = 10;
    int liczba2 = 25;

    int wynikCpp = CppAdd(liczba1, liczba2);

    ui.resultLabel->setText(QString("Wynik C++ (CppAdd): %1").arg(wynikCpp));
}

void MainWindow::on_calculateAsmButton_clicked()
{
    int liczba1 = 10;
    int liczba2 = 25;

    int wynikAsm = AsmAdd(liczba1, liczba2);

    ui.resultLabel->setText(QString("Wynik ASM (AsmAdd): %1").arg(wynikAsm));
}