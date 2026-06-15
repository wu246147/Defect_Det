// toyopuc.h —— 和你原来的几乎一样
#ifndef TOYOPUC_H
#define TOYOPUC_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QElapsedTimer>
#include <QByteArray>


#pragma execution_character_set("utf-8")

class Toyopuc : public QObject
{
    Q_OBJECT

public:
    explicit Toyopuc(QObject *parent = nullptr);
    ~Toyopuc();

    // ★ 全部标记 Q_INVOKABLE，方便 BlockingQueuedConnection 调用
    Q_INVOKABLE bool connectToPLC(const QString &host, quint16 port);
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE bool isConnected() const;

    Q_INVOKABLE bool readInt(quint16 dAddr, qint32 &value);
    Q_INVOKABLE bool readFloat(quint16 dAddr, float &value);
    Q_INVOKABLE bool writeInt(quint16 dAddr, qint32 value);
    Q_INVOKABLE bool writeFloat(quint16 dAddr, float value);

signals:
    void connected();
    void disconnected();

private:
    bool readWords(quint16 dAddr, quint16 wordCount, QByteArray &data);
    bool writeWords(quint16 dAddr, quint16 wordCount, const QByteArray &data);
    bool sendAndReceive(const QByteArray &request, QByteArray &response, int timeoutMs = 3000);

    static quint16 dToInternal(quint16 d);
    static QByteArray le16(quint16 value);
    static QByteArray le32(quint32 value);
    static quint32  toU32(const QByteArray &data, int offset);

    QTcpSocket *m_socket = nullptr;
    QMutex      m_mutex;
};

#endif
