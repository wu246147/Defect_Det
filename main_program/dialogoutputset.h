#ifndef DIALOGOUTPUTSET_H
#define DIALOGOUTPUTSET_H

#include <QDialog>
#include <QThread>
#include <QtConcurrent>

#include "jobmanager.h"
#include "tool/communication.h"
#include "mainwindow.h"

namespace Ui
{
    class DialogOutputSet;
}

class DialogOutputSet : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOutputSet(QWidget *parent = nullptr);
    DialogOutputSet(JobManager *jobManager, QString *_plc_ip, int *_port, std::map<std::string, int> *_IOPara, QWidget *parent = nullptr);

    ~DialogOutputSet();


public:
    JobManager *jobmanager = nullptr;
    QString *plc_ip;
    int *port;

    std::map<std::string, int> *IOPara;


    bool is_loading = false;


private slots:


    void on_lineEdit_ip_textChanged(const QString &arg1);

    void on_spinBox_port_valueChanged(int arg1);


    void on_lineEdit_io_1_textChanged(const QString &arg1);

    void on_lineEdit_io_2_textChanged(const QString &arg1);

    void on_lineEdit_io_3_textChanged(const QString &arg1);

    void on_lineEdit_io_4_textChanged(const QString &arg1);

    void on_lineEdit_io_5_textChanged(const QString &arg1);

    void on_lineEdit_io_6_textChanged(const QString &arg1);

    void on_lineEdit_io_7_textChanged(const QString &arg1);

    void on_lineEdit_io_8_textChanged(const QString &arg1);

private:
    Ui::DialogOutputSet *ui;
};

#endif // DIALOGOUTPUTSET_H
