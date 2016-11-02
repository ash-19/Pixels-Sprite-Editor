#include "PopupWindow.h"
#include "ui_PopupWindow.h"
#include <QPushButton>
#include <iostream>

//Creates a general purpose PopupWindow.
PopupWindow::PopupWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PopupWindow)
{
    ui->setupUi(this);
    connect(ui->okButton, SIGNAL(pressed()), this, SLOT(okClicked()));
}

//Destructs the current PopupWindow.
PopupWindow::~PopupWindow()
{
    delete ui;
}

//Sets the text of the current PopupWindow.
void PopupWindow::setText(const QString& text){
    ui->popupLabel->setText(text);
}

void PopupWindow::setTitle(const QString& title){
    this->setWindowTitle(title);
}

void PopupWindow::okClicked(){
    this->close();
}
