#ifndef _CUTES_SUBPROCESS_HPP_
#define _CUTES_SUBPROCESS_HPP_

#include <QProcess>

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

    Q_PID pid() const
    {
        return ps.pid();
    }

private:

    mutable QProcess ps;
    bool isRunning_;
    bool isError_;

private slots:
    void onError(QProcess::ProcessError);
    void onFinished(int, QProcess::ExitStatus);
};

QByteArray check_output(QString const &cmd, QStringList const &args);


}
#endif // _CUTES_SUBPROCESS_HPP_
