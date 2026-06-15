// toyopuc.cpp —— 完全不用改，保持你原来的实现
#include "toyopuc.h"
#include <QDebug>
#include <cstring>

Toyopuc::Toyopuc(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &Toyopuc::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Toyopuc::disconnected);
}

Toyopuc::~Toyopuc()
{
    disconnect();
}

// ============================================================
// 以下全部保持你原来的实现，一字不改
// ============================================================

bool Toyopuc::connectToPLC(const QString &host, quint16 port)
{
    if(m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->abort();
    }

    m_socket->connectToHost(host, port);
    if(!m_socket->waitForConnected(5000))
    {
        qWarning() << "[TOYOPUC] 连接失败:" << m_socket->errorString();
        return false;
    }
    qDebug() << "[TOYOPUC] 已连接" << host << ":" << port;
    return true;
}

void Toyopuc::disconnect()
{
    if(m_socket && m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->disconnectFromHost();
    }
}

bool Toyopuc::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool Toyopuc::readInt(quint16 dAddr, qint32 &value)
{
    QByteArray data;
    if(!readWords(dAddr, 2, data) || data.size() < 4)
    {
        return false;
    }
    quint32 raw = toU32(data, 0);
    value = static_cast<qint32>(raw);
    qDebug() << "[READ INT] D" << Qt::hex << dAddr << "=" << Qt::dec << value;
    return true;
}

bool Toyopuc::readFloat(quint16 dAddr, float &value)
{
    QByteArray data;
    if(!readWords(dAddr, 2, data) || data.size() < 4)
    {
        return false;
    }
    quint32 raw = toU32(data, 0);
    std::memcpy(&value, &raw, sizeof(float));
    qDebug() << "[READ FLOAT] D" << Qt::hex << dAddr << "=" << Qt::dec << value;
    return true;
}

bool Toyopuc::writeInt(quint16 dAddr, qint32 value)
{
    qDebug() << "[WRITE INT] D" << Qt::hex << dAddr << "=" << Qt::dec << value;
    quint32 raw = static_cast<quint32>(value);
    QByteArray data = le32(raw);
    return writeWords(dAddr, 2, data);
}

bool Toyopuc::writeFloat(quint16 dAddr, float value)
{
    qDebug() << "[WRITE FLOAT] D" << Qt::hex << dAddr << "=" << Qt::dec << value;
    quint32 raw;
    std::memcpy(&raw, &value, sizeof(float));
    QByteArray data = le32(raw);
    return writeWords(dAddr, 2, data);
}

bool Toyopuc::readWords(quint16 dAddr, quint16 wordCount, QByteArray &data)
{
    quint16 addr = dToInternal(dAddr);
    QByteArray req;
    req.append(le16(0x0000));
    req.append(le16(5));
    req.append(static_cast<char>(0x1C));
    req.append(le16(addr));
    req.append(le16(wordCount));

    QByteArray resp;
    if(!sendAndReceive(req, resp))
    {
        return false;
    }

    if(resp.size() < 5)
    {
        return false;
    }
    if(static_cast<quint8>(resp[0]) != 0x80)
    {
        qWarning() << "[TOYOPUC] 响应方向错误:" << Qt::hex << (quint8)resp[0];
        return false;
    }
    if(static_cast<quint8>(resp[4]) != 0x1C)
    {
        qWarning() << "[TOYOPUC] 命令回显不匹配:" << Qt::hex << (quint8)resp[4];
        return false;
    }
    data = resp.mid(5);
    return true;
}

bool Toyopuc::writeWords(quint16 dAddr, quint16 wordCount, const QByteArray &data)
{
    quint16 addr = dToInternal(dAddr);
    quint16 len = 3 + static_cast<quint16>(data.size());

    QByteArray req;
    req.append(le16(0x0000));
    req.append(le16(len));
    req.append(static_cast<char>(0x1D));
    req.append(le16(addr));
    req.append(data);

    QByteArray resp;
    if(!sendAndReceive(req, resp))
    {
        return false;
    }

    if(resp.size() < 5)
    {
        return false;
    }
    if(static_cast<quint8>(resp[0]) != 0x80)
    {
        qWarning() << "[TOYOPUC] 写入响应方向错误";
        return false;
    }
    if(static_cast<quint8>(resp[4]) != 0x1D)
    {
        qWarning() << "[TOYOPUC] 写入命令回显不匹配";
        return false;
    }
    return true;
}

bool Toyopuc::sendAndReceive(const QByteArray &request, QByteArray &response, int timeoutMs)
{
    QMutexLocker locker(&m_mutex);

    if(!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "[TOYOPUC] 未连接";
        return false;
    }

    m_socket->readAll();

    if(m_socket->write(request) != request.size())
    {
        qWarning() << "[TOYOPUC] 发送不完整";
        return false;
    }
    m_socket->flush();

    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();

    while(timer.elapsed() < timeoutMs)
    {
        if(m_socket->bytesAvailable() > 0)
        {
            buffer.append(m_socket->readAll());
        }

        if(buffer.size() >= 4)
        {
            quint16 bodyLen = static_cast<quint8>(buffer[2])
                              | (static_cast<quint8>(buffer[3]) << 8);
            int totalExpected = 4 + bodyLen;
            if(buffer.size() >= totalExpected)
            {
                response = buffer.left(totalExpected);
                qDebug().noquote() << "  [TX]" << request.toHex(' ').toUpper();
                qDebug().noquote() << "  [RX]" << response.toHex(' ').toUpper();
                return true;
            }
        }

        int remaining = timeoutMs - static_cast<int>(timer.elapsed());
        if(remaining <= 0)
        {
            break;
        }
        m_socket->waitForReadyRead(qMin(remaining, 30));
    }

    qWarning() << "[TOYOPUC] 响应超时" << timeoutMs << "ms, 已收到" << buffer.size() << "字节";
    if(!buffer.isEmpty())
    {
        qDebug().noquote() << "  [部分数据]" << buffer.toHex(' ').toUpper();
    }
    return false;
}

quint16 Toyopuc::dToInternal(quint16 d)
{
    return d + 0x1000;
}

QByteArray Toyopuc::le16(quint16 value)
{
    QByteArray a(2, '\0');
    a[0] = static_cast<char>(value & 0xFF);
    a[1] = static_cast<char>((value >> 8) & 0xFF);
    return a;
}

QByteArray Toyopuc::le32(quint32 value)
{
    QByteArray a(4, '\0');
    a[0] = static_cast<char>(value & 0xFF);
    a[1] = static_cast<char>((value >> 8) & 0xFF);
    a[2] = static_cast<char>((value >> 16) & 0xFF);
    a[3] = static_cast<char>((value >> 24) & 0xFF);
    return a;
}

quint32 Toyopuc::toU32(const QByteArray &data, int offset)
{
    return static_cast<quint8>(data[offset])
           | (static_cast<quint8>(data[offset + 1]) << 8)
           | (static_cast<quint8>(data[offset + 2]) << 16)
           | (static_cast<quint8>(data[offset + 3]) << 24);
}
