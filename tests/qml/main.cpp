
#include <QCoreApplication>
#include <QStringList>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const char *dir = "/tmp/vault-test";
    system(qPrintable(QLatin1String("mkdir ") + dir));
    system("qmltestrunner -input " TEST_DIR " -import " IMPORT_DIR);
    system(qPrintable(QLatin1String("rm -rf ") + dir));

    return 0;
}
