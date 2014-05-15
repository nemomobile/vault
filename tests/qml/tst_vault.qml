
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
    SignalSpy {
        id: progressSpy
        target: vault
        signalName: "progress"
    }

    resources: TestCase {
        name: "vault"
        when: windowShown
        function test_init() {
            vault.root = "/tmp/vault-test/root"
            vault.backupHome = "/tmp/vault-test/home"

            vault.connectVault(false)
            tryCompare(doneSpy, "count", 1)
            verify(errorSpy.count == 0)
            verify(vault.snapshots().length == 0)
        }
        function test_units() {
            vault.root = "/tmp/vault-test/root"
            vault.backupHome = "/tmp/vault-test/home"

            var unit_script_fname = "unit1_vault";
            vault.registerUnit({name: "unit1", script: "./" + unit_script_fname}, false)
            var units = vault.units()
            verify(units.length == 1)
            verify('unit1' === units[0]) //Why does "'unit1' in units" fail???

            verify(doneSpy.count == 1)
            vault.startBackup("", ["unit1"])
            tryCompare(doneSpy, "count", 2)
        }
    }
}
