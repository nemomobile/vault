
#include <vault/unit.hpp>

#include "vault/os.hpp"
#include "vault/subprocess.hpp"
#include "vault/error.hpp"
#include "vault/util.hpp"
#include "vault/debug.hpp"

#include <QCoreApplication>
#include <sys/types.h>
#include <signal.h>

const QVariantMap info = {
    {"home", map({{"bin", list({"unit1"})}})}
};

int main(int argc, char *argv[])
{
    try {
        QCoreApplication app(argc, argv);
        using namespace vault::unit;
        execute(getopt(), info);
    } catch (error::Error const &e) {
        qDebug() << e;
    } catch (std::exception const &e) {
        qDebug() << e.what();
    }
    return 0;
}
