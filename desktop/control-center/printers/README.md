# Printers

This view retains KDE's PrinterModel, JobModel, JobSortFilterModel,
PrinterCommands and ProcessRunner. Cards expose pause/resume, printer settings,
the full queue, and cancellation for eligible jobs. The inline list keeps KDE's
three-job limit and links to the full queue for additional jobs. Printer groups,
default printers, locations, busy state and native status messages remain visible.

Search matches description and queue name. Empty, filtered-empty and unavailable
printing-service states have separate explanations. Refresh updates both native
models. Native command/job errors are displayed in the page. Configuration still
honors KAuthorized. No printer is added, paused, resumed or cancelled by tests.

Top-level cards and search match the tray heading width; the compact footer fits
its controls. Natural height depends on visible cards and expanded job lists.

Run `python3 tools/verify-printers.py --output /tmp/printers.png` for fixture
interactions and native model/role/error checks. `--render-only --dark --large
--rtl` covers the alternate presentation. Native tray loading and size switching
are included in `tools/verify-tray-sizing.py`.
