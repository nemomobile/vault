
import QtTest 1.0
import QtQuick 2.0
import Vault 1.0

Item {
    width: 200
    height: 200

    Vault {
        id: vault
    }
    SignalSpy {
        id: doneSpy
        target: vault
        signalName: "done"
    }
    SignalSpy {
        id: errorSpy
        target: vault
        signalName: "error"
    }

    resources: TestCase {
        name: "vault"
        when: windowShown
        function test_vault_init() {
            vault.root = "/tmp/vault-test/root"
            vault.backupHome = "/tmp/vault-test/home"

            vault.connectVault(false)
            tryCompare(doneSpy, "count", 1)
            verify(errorSpy.count == 0)
            verify(vault.snapshots().length == 0)
        }
    }
}
