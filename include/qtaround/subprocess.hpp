#ifndef _CUTES_SUBPROCESS_HPP_
#define _CUTES_SUBPROCESS_HPP_
/**
 * @file subprocess.hpp
 * @brief Subprocess execution API resembing std python lib
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <memory>

#include <QProcess>
#include <QVariant>

namespace subprocess {

class Process : public QObject
{
public:
    Process()
        : ps(new QProcess())
        , isRunning_(false)
        , isError_(false)
    {}

    Process(Process &&from)
        : ps(std::move(from.ps))
        , isRunning_(from.isRunning_)
        , isError_(from.isError_)
    {}

    void setWorkingDirectory(QString const &d)
    {
        ps->setWorkingDirectory(d);
    }

    void start(QString const &, QStringList const &);

    bool wait(int timeout);
    bool is_error() const;
    QString errorInfo() const;

    void popen_sync(QString const &cmd, QStringList const &args)
    {
        start(cmd, args);
        ps->waitForStarted(-1);
    }

    int rc() const
    {
        return is_error() ? -1111 : ps->exitCode();
    }

    qint64 write(QString const &data)
    {
        return ps->write(data.toUtf8());
    }

    QByteArray stdout() const;

    QByteArray stderr() const;

    void stdinClose()
    {
        ps->closeWriteChannel();
    }

    Q_PID pid() const
    {
        return ps->pid();
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

    int check_call(QString const &, QStringList const &, QVariantMap const &);
    int check_call(QString const &cmd, QStringList const &args)
    {
        return check_call(cmd, args, QVariantMap());
    }

private:
    
    mutable std::unique_ptr<QProcess> ps;
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

int check_call(QString const &cmd, QStringList const &args, QVariantMap const &);
static inline int check_call(QString const &cmd, QStringList const &args)
{
    return check_call(cmd, args, QVariantMap());
}


}
#endif // _CUTES_SUBPROCESS_HPP_
