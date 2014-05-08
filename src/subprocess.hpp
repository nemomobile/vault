#ifndef _CUTES_SUBPROCESS_HPP_
#define _CUTES_SUBPROCESS_HPP_

#include <QProcess>
#include <QVariant>

namespace subprocess {

class Process : public QObject
{
    Q_OBJECT
public:
    Process()
        : isRunning_(false)
        , isError_(false)
    {}

    void start(QString const &, QStringList const &);

    bool wait(int timeout);
    bool is_error() const;
    QString errorInfo() const;

    void popen_sync(QString const &cmd, QStringList const &args)
    {
        start(cmd, args);
        ps.waitForStarted(-1);
    }

    int rc() const
    {
        return is_error() ? -1111 : ps.exitCode();
    }

    qint64 write(QString const &data)
    {
        return ps.write(data.toUtf8());
    }

    QByteArray stdout() const
    {
        return ps.readAllStandardOutput();
    }

    QByteArray stderr() const
    {
        return ps.readAllStandardError();
    }

    void stdinClose()
    {
        ps.closeWriteChannel();
    }

    Q_PID pid() const
    {
        return ps.pid();
    }

    void check_error(QVariantMap const &error_info);
    void check_error()
    {
        check_error(QVariantMap());
    }

    QByteArray check_output(QString const &, QStringList const &, QVariantMap const &);
    QByteArray check_output(QString const &cmd, QStringList const &args)
    {
        return check_output(cmd, args, QVariantMap());
    }

private:
    
    mutable QProcess ps;
    bool isRunning_;
    bool isError_;

private slots:
    void onError(QProcess::ProcessError);
    void onFinished(int, QProcess::ExitStatus);
};

QByteArray check_output(QString const &cmd, QStringList const &args, QVariantMap const &);
static inline QByteArray check_output(QString const &cmd, QStringList const &args)
{
    return check_output(cmd, args, QVariantMap());
}


}
#endif // _CUTES_SUBPROCESS_HPP_
