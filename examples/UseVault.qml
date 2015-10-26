import QtQuick 2.0
import NemoMobile.Vault 1.1

QtObject {
    property Vault vault: Vault {
        onDone: vaultOperationDone(operation, data)
        onProgress: vaultOperationProgress(operation, data)
        onError: vaultOperationError(operation, error)
        onData: vaultData(id, context)
    }

    // event handlers

    function vaultOperationDone(operation, data) {
        console.log("vault operation", operation, "done")
        var action
        switch (operation) {
        case Vault.Connect:
            break
        case Vault.Maintenance:
            break
        case Vault.Backup:
            break
        case Vault.Restore:
            break
        case Vault.RemoveSnapshot:
            break
        case Vault.ExportSnapshot:
            console.log("snapshot exporting is done"
                        , data.rc, data.snapshot, data.dst
                        , data.stdout, data.stderr)
            break
        case Vault.ExportImportPrepare:
            break
        case Vault.ExportImportExecute:
            break
        default:
            break
        }
    }

    function vaultData(operation, context) {
        switch (operation) {
        case Vault.SnapshotUnits:
            break
        case Vault.Snapshots:
            break
        case Vault.Units:
            break
        default:
            break
        }
    }

    function vaultOperationProgress(operation, data) {
        switch (operation) {
        case Vault.Backup:
            break
        case Vault.Restore:
            break
        case Vault.ExportImportExecute:
            break
        default:
            break
        }
    }

    function vaultOperationError(operation, error) {
        console.log("vault operation", operation, "error")
        switch (operation) {
        case Vault.Connect:
            break
        case Vault.Backup:
            break
        case Vault.Restore:
            break
        case Vault.RemoveSnapshot:
            break
        case Vault.ExportImportPrepare:
            break
        case Vault.ExportImportExecute:
            break
        case Vault.ExportSnapshot:
            console.log("error exporting snapshot"
                        , data.rc, data.snapshot, data.dst
                        , data.stdout, data.stderr)
        default:
            break
        }
    }

}
