
#include <QCoreApplication>
#include <QStringList>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const char *dir = "/tmp/vault-test";
    int ret = system(qPrintable(QLatin1String("mkdir ") + dir));
    if (!ret) ret = system(qPrintable(QLatin1String("mkdir ") + dir + "/home"));
    if (!ret) ret = system("qmltestrunner -input " TEST_DIR " -import " IMPORT_DIR);
    if (!ret) ret = system(qPrintable(QLatin1String("rm -rf ") + dir));

    return ret;
}
