#include "subprocess.hpp"
#include "error.hpp"
#include "debug.hpp"
#include "util.hpp"

namespace subprocess {

void Process::start(QString const &cmd, QStringList const &args)
{
    if (isRunning_)
        error::raise({{"msg", "Can't start process, it is running"}
                , {"cmd", cmd}, {"args", QVariant(args)}});
    connect(&ps, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>
            (&QProcess::finished), this, &Process::onFinished);
    connect(&ps, static_cast<void (QProcess::*)(QProcess::ProcessError)>
            (&QProcess::error), this, &Process::onError);
    isError_ = false;
    ps.start(cmd, args);
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
    QDebug(&res) << errorNames[ps.error()] << ps.exitStatus();
    return res;
}

void Process::onError(QProcess::ProcessError err)
{
    debug::debug("Process returned error", errorNames[err]);
    isRunning_ = false;
    isError_ = true;
}

void Process::onFinished(int res, QProcess::ExitStatus status)
{
    debug::debug("Process is finished", res, status);
    isRunning_ = false;
    isError_ = false;
}

bool Process::wait(int timeout)
{
    auto res = (ps.state() == QProcess::NotRunning)
        || ps.waitForFinished(timeout)
        || (ps.state() == QProcess::NotRunning);
    isRunning_ = !res;
    return res;
}

bool Process::is_error() const
{
    if (isError_)
        return true;

    auto e = ps.error();
    auto status = ps.exitStatus();
    return (status == QProcess::NormalExit
            && (e == QProcess::UnknownError
                || (!isRunning_ && e == QProcess::Timedout))
            ? false
            : true);
}

void Process::check_error()
{
    if (rc())
        error::raise({{"msg", "Process error"}
                , {"cmd", ps.program()}
                , {"args", QVariant(ps.arguments())}
                , {"rc", rc()}
                , {"stderr", stderr()}
                , {"stdout", stdout()}
                , {"info", errorInfo()}});
}

QByteArray Process::check_output(QString const &cmd, QStringList const &args)
{
    start(cmd, args);
    wait(-1);
    check_error();
    return stdout();
}

QByteArray check_output(QString const &cmd, QStringList const &args)
{
    Process p;
    return p.check_output(cmd, args);
}

}
