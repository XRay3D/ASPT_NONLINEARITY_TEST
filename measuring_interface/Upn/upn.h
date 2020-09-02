#ifndef MY_PROTOCOL_H
#define MY_PROTOCOL_H

#include "../common_interfaces.h"
#include "myprotocol.h"
#include <QMutex>
#include <QObject>
#include <QSemaphore>
#include <QSerialPort>
#include <QThread>

#pragma pack(push, 1)
typedef struct {
    quint8 num;
    double val;
} WriteResistor_t;
#pragma pack(pop)

class Callbacks {
public:
    virtual void RxPing(const QByteArray& data) = 0;

    virtual void RxSetResistor(const QByteArray& data) = 0;
    virtual void RxWriteResistorValue(const QByteArray& data) = 0;
    virtual void RxReadResistorValue(const QByteArray& data) = 0;

    virtual void RxBufferOverflow(const QByteArray& data) = 0;
    virtual void RxWrongCommand(const QByteArray& data) = 0;
    virtual void RxTextualParcel(const QByteArray& data) = 0;
    virtual void RxCrcError(const QByteArray& data) = 0;
    virtual void RxNullFunction(const QByteArray& data) = 0;
};

class UpnPort;
class Upn : public QObject, private MyProtocol, public CommonInterfaces, private Callbacks {
    Q_OBJECT
    friend class UpnPort;
    friend class PortControl;

public:
    Upn(QObject* parent = 0);
    ~Upn();

    // CommonInterfaces interface
    bool IsConnected() const override;
    bool Ping(const QString& PortName = QString()) override;
    bool close();
    bool setResistor(int r);
    bool writeResistorValue(const QVector<double>& r);
    bool readResistorValue();
    QVector<double> resistors() const;

signals:
    void Close();
    void Open(int mode) const;
    void Write(const QByteArray& data);

private:
    mutable bool m_result;
    mutable QSemaphore m_semaphore;

    QByteArray m_data;
    QMutex m_mutex;
    QThread m_portThread;
    QVector<double> m_resistors;
    UpnPort* m_port;

    void RxPing(const QByteArray& data) override;

    void RxSetResistor(const QByteArray& data) override;
    void RxWriteResistorValue(const QByteArray& data) override;
    void RxReadResistorValue(const QByteArray& data) override;

    void RxBufferOverflow(const QByteArray& data) override;
    void RxWrongCommand(const QByteArray& data) override;
    void RxTextualParcel(const QByteArray& data) override;
    void RxCrcError(const QByteArray& data) override;
    void RxNullFunction(const QByteArray& data) override;
};

class UpnPort : public QSerialPort, private MyProtocol {
    Q_OBJECT

public:
    UpnPort(Upn* upn);
    void Close();
    void Open(int mode);
    void Write(const QByteArray& data);

private:
    QByteArray m_data;
    QMutex m_mutex;
    typedef void (Upn::*pFunc)(const QByteArray&);
    QVector<pFunc> m_func;
    Upn* m_upn;
    void ReadyRead();
};

#endif // MY_PROTOCOL_H
