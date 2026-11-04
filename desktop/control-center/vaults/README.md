# Vaults

The Sequoia page retains Plasma Vault's native model, actions model, alphabetical
sort, status/icon reporting and reload-on-expansion behaviour. Its native backend
continues to handle passwords, setup, configuration, open/close operations,
forced closing and error messages. No encryption or filesystem logic is replaced.

Cards show locked/open, unavailable, busy, offline-only and error states. Unlock
and open, unlock only, lock, open files, configuration and force lock retain their
original model calls and device identifiers. Operations are disabled for busy or
unavailable vaults. Creation stays available in the content-sized footer and
native context menu. Top-level card widths match the shared tray header.

The external upstream QML package is copied by `control_center_vaults.py`.
`org.sequoia.vault` uses a native factory adapter; the original Vault library
stores metadata in its KPackage rather than embedding it in the plugin.

`python3 tools/verify-vaults.py --output /tmp/vaults.png` checks all action routes,
state guards, empty/error states, widths and content heights using a fake actions
object. `--render-only --dark --large --rtl` checks the alternate presentation.
The native tray and fork checks use private XDG paths and D-Bus sessions. Tests
do not create, unlock, lock, configure or inspect the contents of real vaults.
