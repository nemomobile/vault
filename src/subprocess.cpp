/**
 * @file subprocess.cpp
 * @brief Subprocess execution API resembing std python lib
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/subprocess.hpp>
#include <qtaround/error.hpp>
#include <qtaround/debug.hpp>
#include <qtaround/util.hpp>

namespace subprocess {

void Process::start(QString const &cmd, QStringList const &args)
{
    if (isRunning_)
        error::raise({{"msg", "Can't start process, it is running"}
                , {"cmd", cmd}, {"args", QVariant(args)}});
    debug::info("Start", cmd, args);
    connect(ps.get(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>
            (&QProcess::finished), this, &Process::onFinished);
    connect(ps.get(), static_cast<void (QProcess::*)(QProcess::ProcessError)>
            (&QProcess::error), this, &Process::onError);
    isError_ = false;
    ps->start(cmd, args);
}

namespace {

const QMap<QProcess::ProcessError, QString> errorNames = {
    {QProcess::FailedToStart, "FailedToStart"}
    , {QProcess::Crashed, "Crashed"}
    , {QProcess::Timedout, "Timedout"}
    , {QProcess::WriteError, "WriteError"}
    , {QProcess::ReadError, "ReadError"}
    , {QProcess::UnknownError, "UnknownError"}
};

} // namespace

QString Process::errorInfo() const
{
    if (isRunning_ || !rc())
        return QString();
    QString res;
    QDebug(&res) << errorNames[ps->error()] << ps->exitStatus();
    return res;
}

void Process::onError(QProcess::ProcessError err)
{
    debug::warning("Process returned error", errorNames[err]);
    isRunning_ = false;
    isError_ = true;
}

void Process::onFinished(int res, QProcess::ExitStatus status)
{
    debug::info("Process is finished", res, status);
    isRunning_ = false;
    isError_ = false;
}

bool Process::wait(int timeout)
{
    auto res = (ps->state() == QProcess::NotRunning)
        || ps->waitForFinished(timeout)
        || (ps->state() == QProcess::NotRunning);
    isRunning_ = !res;
    return res;
}

bool Process::is_error() const
{
    if (isError_)
        return true;

    auto e = ps->error();
    auto status = ps->exitStatus();
    return (status == QProcess::NormalExit
            && (e == QProcess::UnknownError
                || (!isRunning_ && e == QProcess::Timedout))
            ? false
            : true);
}

void Process::check_error(QVariantMap const &error_info)
{
    if (!rc())
        return;
    QVariantMap err = {{"msg", "Process error"}
                       , {"cmd", ps->program()}
                       , {"args", QVariant(ps->arguments())}
                       , {"rc", rc()}
                       , {"stderr", stderr()}
                       , {"stdout", stdout()}
                       , {"info", errorInfo()}};
    err.unite(error_info);
    error::raise(err);
}

QByteArray Process::check_output
(QString const &cmd, QStringList const &args, QVariantMap const &error_info)
{
    start(cmd, args);
    wait(-1);
    check_error(error_info);
    return stdout();
}

int Process::check_call
(QString const &cmd, QStringList const &args, QVariantMap const &error_info)
{
    start(cmd, args);
    wait(-1);
    check_error(error_info);
    return rc();
}

QByteArray Process::stdout() const
{
    auto res = ps->readAllStandardOutput();
    debug::debug("Stdout", res);
    return res;
}

QByteArray Process::stderr() const
{
    auto res = ps->readAllStandardError();
    debug::debug("Stderr", res);
    return res;
}


QByteArray check_output(QString const &cmd, QStringList const &args
                        , QVariantMap const &error_info)
{
    Process p;
    return p.check_output(cmd, args, error_info);
}

int check_call(QString const &cmd, QStringList const &args
               , QVariantMap const &error_info)
{
    Process p;
    return p.check_call(cmd, args, error_info);
}

}
