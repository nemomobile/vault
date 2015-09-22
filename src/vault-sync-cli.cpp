#include "vault-sync.hpp"

#define QS_(s) QLatin1String(s)

static Action actionFromName(QString const &name)
{
    Action res;
    if (name == QS_("import"))
        res = Action::Import;
    else if (name == QS_("export"))
        res = Action::Export;
    else
        error::raise({{QS_("msg"), QS_("Parameter 'action' is unknown")}
                , {QS_("action"), name}});
    return res;
}

int main_(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QS_("vault-copy"));
    parser.addHelpOption();

    parser << QCommandLineOption({QS_("a"), QS_("action")},
                                 QS_("The action passed by vault"), QS_("ACTION"))
         << QCommandLineOption({QS_("L"), QS_("dereference")},
                                 QS_("Follow symlinks in src"))
         << QCommandLineOption({QS_("n"), QS_("no-clobber")},
                                 QS_("Do not overwrite an existing file"))
         << QCommandLineOption({QS_("b"), QS_("blobs")},
                                 QS_("Use blob mode"))
         << QCommandLineOption({QS_("r"), QS_("recursive")},
                                 QS_("Copy directories recursively"))
         << QCommandLineOption({QS_("u"), QS_("update")},
                                 QS_("Update dst only if newer"))
        ;
    parser.addPositionalArgument(QS_("src"), QS_("Source file/directory"));
    parser.addPositionalArgument(QS_("dst"), QS_("Destination file/directory"));
    parser.process(app);

    auto args = parser.positionalArguments();
    auto last = args.size() - 1;
    if (last < 1)
        error::raise({{QS_("msg"), QS_("There is no src or dst")}, {QS_("args"), args}});
    auto dst = args[last];
    auto action = actionFromName(parser.value(QS_("action")));
    auto vault = std::make_shared<Vault>(action == Action::Import ? args[0] : dst);
    options_type options{vault
            , parser.isSet(QS_("blobs")) ? DataHint::Big : DataHint::Compact
            , parser.isSet(QS_("recursive")) ? Depth::Recursive : Depth::Shallow
            , parser.isSet(QS_("no-clobber")) ? Overwrite::No : Overwrite::Yes
            , parser.isSet(QS_("dereference")) ? Deref::Yes : Deref::No
            , parser.isSet(QS_("update")) ? Update::Yes : Update::No
            };

    Processor processor;
    for (auto i = 0; i < last; ++i) {
        auto src = args[i];
        debug::debug(QS_("Source"), src);
        auto ctx = std::make_shared<context_type>
            (options, action, QFileInfo(src), QFileInfo(dst));
        processor.add(ctx);
    }
    processor.execute();
    return 0;
}

int main(int argc, char **argv)
{
    int res = 1;
    try {
        res = main_(argc, argv);
    } catch (std::exception const &e) {
        debug::error(QS_("Error:"),  QS_(e.what()));
    } catch (...) {
        debug::error(QS_("Unknown error"));
    }
    return res;
}
