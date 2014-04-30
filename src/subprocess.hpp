#ifndef _CUTES_SUBPROCESS_HPP_
#define _CUTES_SUBPROCESS_HPP_

#include <QProcess>

namespace subprocess {
class Process
{
public:
    Process()
        : is_finished(false)
    {}

    void start(QString const &cmd, QStringList const &args)
    {
        ps.start(cmd, args);
    }

    bool wait(int timeout)
    {
        return (ps.state() == QProcess::NotRunning)
            || ps.waitForFinished(timeout)
            || (ps.state() == QProcess::NotRunning);
    }

    bool is_error()
    {
        auto e = ps.error();
        auto status = ps.exitStatus();
        return (status == QProcess::NormalExit
                && (e == QProcess::UnknownError
                    || (is_finished && e == QProcess::Timedout))
                ? false
                : true);
    }

    int rc()
    {
        return is_error() ? -1 : ps.exitCode();
    }

    qint64 write(QString const &data)
    {
        return ps.write(data.toUtf8());
    }

    QByteArray stdout()
    {
        return ps.readAllStandardOutput();
    }

    QByteArray stderr()
    {
        return ps.readAllStandardError();
    }

private:

    QProcess ps;
    bool is_finished;
};

}
#endif // _CUTES_SUBPROCESS_HPP_
