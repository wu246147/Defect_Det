#include "toyopuc_caller.h"

ToyopucCaller::ToyopucCaller()
{
    m_worker = new Toyopuc;
    m_thread = new QThread;
    m_worker->moveToThread(m_thread);
    m_thread->start();
}

ToyopucCaller::~ToyopucCaller()
{
    QMetaObject::invokeMethod(m_worker, [this]()
    {
        m_worker->disconnect();
    }, Qt::BlockingQueuedConnection);

    m_thread->quit();
    m_thread->wait();
    delete m_worker;
    delete m_thread;
}

bool ToyopucCaller::connectToPLC(const QString &host, quint16 port)
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->connectToPLC(host, port);
    });
    return result;
}

void ToyopucCaller::disconnect()
{
    blockingCall([&]()
    {
        m_worker->disconnect();
    });
}

bool ToyopucCaller::isConnected()
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->isConnected();
    });
    return result;
}

bool ToyopucCaller::readInt(quint16 dAddr, qint32 &value)
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->readInt(dAddr, value);
    });
    return result;
}

bool ToyopucCaller::readFloat(quint16 dAddr, float &value)
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->readFloat(dAddr, value);
    });
    return result;
}

bool ToyopucCaller::writeInt(quint16 dAddr, qint32 value)
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->writeInt(dAddr, value);
    });
    return result;
}

bool ToyopucCaller::writeFloat(quint16 dAddr, float value)
{
    bool result = false;
    blockingCall([&]()
    {
        result = m_worker->writeFloat(dAddr, value);
    });
    return result;
}

Toyopuc* ToyopucCaller::worker() const
{
    return m_worker;
}
