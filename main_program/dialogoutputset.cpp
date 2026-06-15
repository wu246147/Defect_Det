#include "dialogoutputset.h"
#include "ui_dialogoutputset.h"
#include "qDebug"

DialogOutputSet::DialogOutputSet(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogOutputSet)
{
    ui->setupUi(this);
}


DialogOutputSet::DialogOutputSet(JobManager *j, QString *_plc_ip, int *_port,
                                 std::map<std::string, int> *_IOPara, QWidget *parent):
    QDialog(parent),
    ui(new Ui::DialogOutputSet)
{
    ui->setupUi(this);
    jobmanager = j;
    plc_ip = _plc_ip;
    port = _port;
    IOPara = _IOPara;


    is_loading = true;
    ui->lineEdit_ip->setText(*plc_ip);
    ui->spinBox_port->setValue(*port);

    ui->lineEdit_io_1->setText(QString::number(IOPara->at("heardBeat"), 16).toUpper());
    ui->lineEdit_io_2->setText(QString::number(IOPara->at("vehicleModelFeedback"), 16).toUpper());
    ui->lineEdit_io_3->setText(QString::number(IOPara->at("getReady"), 16).toUpper());
    ui->lineEdit_io_4->setText(QString::number(IOPara->at("detFinish"), 16).toUpper());
    ui->lineEdit_io_5->setText(QString::number(IOPara->at("detResult"), 16).toUpper());

    ui->lineEdit_io_6->setText(QString::number(IOPara->at("check"), 16).toUpper());
    ui->lineEdit_io_7->setText(QString::number(IOPara->at("vehicleMode"), 16).toUpper());
    ui->lineEdit_io_8->setText(QString::number(IOPara->at("vehicleModeChange"), 16).toUpper());


    is_loading = false;

    ui->groupBox->setVisible(false);
}

DialogOutputSet::~DialogOutputSet()
{
    delete ui;
}


void DialogOutputSet::on_lineEdit_ip_textChanged(const QString &arg1)
{
    if(is_loading)
    {
        return;
    }
    *plc_ip = arg1;
    // qDebug() << "IP address changed to:" << arg1;
}


void DialogOutputSet::on_spinBox_port_valueChanged(int arg1)
{
    if(is_loading)
    {
        return;
    }
    *port = arg1;
}





void DialogOutputSet::on_lineEdit_io_1_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("heardBeat") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_2_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("vehicleModelFeedback") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_3_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("getReady") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_4_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("detFinish") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_5_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("detResult") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_6_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("check") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_7_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("vehicleMode") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}


void DialogOutputSet::on_lineEdit_io_8_textChanged(const QString &arg1)
{
    bool ok;
    int val = arg1.toInt(&ok, 16);
    if(ok)
    {
        IOPara->at("vehicleModeChange") = val;
    }
    else
    {
        QMessageBox::warning(this, "输入不规范.", "输入必须位16进制的数字", QMessageBox::StandardButton::Ok);
    }
}

