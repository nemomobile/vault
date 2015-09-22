#include <qtaround/os.hpp>

#include "vault-util.hpp"
#include <fcntl.h>

namespace os = qtaround::os;

#define QS_(s) QLatin1String(s)

int main_(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription
        (QS_("vault-resolve is used to resolve URI stored in the "
             "blob reference file to the blob path in the vault storage.\n"
             "\tAlso, if option 'vault' is provided, it converts passed URI to the blob path."));
    parser.addHelpOption();

    parser << QCommandLineOption({QS_("V"), QS_("vault")},
                                 QS_("Vault root directory"), QS_("vault"))
           << QCommandLineOption({QS_("R"), QS_("reverse")},
                                 QS_("Generate URI from the blob path"))
           << QCommandLineOption({QS_("c"), QS_("convert")},
                                 QS_("Try to convert old-style symlink to the reference file"))
        ;
    parser.addPositionalArgument(QS_("src"), QS_("Blob reference file path or uri"));
    parser.process(app);

    auto args = parser.positionalArguments();
    if (args.size() < 1) {
        parser.showHelp();
        error::raise({{"msg", "Parameter is missing"}});
    }
    QString root, uri, src;
    auto is_reverse = parser.isSet("reverse")
        , is_convert = parser.isSet("convert");
    if (parser.isSet("vault")) {
        root = parser.value(QS_("vault"));
        uri = args.at(0);
        if (is_reverse)
            src = args.at(0);
    } else {
        if (is_reverse) {
            parser.showHelp();
            error::raise({{"msg", "Reverse conversion requires vault path"}});
        }
        src = args.at(0);
        if (!is_convert)
            uri = read_text(src, VAULT_URI_MAX_SIZE).trimmed();
        root = src;
    }

    auto vault = std::make_shared<Vault>(root);
    QTextStream cout{stdout};

    auto convert_symlink = [&cout, &vault](QString const &src) {
        if (os::path::isSymLink(src)) {
            QFileInfo linked_path(readlink(src));
            if (linked_path.isRelative()) {
                auto link_dir = QFileInfo(src).dir().absolutePath();
                linked_path.setFile(path(link_dir, linked_path.filePath()));
                if (vault->is_blob_path(linked_path)) {
                    debug::info("Convert", src);
                    auto hash = vault->blob_hash(linked_path.filePath());
                    auto uri = vault->uri_from_hash(hash);
                    auto perm = linked_path.permissions();
                    unlink(src);
                    auto ref_file = open_file(src, O_RDWR | O_CREAT | O_EXCL, &perm);
                    ref_file->write(uri.toUtf8());
                    cout << uri;
                }
            }
        } else {
            debug::info("Not a link", src);
        }
        return 0;
    };

    if (is_convert) {
        return convert_symlink(src);
    } else if (is_reverse) {
        cout << vault->uri_from_hash(vault->blob_hash(src));
    } else {
        cout << vault->path_from_uri(uri.trimmed());
    }
    return 0;
}

int main(int argc, char **argv)
{
    int res = 1;
    try {
        res = main_(argc, argv);
    } catch (std::exception const &e) {
        debug::error(QS_("vault-resolve"), QS_("Error:"),  QS_(e.what()));
    } catch (...) {
        debug::error(QS_("vault-resolve"), QS_("Unknown error"));
    }
    return res;
}
