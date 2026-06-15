#ifndef TOYOPUC_CALLER_H
#define TOYOPUC_CALLER_H

#include "toyopuc.h"
#include <QThread>

#pragma execution_character_set("utf-8")

class ToyopucCaller
{
public:
    ToyopucCaller();
    ~ToyopucCaller();

    bool connectToPLC(const QString &host, quint16 port);
    void disconnect();
    bool isConnected();

    bool readInt(quint16 dAddr, qint32 &value);
    bool readFloat(quint16 dAddr, float &value);
    bool writeInt(quint16 dAddr, qint32 value);
    bool writeFloat(quint16 dAddr, float value);

    Toyopuc* worker() const;

private:
    template<typename Func>
    void blockingCall(Func &&func)
    {
        QMetaObject::invokeMethod(m_worker,
                                  std::forward<Func>(func),
                                  Qt::BlockingQueuedConnection);
    }

    Toyopuc *m_worker = nullptr;
    QThread *m_thread = nullptr;
};

#endif // TOYOPUC_CALLER_H
