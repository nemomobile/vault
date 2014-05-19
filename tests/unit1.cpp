#include <vault/unit.hpp>
#include "vault_context.hpp"

#include <vault/util.hpp>
#include <vault/sys.hpp>
#include <vault/error.hpp>

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    try {
        QCoreApplication app(argc, argv);
        using namespace vault::unit;
        execute(getopt(), get(context, "unit1").toMap());
    } catch (error::Error const &e) {
        qDebug() << e;
    } catch (std::exception const &e) {
        qDebug() << e.what();
    }
    return 0;
}
