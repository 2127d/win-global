# Validation — Sequoia Eleven 2.0

Environment: Fedora 44, Plasma 6.7.5, Qt 6.11.2, KDE Frameworks 6.30.

## Qt Quick activation from menu commands (2026-10-13)

- The native Sequoia style now enables the verified Quick overlay before
  supported applications construct QML engines. It sets Qt's cached control
  style as well as the import environment, so direct commands receive the same
  shared surfaces as patched desktop entries.
- `verify-quick-style-startup.py` loads real Kirigami pages through the compiled
  plugin under all six supported executable names. It checks import preservation
  and deduplication, native control-style selection, missing/malformed/empty/stale
  manifests, and exclusion of Plasma, greeters and unrelated applications.
- Real direct-launch previews pass for Discover and System Monitor. Stock System
  Settings passes in a private Wayland compositor; offscreen rendering hits an
  upstream touchpad module crash before its UI loads.
- `preview-quick-islands.py --app systemsettings --dark --direct
  --launch-via-kcm --platform wayland` also passes in that compositor, exercising
  the API used by Plasma settings buttons with user executable wrappers excluded.
  Direct Merkuro passes the shared-surface check; its embedded calendar layout
  remains covered by the existing wrapper.
- `verify-window-materials.py` passes all six application cases, including
  Plasma's own blur-region handling and window surface recreation.

## Qt Quick styling refresh (2026-10-13)

- `verify-quick-islands.py` passes installation/refresh, argument forwarding,
  stale-component fallback, edit protection and restoration.
- `verify-quick-backgrounds.py` passes all four cases, including loading the
  generated page in Qt and preserving the overlay after a failed build.
- `verify-theme-installer.py` passes (19 tests, two skipped), including new
  coverage for refreshing with unrelated edits and protecting incoming files
  outside the retirement scope.
- The isolated System Monitor dark/transparent preview loads the real floating
  components and passes card opacity, navigation and clipping checks. Its private
  D-Bus session requires execution outside the restricted sandbox.
- After deployment, all 255 KDE/Merkuro source fingerprints match; the installed
  wrapper selects the overlay and desktop controls style. The Quick receipt
  validates and unrelated component receipt entries remain unchanged.

## Tray popup repair (2026-10-07)

- The native tray fork copied CompactApplet.qml to contents/ui, but Plasma's
  KPackage containment lookup requires contents/applet/CompactApplet.qml.
  That missing entry made tray children use the shell's standalone popup while
  the shared tray popup also opened, causing focus loss and an extra blur surface.
- The builder now installs the compact component at the required path and imports
  its sibling UI types explicitly. The theme payload validator and archive tests
  check this entry before installation.
- A new private-Wayland regression test uses real mouse clicks in a Plasma panel.
  It reproduced the installed package closing Volume immediately after one click.
  The corrected package passes three rounds of Volume/Notifications/Clipboard
  clicks, switching pages, inside clicks, outside dismissal, and Control Center
  activation, with exactly one visible popup surface while open.
- Archive staging/rollback/migration tests and theme installer unit tests pass.
  Backups and before/after test logs are in `.staging/tray-popup-repair-20261007/`.

## Panel installer repair (2026-10-07)

- Global Theme now builds native tray, trash and animated task-manager forks on
  the destination system, installs their libraries and child packages under the
  system receipt, and selects custom IDs in all six optional desktop layouts.
- Archive migration preserves widget IDs, launchers, child settings and visibility.
  Installing archives without migration no longer removes the old shell override.
- Native widget defaults are limited to notification-area applets. The archive
  must be installed with its Python installer; a local-file QML install cannot
  deploy native adapters or child packages.
- Real staged Global Theme installation includes 623 tracked native package files
  on this Fedora system. Receipt validation, repeat install and removal pass.
- Widget archive functional tests pass: fresh tray with no seeded item settings
  loads custom Notifications, Volume and Clipboard; trash and other packages load;
  task controls, notification delivery and tray detail/Back navigation pass.
- Private Wayland Plasma applied all six layouts with the expected panel geometry,
  widget order and settings. Theme installer unit tests and GUI tests pass.
- Live deployment migrated the existing panel instances and restarted Plasma.
  DBus reports custom tray, trash and animated task-manager IDs; panel AppletOrder
  and pinned launchers match the backup, and the service is active with zero
  automatic restarts. The obsolete shell override was backed up and removed.
  Unrelated local lock-screen edits were preserved. Backup:
  `.staging/panel-repair-20261007/live-backup-20261007-164824/`.
- Other distributions and a full logout/login still require testing. Existing
  upstream QML integration checks fail the build before installation if the target
  Plasma version has incompatible APIs.

## Graphical installer (2026-10-02)

- Added a native PySide6 wizard, portable desktop launcher, Qt dependency
  bootstrap, distribution profiles, component selection, independent Settings
  and Gwenview makeovers, review, graphical sudo authentication, progress and
  saveable logs. The original `install.sh` remains available.
- `SEQUOIA_APP_INSTALL_TEST=1 python3 tools/verify-installer-gui.py`: 14 passing
  tests. Covers distro mismatch rejection, package dispatch for all four distro
  profiles, version normalization, empty/app-only selections, whole-theme
  exclusivity and selection restoration, login omission, argument forwarding,
  process start/failure/success states, and actual install/reinstall/removal of
  the existing Settings and Gwenview binaries into temporary destinations.
- `SEQUOIA_FULL_INSTALL_TEST=1 python3 tools/verify-theme-installer.py`: 13 passing
  tests, including real native builds and complete theme install/update/removal
  under temporary user/system roots. Selective artwork installs preserve prior
  components; stale files are retired only inside the selected scope. Original
  launchers, receipts, local-edit checks and rollback remain covered.
- Desktop launcher and generated app launchers pass `desktop-file-validate`.
  Modified Python files compile; shell entry points pass `bash -n`.
  Offscreen GUI captures are `preview/installer-distribution.png` and
  `preview/installer-components.png`.
- No host package transactions or live-account installations were performed.
  Actual Arch/Ubuntu/Debian installs, clean-machine dependency bootstrapping and
  interactive sudo/polkit password dialogs still need testing on those desktops.
  Distro selection does not remove the upstream Qt/KDE version requirements or
  the existing source compatibility guards for Quick/lock-screen overrides.

## Frosted login concept (2026-09-28)

- Rebuilt the account card, Sign In/Switch User controls, white icon set and
  labeled Sleep/Restart/Shut Down controls. The restart SVG keeps its original
  geometry. Native network/audio/battery models supply the status strip; missing
  devices are hidden. Clock/date/account data remain live.
- Added a bounded, cached native wallpaper-blur renderer. High-frequency test
  images verify that the card and power circles actually soften background detail,
  sample the correct colored region, keep rounded corners clear, and update their
  sampling coordinates after movement and resizing.
- `python3 tools/verify-login-screen.py` passes 19 cases including setup/cleanup,
  plus exact payload, repeat install/remove, backup restoration and native fallback
  checks. `SEQUOIA_FULL_INSTALL_TEST=1 python3 tools/verify-theme-installer.py` passes
  all nine tests. Native status dependencies are included in the Fedora package list.
- Both real test-greeter previews load without QML errors at 1280×720 in software
  rendering. A striped diagnostic preview also verifies blurred power controls
  with sharp foreground icons/labels. New native sources compile and Python syntax
  checks pass. No live installation, authentication or power action was performed.
- OpenGL preview attempts could not create a context: this environment has no
  usable graphics device/display. The renderer uses QPainter-backed images on both
  Qt Quick paths, but accelerated rendering still needs a live-system check.

## Login card redesign (2026-09-28)

- The new layout uses an upper-center translucent card, actual account avatar/name,
  password visibility control and Login button, lower-left live time/date, and
  lower-right restart/shutdown buttons. Both clock labels share one system-clock
  value refreshed each second; template text is not used.
- `python3 tools/verify-login-screen.py` passes all 15 QML cases, including the
  new Login button, account-name switching, date rollover, real-clock refresh,
  circular avatar rendering and responsive layouts from 800×480 through 4K.
  The avatar test captures scene coordinates to support the nested card layout.
- Exact payload, repeat installation/removal, original service-override restoration
  and all three native fallback checks pass with the new icon assets included.
- Updated light/dark previews were captured from the native test greeter at
  1024×536 without QML errors and visually reviewed against the new reference.
  No live installation, authentication, power action or service restart was run.

## Custom login screen (2026-09-27)

- `python3 tools/verify-login-screen.py` passes all 12 QML test cases, including
  initialization/cleanup, login failure/retry, passwordless/manual login, password
  reveal/Escape, account selection, real avatar loading, power availability and
  layout bounds at 800×480, 1216×640, 1920×1080 and 3840×2160.
- The same command builds/stages the real payload, checks the exact file list,
  repeats installation, and verifies removal/restoration of a pre-existing service
  drop-in. Native `plasma-login-greeter --test` checks pass for stock fallback with
  missing QML, invalid QML, and a different greeter executable hash.
- Both updated `preview/login-light.png` and `preview/login-dark.png` were captured
  from the native mock-authentication greeter at 1216×640 without QML errors and
  visually reviewed. Preview power handlers are replaced with no-ops; UI tests use
  counters. No shutdown or restart requests were sent.
- The full nine-test unified installer suite passes with the login payload added.
  Python compilation checks pass. The source archive recipe includes `login-screen`;
  distribution archives have not been regenerated.
- No live installation, PAM login, service restart or reboot was performed.
  Those remain manual checks after installation on Plasma Login Manager. SDDM
  is outside this implementation's supported scope.

## Unified theme installer (2026-09-27)

- `SEQUOIA_FULL_INSTALL_TEST=1 python3 tools/verify-theme-installer.py` passes
  all nine tests. The integration test builds the actual native plugins and
  installs to temporary user/system roots, repeats installation, rejects removal
  of edited files before deleting either scope, then uninstalls with `--force`.
  It checks adoption/removal of old theme files, exact restoration of a customized
  application launcher, preservation of an unrelated widget, and installation
  and removal of the complete login theme payload. `/etc/plasmalogin.conf` and
  authentication settings remain untouched.
- Receipt tests cover upgrades, obsolete-file removal, symlinks and path
  boundaries, rollback after an injected copy failure, local-edit protection,
  and repeat uninstall. Dependency tests simulate RPM results and assert that
  DNF is asked to install only missing packages; they do not run DNF or sudo.
  The dependency names were also checked against the local Fedora RPM database
  and the official Fedora package listings referenced in `tools/install-theme.py`.
- `tools/verify-materials.py`, `tools/verify-quick-islands.py`,
  `tools/verify-user-portability.py`, and `tools/verify-desktop-portability.py`
  pass. The account test now installs its fixtures before running the
  activation-only `apply-desktop.py`; restoration covers that command's settings
  changes, while theme-file removal is tested by the new uninstaller tests.
- Shell and Python syntax checks pass. The elevated live-system copy, SELinux
  relabeling, and package-manager transaction were not executed. Full install
  testing used isolated destinations, and desktop activation remained stubbed.
  Existing distribution archives have not been rebuilt for these changes.

## Additional account portability checks (2026-09-26)

- `python3 tools/verify-user-portability.py` passes the desktop and font
  installation/restoration workflows with disposable homes, default XDG paths,
  external XDG directories, empty XDG variables, paths containing spaces, and
  an unrelated working directory. Artwork, fonts, lock-screen files, KConfig
  writes, backups and restoration are real; commands that activate the desktop
  are intercepted. Existing settings are restored and newly installed assets
  are removed by restoration.
- These tests reproduced and fixed two issues: empty XDG variables caused some
  Python installers to use the working directory, and restoration rejected
  valid external XDG destinations. A separate boundary check confirms another
  account's paths and whole HOME/XDG roots are rejected before any restore edits.
- `python3 tools/verify-user-portability.py --optional` passes account-path checks
  for panel/Downloads installation and backups, KPackage discovery, the Game
  Drawer patch, and Quick/Control Center removal. Panel responses and service
  commands are simulated; this does not retest live panel layout behavior.
  Panel tools now respect XDG paths and save backups in user data rather than
  the source checkout. Separate desktop-menu checks pass the same three layouts
  with simulated Plasma responses and real backup/config verification.
- `python3 tools/verify-login-portability.py` passes staged login installation
  for two simulated passwd accounts, normal and sudo invocation, and default
  and external configuration paths. `--config-dir` carries a custom desktop
  configuration path through sudo. No system services or user accounts change.
- All Python tools compile. The previously successful native widget and moved
  CMake build tests below were not repeated. The source distribution was refreshed
  with these fixes; the independent widget archives needed no changes.
- These checks exercise account/path portability on the documented Fedora/KDE
  stack. They do not establish compatibility with every distribution, Plasma
  version, permission setup, or a real separate-user login session. Native
  dependencies and the existing-panel requirements of optional panel tools
  still apply. Hardware integration and authentication were not retested.

## Independent widget archives (2026-09-26)

- `tools/build-widget-forks.py` produces four `.tar.gz` archives and one
  installer. Downloads and Separator are QML packages. The animated task
  manager and tray use their own package IDs, QML, configuration schemas and
  normal native applet adapters. Styled tray children and Trash also have
  independent IDs and are bundled in the tray archive (18 packages total).
- `python3 tools/verify-widget-archives.py --functional` passes installation
  into a temporary system prefix, notification selection, preservation of
  stock tray/task-manager IDs and pinned apps, disabled-notification handling,
  incomplete-archive rejection, rollback after a simulated copy failure,
  retirement of shadowing local packages and preservation of unrelated service
  overrides. It does not install files into the running desktop.
- The four top-level forks and the notification fork load with no Sequoia
  shell-wide preload. Native adapter libraries reuse the installed backends.
  Archive installation verifies CPU architecture, Qt version, upstream
  fingerprints and package checksums before modifying files.
- In a private virtual KWin/Wayland session, the animated task manager passes
  hover/reset, animation toggle, styles/intensity, running-dot checks, native
  schema, Apply and all configuration pages. The fork tray passes audio,
  notification and clipboard detail navigation, including clipboard barcode
  navigation and Back.
- The notification fork passes real delivery on a private D-Bus notification
  server, exactly one styled banner, actions, inline reply, close, critical and
  long-text bounds, progress layout, expiry and DND/history. All test settings,
  clipboard data and services are isolated from the user's session.

## Installation portability, legacy export format (2026-09-26)

- `python3 tools/verify-desktop-portability.py` passes real CMake configure,
  build and staged installation after copying and renaming a checkout. Paths
  contain spaces; incremental builds retain their cache, and relocated builds
  preserve the old directory. `bash tools/build-desktop.sh` also succeeds with
  the full desktop sources.
- `python3 tools/verify-widget-bundle.py --bundle dist/sequoia-widgets --native`
  passes checksum verification, source relocation, installation with a temporary
  HOME and XDG directories, both standalone archive installs, same-version
  reinstall, backups and restoration. A missing QML entry point is rejected
  before the existing installation changes. Panel configuration is preserved.
- Both installed applets create visible QML items in offscreen `plasmawindowed`
  with the control-center overlay enabled, on a private D-Bus session with
  service activation disabled. These are isolated directory/session tests on
  this machine, not a retest of the reported guest login or its panel.
- The reported guest widget error was not reproduced. The former installer
  copied packages without checking their entry points; installation now uses
  KPackage and explicitly verifies IDs and QML paths before and after install.
  `tools/install-widgets.py --check` provides installed paths for diagnosis.

## Merkuro Calendar (2026-09-21)

- RPM 26.08.1: local QML override loads with the native executable. Month Light
  Transparent, Week Dark Transparent at 150%, Schedule Light Solid, and Tasks
  Dark Transparent in RTL at 650px height pass navigation and rendering checks.
- Opaque sidebar surfaces, expected window tint (184/255 or 255/255), transparent
  native clear color, and material alpha at multiple outer margins verified.
  Previews inspected; no QML binding/type errors. Software rendering avoids an
  incomplete offscreen OpenGL framebuffer capture after resizing at high DPI;
  the month view was also inspected under real Wayland.
- Preview startup hooks are scoped to the target executable. Calendar previews
  use a distinct AKONADI_INSTANCE, private XDG directories and bus, and stop the
  private instance/process group before cleanup.
- Launcher installation/restoration and stale-source fallback pass. Installed
  wrapper preserves calendar-file URL arguments and enables the Merkuro-only
  interceptor. The current and legacy desktop entries are installed.

## Sequoia Tahoe icons (2026-09-21)

- Qt successfully renders all 14,966 modified SVGs. The bundled theme family
  has no missing or external symlink targets; directory declarations and all
  six global-theme icon selections validate. Upstream GPL-3.0 notices retained.
- Hash comparison confirms 3,721 application, MIME, preference and animation
  assets remain unchanged. Native-size and enlarged light/dark previews reviewed
  in `preview/tahoe-icons.png`, including alert colors and the widened close X.
- `install.sh --prefix /tmp/sequoia-tahoe-install-check` installs all assets and
  successfully generates the three GTK icon caches. Python compilation and
  shell syntax checks pass.
- Live `kiconfinder6` resolves arrow, volume, brightness and copy icons from
  SequoiaTahoe-dark. Plasma was refreshed; the live tray capture confirms heavier
  glyphs. Original MacTahoe theme directories remain available.

## Start menu theme integration (2026-09-29)

- Updated `dist/widgets/start-menu.tar.gz` to version 1.0.2. The menu uses
  Plasma's standard dialog background, blur mask, shadow, theme colors and
  controls. Removed its separate color-mode and opacity overrides. Popup content
  dimensions account for the theme margins to preserve placement and size.
- The profile image is cropped to a circle with a themed user-icon fallback.
- Existing functional checks pass for navigation, search, pinning, launching,
  settings, animations and panel placement. The accelerated offscreen run timed
  out; the standard software run passed, and a live Plasma capture confirmed
  the themed glass background and circular profile photo after installation.
- Upgraded the user-local widget and restarted Plasma. Verified every packaged
  file matches the installed copy, validated the widget archives, and removed
  the temporary extracted package from `dist/widgets`.

## Plasma blur ownership (2026-09-29)

- Found that the application style's Quick-window lifecycle handler replaced
  Plasma's rounded blur region with a whole-window blur request on show/expose.
  The SVG mask itself was correct; further changes to its radius would not fix
  the rectangular blur around floating panels.
- The style now leaves Plasma-owned blur alone, while honoring the Downloads
  fan's explicit `sequoiaNoBlur` opt-out. Other applications retain their existing
  translucent Quick-window blur behavior.
- Extended `tools/verify-window-materials.py` with Plasma-host regression checks.
  The old binary failed with `Style overwrote Plasma-owned blur`; the rebuilt
  binary passes show/exposure, surface recreation, color changes, and Downloads
  opt-out checks for both plasmashell and plasmawindowed. Existing dialog-test,
  Konsole, and Dolphin checks also pass. All 24 rendered panel masks pass.
- Built and staged the style, backed up the active user-local Plasma override
  under `.staging/before-panel-blur-style-20260929-213127`, replaced it atomically,
  and restarted Plasma. Verified that the running shell loads the rebuilt
  library and retains the existing top and bottom panels. The bottom panel uses
  dodge-windows hiding and was hidden in desktop captures, so those captures do
  not provide a visible corner comparison.

## Rounded panel blur containment (2026-09-21)

- `tools/verify-panel-mask.py` checks actual KSvg regions for all eight panel
  assets at bottom-panel, top-panel, and vertical-panel sizes. All 24 masks pass
  at both 100% and 150% scaling: one-pixel inset, rounded corner exclusions,
  and continuous interior coverage. Theme blur stays enabled.
- Generated SVGs are unchanged outside their mask groups: visible surfaces,
  shadows, padding, and opacity are preserved.
- Installed assets match the generated files after Plasma reload; saved panel
  configuration and compositor blur remain unchanged. Live captures are
  `preview/panel-blur-before.png` and `preview/panel-blur-after.png`; the different
  wallpapers limit direct visual comparison.

## Restored translucent sidebar backing (2026-09-21)

- Settings Light Transparent and Info Center Dark Transparent: actual sidebar
  backing color exactly equals the native Window palette, including alpha 184.
  The floating sidebar and header remain opaque; clipping checks pass.
- QWidget::grab repaints the parent underneath the embedded scene, reporting
  alpha 235 in the outer margin. Validation separately checks the actual scene
  backing's alpha rather than treating that composed diagnostic pixel as its
  paint color. This restores the intermediate rendering confirmed by the user.
- Launcher installation/refresh/fallback/restoration checks pass.

## Most-used pages and sidebar base color (2026-09-21)

- Confirmed saved Settings usage history remained present. Selecting the
  existing Default activity restored the live MostUsedModel to six rows;
  its source model reports twelve rows. Live Settings capture visibly shows
  all six shortcuts (`preview/settings-most-used-restored.png`).
- Preview sessions now use private XDG_DATA_HOME and XDG_STATE_HOME. The live
  current activity remains correct after the isolated captures complete.
- Wayland Settings and Info Center Light Transparent captures measure exact
  sidebar outer-margin RGBA (242,243,245,255), matching `#F2F3F5`. Dark Settings
  with 150% scaling, RTL and scrolled navigation measures (37,38,41,255).
- Content retains alpha 184, floating sidebar/header alpha 255, and scrolling
  clips inside its inset viewport. Launcher lifecycle checks pass.

## Sidebar overflow clipping (2026-09-21)

- Real Wayland previews scroll sidebars partway down to expose partial rows.
  Checks require the viewport's clip flag and containment within the ScrollView's
  padded bounds. The capture helper traverses the visual tree to include drawers.
- Passes Settings Dark Transparent, Settings Light Transparent at 150% in RTL,
  Info Center Light Solid, and Discover Dark Transparent at 650px window height.
  Settings capture inspected: partial rows stop above the lower panel edge.
- Sidebar/header opacity and full-height surface checks pass. Discover also
  passes its three-card opacity and 46 highlight containment checks.
- Quick installation, refresh, argument handling, fallback, edit protection,
  and restoration checks pass. Refreshed the installed overlay.

## Settings follows the Discover reference (2026-09-21)

- Supersedes the transparent sidebar below. A single Island spans the shared
  SideBarItem host, including search/back header and category pages. The rendered
  component matches Discover's 5px inset, 14px radius, and opaque surface.
- Real Wayland previews: Settings Light Transparent, Keyboard category Light
  Transparent, Settings Dark Transparent at 150% with RTL layout, Info Center
  Dark Transparent and Light Solid. Inspected Settings and category captures.
- Capture checks require exactly one visible sidebar surface covering the host
  height. Sidebar body, header/body join, and toolbar alpha are 255; transparent
  content alpha remains 184. Solid content alpha remains 255.
- Quick launcher installation, refresh, arguments, fallback, edit protection,
  and restoration checks pass.

## Settings sidebar and content matching (2026-09-21)

- Corrects the earlier opaque sidebar behavior: the shared navigation pages
  now use the content canvas with no added island fill, gradient, or shadow.
- Real Wayland captures of System Settings and Info Center: dark Transparent
  sidebar/content RGBA both (37, 37, 42, 184), light Transparent both
  (243, 243, 245, 184). System Settings Light Solid matches at
  (242, 243, 245, 255); Info Center Dark Solid at (37, 38, 41, 255).
- Preview validation now compares sidebar and content RGB as well as alpha.
  Toolbar alpha remains 255. Captures were visually inspected.
- Discover Dark Transparent regression passes: three opaque cards, 46 contained
  sidebar selection/hover surfaces, opaque drawer, and page alpha 184.
- Quick launcher installation, refresh, stale-version fallback, edit protection,
  argument forwarding, and restoration checks pass. Updated overlay installed.

## Canvas and titlebar tint matching (2026-09-20)

- Controlled native Wayland compositor captures over a uniform RGB (64, 96, 128)
  backdrop exposed a mismatch invisible in raw `grabWindow()` RGBA captures:
  the translucent Quick clear color appeared white while the titlebar appeared
  RGB (193, 202, 213). Painting the same 184/255 tint as a scene rectangle over
  a transparent native clear color gives RGB (193, 202, 213) in both areas.
  Cropped test windows: [before](preview/tint-composited-before-detail.png) and
  [after](preview/tint-rectangle-detail.png).
- Real Discover's Plasma Widgets category passes native Wayland capture checks:
  solid cards/sidebar, page alpha 184/255, and a full-window scene tint with a
  transparent native clear color. Dark Transparent System Monitor also passes.
- Solid Discover retains fully opaque canvas, cards, sidebar, and native clear
  color. Preview checks now assert the scene tint's size and alpha separately
  from the native window clear color. Decoration checks compare titlebar RGBA
  against the selected material's Window palette.
- Quick install/refresh, system-module version fallback, edited-launcher
  protection, and restoration checks pass. Refreshed the installed overlay.

## System Settings canvases and dialog blur (2026-09-20)

- Real System Settings on Wayland: light Transparent canvas alpha 184/255,
  sidebar/header 255/255; dark Transparent also passes at 150% scale. Solid
  canvas/sidebar/header all remain 255/255. Captures were visually inspected.
- The actual Colors KCM grid page also measures 184/255 on its unframed canvas.
  Canvas checks now reject zero-size settings pages and detect stacked tint
  (formerly 235/255), rather than merely checking for some transparency.
- A Wayland protocol trace confirms `ext_background_effect_surface_v1.set_blur_region`
  requests on the System Settings surface. KWin reports its Blur effect loaded.
- `verify-window-materials.py` interposes only the KWindowEffects call in an
  isolated test process. It verifies actual style requests and pixel alpha for
  parented/unparented dialogs, tool windows, popups, masks, reopening, Quick
  surface recreation without native controls, switching to opaque Quick colors,
  embedded scenes, and style cleanup. Generic, Konsole, and Dolphin app identities
  pass. This test checks requests; the separate Wayland trace checks delivery.
- Discover retains solid card/sidebar surfaces with canvas alpha 184. Existing
  toolbar/scrolling/docking/RTL checks and Quick install/restore checks pass.
- Full System Settings still crashes during offscreen platform initialization;
  its working native Wayland path is used for validation. Offscreen embedded
  KCMs can produce an empty ColumnView; the preview now detects that condition.
- Installed the compiled plugin and refreshed the existing Quick launchers.
  Installed/staged plugin hashes match. Backup location is in `DEPLOYMENT.md`.

## Plasma artwork and color schemes

- All 44 SVGs load through QtSvg and have unique element IDs.
- KSvg rendered light and dark component previews; corners, shadows, highlights,
  and surface transparency were visually inspected.
- Desktop widget backgrounds include the `blurred` and `blurred-mask` frames
  required by Plasma's installed BasicAppletContainer. The blurred surface uses
  a 32% white tint in light mode and 50% charcoal tint in dark mode, with a 16px
  corner radius and matching rounded mask. White rims use 50%/22% opacity. Solid and
  opaque variants retain their unblurred fallback.
- The glass comparison renders actual generated KSvg frames over a sample
  wallpaper using Qt Quick's accelerated blur. Example controls illustrate the
  material; individual widgets retain their own control styling.
- Normal surfaces use 76% opacity; translucent surfaces use 64%. Opaque and solid
  fallbacks remain opaque. Shadow interiors stay transparent, and corner/edge
  alpha matches within 2/255 at their joins.
- Task states retain compact insets and distinguish focused, running, minimized,
  and idle pinned apps. Expanded panel widgets use neutral gray/white highlights.
- Both Global Theme IDs match their package names. Their defaults reference the
  generated color schemes, Plasma styles, independent application style and
  decoration, and bundled splash. They do not replace icons or panel layouts.

## Application style and decoration

- The source copy is separate from `~/Darkly`, excludes its Git history, retains
  its upstream license/copyright notices, and uses independent names/settings.
- Release builds of the Qt 6 style, configuration modules, settings application,
  and KDecoration3 plugin completed successfully.
- `QStyleFactory.create('SequoiaEleven')` succeeds. Real Qt widget galleries were
  rendered in both light and dark palettes, including tabs, fields, buttons,
  checkboxes, a slider, table, and progress indicator.
- KDE's native decoration preview engine successfully loads
  `org.kde.sequoiaeleven`. Its light/dark previews show rounded frames, shadows,
  centered captions, and colored controls. The sample preview engine supplies
  its own button arrangement; live activation configures close/minimize/maximize
  on the left.
- The title bar now reserves two small-spacing units above the caption and
  controls (previously one). Border height, caption position, and button offsets
  share this metric, including screen-edge button hit areas. The rebuilt plugin
  renders successfully in both native decoration previews using the current
  Adwaita Sans title font. This padding update is staged; system installation
  requires `sudo cmake --install build/desktop`.

## Floating application islands

- The updated Qt 6 plugin builds and stages successfully. Light and dark previews
  use the staged plugin with isolated configuration and show an actual toolbar,
  dock sidebar, and inset table alongside the existing widget gallery.
- Toolbars, docked panels, and framed views use 5px surface insets, up to 14px
  corner radii, a fine outline, and soft shadows. Content frames reserve 12px
  around child viewports. Standalone KDE-marked frameless navigation lists gain
  a frame; lists inside docks share the dock's surface.
- `python3 tools/verify-application-islands.py` passes for toolbar clicks,
  overflow access, scrolling to the last item, narrow windows, RTL layout,
  vertical/stacked toolbars, dock detach/reattach, and restoring style-owned
  margins/frames when switching to Fusion. It also passes at 150% scale.
- Offscreen checks do not verify live compositor blur or every application's
  custom sidebar implementation. This update is built and staged locally;
  the installed system plugin requires reinstalling and restarting applications.

## Qt Quick/Kirigami application islands

### Locked Okular sidebar containment

- Reproduced Okular's locked sidebar with a generated PDF in an isolated session.
  Its zero-height custom dock title caused Qt to ignore the dock's top margin,
  leaving the tab strip above the theme's inset surface. The style now reserves
  the missing top space inside the content layout after Qt positions the dock.
- The real window now places the tab strip 12px below the dock's top, inside the
  surface's 5px inset. Regression checks cover narrow and RTL layouts, floating
  and re-docking, and restoration of original content margins on style changes.
- Captures: `preview/okular-sidebar-before.png` and
  `preview/okular-sidebar-fixed.png`, with widget geometry trees alongside them.

- Generated local overrides from the installed KDE components. Drawers and page
  headers use 5px insets, 14px corners, subtle rims, and shadows; cards use matching
  corners. Framed scroll views reserve 12px for content. System Monitor's
  zero-margin table receives padding, and System Settings' navigation/form pages
  receive explicit floating scroll surfaces.
- Captured real System Monitor windows in light/dark mode using accelerated
  offscreen rendering. Native pointer clicks selected Applications and returned
  to Overview; this also passed with RTL layout at 150% scale.
- Discover and System Settings loaded the shared overrides and were visually
  checked on Wayland in temporary, automatically closed test windows. System
  Settings' touchpad module crashes with the offscreen platform before loading
  the UI, so its preview must use `--platform wayland`. Discover's accelerated
  offscreen capture showed artifacts; its final capture also uses Wayland.
- Discover's custom drawer now reserves 12px around its navigation viewport and
  scrollbars, keeping hover and selection surfaces inside the 5px island border.
  The real-app preview checks all 46 sidebar style surfaces against the border;
  this passed in light mode and in dark mode with RTL layout at 150% scale.
- `python3 tools/verify-quick-islands.py` verifies installation, refresh, paths
  containing spaces, argument forwarding, import-path precedence, fallback after
  source changes, protection of subsequently edited launchers, and exact restore.
- Installed user-local launchers for System Monitor, System Settings, and Discover,
  including their desktop actions. No global QML environment override or system
  component replacement is used. New menu launches load the extension; existing
  instances must be fully closed first.
- The generic components do not override every application-defined layout. In
  particular, apps can choose their own drawer content spacing. GTK, Electron,
  and sandboxed runtime copies are not covered. All settings pages and app
  workflows have not been exhaustively exercised.

## Solid and Transparent application variants (2026-09-20)

- All six selectable schemes, the bundled legacy palette, and the embedded
  Plasma palettes now set `[KDE] frameContrast=0`. KDE's native
  `KColorScheme::frameContrast()` confirms 0.000 for each scheme. Normal and
  alternate view backgrounds match their window equivalents in RGB; content
  views are opaque while Transparent canvases use 184/255 alpha. Style-owned island/card rims and
  decorative window/widget outlines honor zero contrast.

- Generated and staged Light/Dark × Solid/Transparent color schemes and Global
  Themes. `verify-materials.py` checks a complete installation into a path with
  spaces, pairing each scheme with its decoration and preserving legacy entries.
- Built separate native `org.kde.sequoiaeleven.solid` and
  `org.kde.sequoiaeleven.transparent` decorations from the shared source. Both
  follow the active light/dark colors and retain the existing controls/geometry.
  Native decoration previews measured titlebar alpha 255 for Solid and 184 for
  Transparent in both palettes.
- Real Qt widget captures measured Solid canvas/toolbars/sidebars at alpha 255;
  Transparent canvas at 184 and toolbar/sidebar/content/group/tab-page interiors
  at 255 in both light and dark palettes. Surface painting is independent of
  window alpha; transparent preset islands also ignore legacy bar/sidebar opacity.
  Text/button palette colors remain opaque. Material presets do not overwrite
  the user's custom legacy style configuration.
- Real Discover captures confirmed page alpha 184 and solid drawer/header surfaces
  (254–255 after scene-graph rendering), with Solid measuring 255 throughout.
  Discover and System Monitor card interiors measure 255. The preview checks
  allow one alpha step for scene-graph rounding. The 46 sidebar selection/hover surfaces remain inside
  the border. Quick windows request alpha buffers before creation and compositor
  blur when their background is translucent.
- The earlier Transparent implementation passed native Wayland widget rendering
  and Discover RTL at 150%. This solid-island update passes light/dark native-scale
  previews, including Discover RTL. Its offscreen OpenGL capture at 150% produced
  an incomplete frame (missing drawer pixels); fractional-scale rendering of the
  updated Quick layer still needs a live desktop check.
- Installation/restoration, source-version fallback, toolbar clicks/overflow,
  scrolling, RTL, stacked/vertical toolbars, dock transitions, and style cleanup
  checks pass. Login palette selection recognizes all four material scheme names.
- Application-defined opaque content can still cover the transparent background.
  Compositor blur depends on KWin's Blur effect. GTK/Electron and isolated Flatpak
  runtimes do not inherit the native Qt implementation.

## Typography

- Adwaita Sans upright/italic variable fonts were copied unmodified from the
  installed Fedora package, including its SIL OFL license. The bundled source
  manifest records the package, source paths, upstream project, and checksums.
  The previously bundled Nunito files remain for clock and splash display text.
- Qt resolves real Adwaita Sans regular, medium, semibold, bold, and italic faces;
  required menu text, digits, currency, and punctuation glyphs are present.
- Desktop/menu/toolbar text remains 10pt, small labels 9pt, and window titles
  become 10pt medium. The sample menu text grows by about 19px (4.6%) compared
  with Nunito; the comparison preview fits the requested menu labels.
- Fontconfig resolves antialiasing enabled, slight hinting (1), and grayscale
  rendering (rgba none/5). No font aliases, forced DPI, monospace substitutions,
  panel geometry, or menu spacing changes are introduced.
- The focused font installer snapshots the prior KDE/GTK settings, font rule,
  installed fonts, and installed theme defaults. Staged settings were diffed
  before applying; unrelated theme preferences are preserved.

## Lock screen

- The redesigned shell loaded in the actual greeter's **offscreen testing mode**.
  Its journal was checked for missing components and QML type/reference errors.
- A captured 800×800 preview exercises the clock, avatar, password field, steady
  unlock/session actions, footer, and media card with sample album artwork.
- A dedicated integration check on an **isolated D-Bus session** discovered a mock
  MPRIS player, displayed its track/artist and playback state, and verified that
  Previous, Play/Pause, and Next reach the player in order. Real players were not
  controlled by this test.
- The original Plasma authentication and password synchronization handlers are
  retained. Interactive authentication, fingerprint, and smartcard flows were
  not exercised by the visual test.
- The password row is capped at 360px, with a 36px-high field and matching
  36px-square unlock button. The media card is 392×156px (previously 432×174px),
  centered below the field with 64px artwork and 36px-high playback controls.
- The password field has a constant 1px antialiased white border at 70% opacity.
  The avatar is 6 grid units across, with extra list height preserving label spacing.
- Separate light/dark greeter previews use isolated settings, without changing
  the active desktop. The media card follows the Window palette independently
  of the lock screen's Complementary palette: pale background/dark text in light
  mode, charcoal background/light text in dark mode. Both loaded without QML
  errors, and MPRIS discovery and playback checks passed after the resize.
- The lock-screen copy must be refreshed with the installer after Plasma updates.

## System Settings usage history diagnosis (2026-09-20)

- The empty “Most used pages” section reproduced with both stock Breeze and
  Sequoia Eleven in the real Quick Settings KCM. Its repeater had zero entries;
  the styling was not hiding any delegates.
- The activity database contained history for 32 System Settings pages, but the
  live KDE ActivityManager returned an empty current activity despite listing
  the existing Default activity. Selecting Default in an isolated session with
  a copy of that database restored the page buttons under the custom styling.
- Restored the existing Default activity through ActivityManager's supported
  `SetCurrentActivity` method and verified `CurrentActivity` returned its ID.
  No application-style patch or history-database edit was needed.
- Full offscreen System Settings startup hit an unrelated crash in the installed
  touchpad module; the comparison used `kcmshell6 kcm_landingpage` to run the same
  landing page without loading unrelated settings modules.

## Adaptive wallpaper

- Original preview geometry and colors are preserved in editable SVG sources,
  with matching 3840×2160 light/dark PNGs in one `Wallpaper/Images` package.
- Plasma's native `MediaProxy`, running offscreen on an isolated D-Bus session,
  resolved the light, dark, and automatic package URLs to the expected bundled
  images. The automatic URL has no forced-color or day/night fragment; installed
  Plasma configuration documents this as following the active color scheme.
- Both Global Themes reference `SequoiaEleven` in their wallpaper defaults.
  The full desktop installer applies the package and snapshots the previous
  desktop wallpaper configuration; `--keep-wallpaper` skips activation.
- Artwork installation and reinstallation passed in an isolated prefix with
  spaces. PNG decoding, package metadata, defaults, shell/Python syntax, and
  wallpaper/source archive contents were checked. No live desktop was changed;
  a live color-scheme transition and multi-monitor behavior remain untested.

## Merkuro collapsed sidebar — 2026-09-21

- The collapsed width now includes the drawer's left and right island padding;
  the 12px margins no longer consume the navigation icons' available space.
- `preview-quick-islands.py --app merkuro --collapse-sidebar --material transparent`
  passes in light mode. The preview clicks the real collapse control and checks
  that all four navigation icons have usable dimensions and fit inside their
  buttons and every clipping ancestor.
- The same check passes with `--calendar-view tasks --dark --material solid
  --scale 1.5 --rtl`, including navigation to Tasks after collapse. Both captures
  were visually inspected; material opacity and QML error checks also pass.
- Python syntax checks pass for the modified builder and preview tool.

## Native settings sidebar containment — 2026-09-21

- Reproduced the Konsole-style settings list with the installed KDE
  `KPageDialog`: a 91px delegate was clipped by a 78px viewport. KDE's fixed
  list width did not include the theme's 12px content margins on both sides.
- The style now adds only the missing width after viewport layout. The original
  fixed width is restored when switching styles.
- `python3 tools/verify-settings-sidebar.py` and its `--dark --scale 1.5`
  variant pass. The check fails against the original plugin. It verifies complete
  row bounds, navigation clicks, vertical scrolling, RTL, larger fonts, label
  changes, and style cleanup using KDE's real settings widget and delegate.
- Light and dark captures were visually inspected. The existing
  `verify-application-islands.py` interaction and containment checks also pass.

## Deployment and limits

- The activation tool snapshots the affected settings and installed local artwork
  before changing them. A restoration tool restores that snapshot; independently
  named system plugins remain installed but unused after restoring.
- Live activation confirmed: KWin's support information reports
  `Plugin: org.kde.sequoiaeleven`, both Global Themes are listed, and a fresh Qt
  application using KDE platform integration selects `sequoiaeleven`.
- Python tools pass syntax checks. The generated source archive includes the
  application/decoration source and its license, original artwork, lock-screen
  components, and build/install/preview tools.
- Desktop wallpaper blur requires accelerated Qt Quick rendering and a widget
  with StandardBackground. Backgroundless widgets keep their chosen appearance.
- Software previews do not reproduce KWin's native blur. Album artwork falls back
  to an unmasked image in software mode; GPU rendering uses rounded masking.
- Fractional scaling, multiple-monitor lock-screen layout, and all third-party
  widgets still require observation in the live desktop. The application style
  targets Qt 6 widgets; Qt 5, GTK, Electron, browser content, and isolated Flatpaks
  can retain independent application styling.
# Plasma Login appearance — 2026-09-20

- Detected the active display manager as `plasmalogin.service` and checked the
  matching KDE Plasma 6.7 source for supported settings and appearance sync.
- `python3 tools/verify-login.py` passes: inherited KDE defaults, explicit
  light/dark/light switching, matching palettes/fonts, preservation of unrelated
  greeter settings, idempotent writes, and retention when another theme is used.
- Both `tools/preview-login.py --mode light` and `--mode dark` load the installed
  `/usr/libexec/plasma-login-greeter --test` without QML errors and capture the
  real controls. The preview adds only a temporary capture hook and wallpaper
  backdrop; authentication is mocked and power operations are unavailable in
  the isolated offscreen session. Preview PNGs were visually inspected.
- Python compilation and `systemd-analyze verify` pass for the new tools/units.
- System installation completed through administrator authentication, with
  backup `/var/backups/sequoia-eleven-login/20260920-082042-917532`.
- The path unit is enabled and active (waiting). The oneshot service is enabled,
  completed with `0/SUCCESS`, and reports `Sequoia Eleven Light`, matching the
  desktop. Installed helper and palettes are root-owned, mode 0644.
- `/etc/plasmalogin.conf` selects the globally readable adaptive wallpaper
  package and native image plugin. Adwaita Sans resolves through fontconfig.
- No live login, logout, authentication attempt, or display-manager restart was
  performed. The next greeter startup is the remaining real-session visual check.


## System tray Control Center — 2026-09-21

- Installed a user-local dashboard for the existing tray; the panel's application
  icons and widget IDs are preserved. The overflow arrow is now a sliders icon.
- The deterministic QML fixture passes Space-key activation for Wi-Fi, Bluetooth,
  Do Not Disturb, microphone mute, speaker mute and play/pause. Keyboard volume
  changes reach the backend, and later external volume updates still reach the slider.
- Dark rendering and light rendering with larger text/RTL were captured using the
  GPU renderer. The live backend preview initializes without dashboard QML errors.
- The actual system tray, hosted by plasmawindowed with a temporary configuration
  and private bus, opens the dashboard, navigates to native audio details and
  returns to the dashboard successfully. No actual hardware toggles or session
  actions are invoked by automated tests.
- Hardware availability is explicit. PowerDevil must expose an adjustable display
  for a brightness slider. The native device pages remain accessible.
- A version-checked launcher scopes the entry-point helper to plasmashell and
  falls back to the normal tray after a Plasma library/plugin change. Installation
  and removal are documented in README.md and plasma/control-center/README.md.

## Control Center detail menus — 2026-09-22

- Native audio, brightness, battery, Bluetooth, network, notifications,
  clipboard, removable-device, printer and camera pages open with the new
  surfaces and return to the compact dashboard. Their existing models and
  actions remain native; tests do not change hardware settings.
- Light and dark/13pt/RTL captures verify page sizing and surface colors,
  including the audio page's first-open background palette.
- Clipboard opens a sample barcode and invokes its native Back action before
  returning home. No clipboard content is changed by this check.
- Existing tray visibility-policy tests and native media tabs/artwork/Back
  regression checks pass. Python installer/check scripts compile successfully.
- Native checks isolate configuration, data, cache, state and session D-Bus.
  They register a dummy PowerDevil service name solely to load power/brightness
  pages; actual power services and hardware mutations are not exercised.
- Installed files and source fingerprints match the staged build. Multi-display
  brightness delegate construction now checks neighboring items before linking
  keyboard navigation, avoiding an upstream startup null dereference.

## Weather floating cards — 2026-09-22

- Day and night have separate padded forecast cards; a daily-only provider uses
  one card with both high and low temperatures. The native weather backend and
  unit conversion remain intact.
- Current conditions, details and notices have rounded card surfaces. Notice
  text wraps within 16px padding; the outer viewport scrolls long pages.
- Light and dark/13pt/RTL fixtures pass table dimensions, tab switching, card
  content bounds, zero-valued details, daily-only, and unavailable/loading checks.
- The real KDE weather applet loads through the staged tray and returns to the
  dashboard. The installed overlay matches the stage; the reloaded shell reports
  no Control Center QML errors. Fixtures use sample data and no weather requests.

## Contacts menu — 2026-09-22

- Native Merkuro applet entry point and backend imports retained; only the exact
  Merkuro entry URL joins the managed bridge and plugin fingerprint manifest.
- Sample-data checks cover filtering, no results, keyboard open, long profile
  fields, QR sharing and search retention after Back in light and dark/large/RTL.
- Hardware-rendered dark preview confirms monochrome action icon contrast.
- All twelve native menus and Back navigation pass, including two nested
  Contacts pages. Native checks use a separate Akonadi instance; sample profile
  data is injected into the view instead of fetching a nonexistent contact ID.
- Installed overlay bytes and plugin fingerprints match the verified stage.

## Desktop repair (2026-09-23)

See [desktop repair findings and validation](DESKTOP-REPAIR.md) for Tahoe app
aliases, Activity restoration, the Contacts login hang fix and preview isolation.

## Control Center icon animation (2026-09-23)

Added hover and popup-state slider motion, using Kirigami animation durations.
Installed the icon and expander binding after verifying intermediate animation
values, endpoints and Wayland renders. The native tray/dashboard/navigation
check passes. Plasma was restarted to load the update.

## Search focus border (2026-09-23)

The native Plasma TextField expands its hover/focus overlay by the SVG frame
margins. Added prefix-specific `hint-no-border-padding` to the line-edit hover,
focus and keyboard-focus frames in both theme variants. The base keeps 6px text
padding. This also corrects other native Plasma text fields using these assets.
The generator preserves the change. Native SearchField renders verify exact
overlay/base alignment in Light and Dark (`preview/search-highlight-*.png`).
Installed the two SVGs with backups under `.staging/before-search-highlight-*`
and restarted Plasma.

KSvg's [frame implementation](https://github.com/KDE/ksvg/blob/master/src/ksvg/framesvg.cpp)
supports this hint per prefix; zero-size margin rectangles are invalid and fall
back to the artwork's border size.


## Login-style lock screen (2026-09-30)

- The generated shell retains Plasma's authentication connections, password
  synchronization, grace-lock handling, keyboard loader, and passwordless unlock.
- `python3 tools/verify-lockscreen.py`: 11 checks passed with mock authentication
  and session actions, including keyboard entry/submission, password reveal/clear,
  hidden-screen submission suppression, user switching, failure messages,
  passwordless layout, and light/dark geometry at 640×480, 800×600, 1280×720,
  and 1920×1080. This does not authenticate against PAM or trigger power actions.
- `python3 tools/verify-media.py`: isolated MPRIS discovery, metadata, previous,
  pause/resume, and next passed.
- Both appearances loaded in the real greeter's offscreen testing mode without
  QML errors. Preview PNGs are in `preview/lockscreen-light.png` and
  `preview/lockscreen-dark.png`. The preview accepts `--size WIDTHxHEIGHT` and
  `--renderer software|opengl`; the effective graphics API is recorded in its log.
- The offscreen greeter selected the software graphics API even when OpenGL was
  requested; the accelerated glass effect still needs a live-renderer visual check.
- A real interactive unlock, physical fingerprint/smartcard input, and multi-monitor
  virtual keyboard placement were not exercised by these automated checks.


## Accent-aware macOS/iOS-style switches (2026-09-30)

- Native Plasma switches use generated `widgets/switch.svg` assets in both
  project themes. Active tracks and focus rings use `ColorScheme-Highlight`.
  The themes omit their old fixed palette override so the account palette and
  custom accents reach KSvg; named application color schemes remain available.
- The existing Qt Quick overlay replaces only `org.kde.desktop`'s visual switch
  indicator. Native Switch and SwitchDelegate retain their input behavior.
- `python3 tools/verify-switches.py`: 18 QtTest passes across light/dark settings,
  covering clicks, keyboard, disabled input, drag, focus, RTL, and live accent
  bindings. Native Plasma captures matched configured blue and purple accent
  pixels. Previews are `preview/switches-light.png` and `preview/switches-dark.png`.
- `python3 tools/verify-quick-islands.py`: installation/refresh, argument forwarding,
  stale-version fallback, local-edit protection, and restoration passed.
- `python3 tools/verify-theme-installer.py`: 10 passed, one optional integration
  test skipped. The switch refresh test verifies repeatability, preservation of
  unrelated receipt entries, local-edit protection, and removal.
- `tools/install-switches.py` updates only switch files and obsolete palette
  overrides in an existing tracked installation, preserving ownership/backups.
  `./install.sh` and `tools/build.py` include the same change for fresh installs.


## Menu toggle inheritance and installer integration (2026-09-30)

- Qt Quick binary menu actions share the switch indicator. Native KDE menus paint
  a matching 42×24 accent-colored track and inset white thumb. Native QAction
  activation, submenu arrows, shortcuts, RTL layout, and exclusive radio actions
  remain intact. Radio/action-group detection in Quick menus follows KDE's
  existing `autoExclusive`, attached ActionGroup, and ButtonGroup rules.
- `python3 tools/verify-switches.py`: 22 QtTest passes across light/dark palettes,
  including menu activation, accent bindings, RTL, and exclusive-group behavior.
- `python3 tools/verify-menu-toggles.py`: native menu accent pixels, click/keyboard
  activation, disabled actions, exclusive actions, and RTL passed in both palettes.
  Captures: `preview/menu-toggles-light.png` and `preview/menu-toggles-dark.png`.
- `SEQUOIA_FULL_INSTALL_TEST=1 python3 tools/verify-theme-installer.py`: all 11 tests
  passed, including real `install.sh` builds, installation, repeat installation,
  edit protection, and uninstall in temporary user/system destinations. Assertions
  verify the installed Quick menu indicator and both accent-aware Plasma assets.
- The standard installer already builds the native application style and the
  Quick overlay, so these changes are included without a separate manual step.
  `tools/install-switches.py --native` supports targeted refresh of the staged
  native style while preserving the system installation receipt.


## System Settings navigation — 2026-09-30

Built KDE System Settings 6.7.5 with category navigation around the unchanged
ModuleView and installed KCMs. The additional build libraries were extracted to
`/tmp/sequoia-settings-sdk/usr`; no system package installation was performed.
The executable has no temporary SDK RPATH/RUNPATH.

Passed `tools/verify-settings-navigation.py` in a private virtual Wayland
compositor and D-Bus session, with separate XDG configuration/data/cache/state/
runtime directories:

- Exact stock/fork discovery equality and complete navigation/search coverage
  for all 77 installed, platform-eligible settings modules.
- Dark theme plus a synthetic external module in an unknown vendor category:
  all 78 entries retained, including successful external-page activation.
- Native Colors (QML) and Keyboard (QWidget) page hosting; original module
  loading, Apply/Reset/Defaults/Help code remains unchanged.
- Unsaved-change Cancel blocks Back, category changes, and search; the module
  and previous query remain intact. No settings were applied during testing.
- Startup deep link to Colors, real category-card clicks, search field updates
  and clearing, literal punctuation in searches, compact layout, RTL, and 150%
  scaling.
- Temporary install/reinstall/removal restores a previous executable launcher
  byte-for-byte and preserves its permissions; module arguments including spaces
  are forwarded; changed upstream fingerprints select the stock application.
- Installed account launcher tested again with the existing Quick theme overlay:
  the complete 77-module check passed. Plasma's live PATH was checked and places
  `~/.local/bin` before `/usr/bin`.

GUI testing uses Wayland because the distribution touchpad module crashes while
constructing module metadata under Qt's offscreen backend. The initial test
assertion was also corrected to accept upstream's `QQuickWidget::setContent()`
path, which has a root object without a `Ready` source status. Final logs contain
no QML loading errors. Representative modules were opened; privileged writes and
every possible hardware operation were not exercised.

Artifacts: `preview/settings-navigation-light/`,
`preview/settings-navigation-dark-third-party/`, and
`preview/settings-navigation-light-rtl-1.5/`. The light captures include the final
installed launcher and overlay. Source provenance and reversible removal are in
[apps/settings/README.md](apps/settings/README.md).


### Settings divider removal — 2026-09-30

Removed the sidebar edge separator, replaced its two horizontal dividers with
non-painting spacers, and omitted the native QWidget header's painted bottom
line. ModuleView changes are cosmetic; module hosting and change handling are
untouched. Rebuilt successfully and passed the existing settings navigation
verification (`--skip-install-check`): 77 modules, search/category coverage,
native QML/Widgets pages, and unsaved-change cancellation. Inspected the updated
home screenshot.


### Quick Settings sidebar shortcut — 2026-09-30

Pinned the discovered `kcm_landingpage` module beneath Home in the sidebar and
highlight its shortcut while it is open. The same module remains available in
System & Accounts, All Settings, and search. Its native theme controls and all
other controls remain unchanged. Rebuilt and passed the existing navigation
checks with `--startup kcm_landingpage --skip-install-check` (77 modules).
Inspected `preview/settings-navigation-light/startup.png`, confirming light,
dark, and automatic theme choices on the original Quick Settings page.


### Quick Settings floating panel — 2026-10-01

Added an inset rounded surface and soft shadow around Quick Settings, including
its native header, scrollable content, and action buttons. Made the embedded
page backgrounds transparent and hid its header/form/footer separators while
retaining layout spacing and native controls. The surface follows the active
module and leaves other modules' presentation unchanged.

Rebuilt successfully. Existing navigation verification passed with
`--startup kcm_landingpage --skip-install-check` in dark mode, then through the
installed account launcher in light mode with its existing Quick theme overlay.
Both runs retained all 77 modules and passed search/category coverage, native
QML/QWidget hosting, real navigation, and unsaved-change cancellation. Inspected
both startup captures to confirm the continuous rounded panel and hidden lines.
Tests used isolated configuration and did not apply live settings. Installed
the updated per-user executable; close and reopen System Settings to load it.


### Appearance floating panels — 2026-10-01

Extended the Quick Settings surface to all 13 discovered Appearance modules,
using the navigation category classifier rather than a separate hard-coded
module list. Header, page, form, and footer separators are hidden within those
hosts. Removed the extra AbstractKCM view background and made the native Font
Management header transparent. Embedded controls, previews, editor surfaces,
and module behavior remain native. Child-item changes schedule a coalesced
presentation refresh for dynamically created headers and subpages.

Built and installed the updated per-user executable. Verification opened and
captured all 13 Appearance modules in dark mode and through the installed
launcher with the existing light Quick overlay. Both runs passed 77-module
coverage, category/search navigation, native hosting, and unsaved-change Cancel.
The installed run also opened Window Decorations' Titlebar Buttons subpage,
requested a smaller Fonts window (subject to native minimum sizing), and checked
Quick Settings again. Inspected the resulting screenshots in
`preview/settings-navigation-{light,dark}/`. No settings were applied.

Added `--capture-group` to the existing GUI verification tool for category-wide
visual checks. The preload hook now ignores native Qt helper processes started
by modules such as Font Management, which inherit the test environment.
Installed and built executable hashes match; no temporary SDK RPATH/RUNPATH.


### Floating panels across every category — 2026-10-01

Made the inset rounded surface the default for the shared ModuleView host,
removing the Appearance/Quick Settings restriction and category-specific state.
All native module headers and action footers now share the surface, including
the host for external applications. QML pages and dynamically created subpages
use transparent page layers; empty full-content background rectangles (including
Power Management's extra rectangle) are hidden. Native QWidget horizontal and
vertical line frames are hidden while retaining their layout space. Interactive
control frames, previews, editor surfaces, and status overlays remain intact.

The dark run with `--capture-group all --third-party` opened and captured all
77 installed modules plus the synthetic external module. Category/search
coverage, native QML/QWidget hosting, unsaved-change Cancel, real card navigation,
and the titlebar-button subpage passed. The category capture harness now discards
initialization changes through KDE's native Discard action when needed (Spell
Check marks its initial defaults dirty in an empty profile), never Apply.

Verification uses a private Wayland compositor and isolated XDG directories.
Some pages display their existing missing-service or missing-hardware states in
this environment, including Power Management and Sound; these captures verify
page presentation rather than the unavailable service's operations. Installed
the updated per-user executable and confirmed its hash matches the built binary.

The final installed-launcher run with the existing light Quick overlay also
opened and captured all 77 modules and passed the same navigation checks.
Inspected representative captures from all eight categories, native divider
removal, Quick Settings, and the existing Appearance subpage/compact captures.
Final captures and logs: `preview/settings-navigation-light/` and
`preview/settings-navigation-dark-third-party/`. No QML loading errors were
reported. No settings were applied to the live account.


### Application source organization — 2026-10-01

Moved the System Settings sources and launcher from `settings/` to
`apps/settings/`, establishing `apps/` for application-specific overrides.
Updated build, installer, verification, and documentation paths. Regenerated
the existing staging CMake cache for the new source location and successfully
rebuilt using `tools/build-settings-navigation.py`. Python syntax checks and
temporary installation/removal passed, including argument forwarding and
restoration of a previous launcher. The installed desktop application does not
depend on the source folder location and required no reinstallation.


### Gwenview override baseline — 2026-10-01

Created `apps/gwenview/` with the official Gwenview 26.08.1 source archive,
matching the installed Fedora package. Recorded source provenance and the
downloaded archive's SHA-256 in `override.json` and `upstream.sha256`; preserved
upstream licensing and translations. Verified every extracted regular file
against the archive and parsed the XMLGUI toolbar/menu definition. Added a
technical source map for the forthcoming presentation work. This is an
unmodified source baseline: no custom Gwenview build or live activation yet.

### Gwenview welcome page and recent images — 2026-10-01

Replaced the recent-files/folders startup grid with a centered Add files and
image drop target. Replaced the folder-navigation sidebar with recent-image
thumbnails using the existing history model and thumbnail provider. Information,
Operations, native browsing/viewing, and editing controls remain available.
The picker accepts multiple images; duplicate URLs are removed, identical names
from different directories remain distinct, and history stores original paths.

Built the executable and matching private library against installed Qt/KF6,
retaining RAW, FITS, Baloo, annotation, sharing, color management, and Wayland
gestures. The development SDK was assembled under `/tmp`; runtime binaries use
only `$ORIGIN` and installed system dependencies, with no SDK runtime path.

`tools/verify-gwenview.py` passed on a private session bus with isolated XDG data
and disposable images. Verified empty startup, multiple-selection picker,
rejection of text drops, image drag/drop, navigation/wrapping through two images
with identical filenames, URL deduplication, MRU ordering, native rotation and
saving to the original image through a temporary link, recent-thumbnail
activation, history persistence across restart, and disabled-history behavior.
Inspected welcome and viewer captures under `preview/gwenview/`.

Temporary installation/removal passed with a pre-existing launcher and paths
containing spaces. Verified argument forwarding, fallback after a changed stock
binary, and private-library resolution without any development SDK dependency.
Installed the reversible per-user launcher, desktop entry, executable, and
library. Validated installed artifact hashes and the desktop entry, and refreshed
the application cache. Existing windows must be closed and reopened to use this
build. No live image files were opened or edited during verification.

### Gwenview unified sidebar island — 2026-10-01

Moved the sidebar's native tab widget inside one painted, rounded container.
The 16-pixel corner radius and outer inset surround the Recent images header,
thumbnail content, and all three bottom tab buttons. The thumbnail viewport is
transparent; tab buttons paint only their rounded selection/hover state. The
surface uses the current theme's opaque raised-panel color, so it is white in
the light theme and a lighter charcoal in the dark theme. Information and
Operations share the same container, with their existing controls preserved.

Rebuilt Gwenview and captured the empty sidebar plus all three pages under the
project's light and dark application styles. Native clicks switched all three
tabs successfully. Inspected the resulting images in
`preview/gwenview-sidebar-light/` and `preview/gwenview-sidebar-dark/`.
Installed the updated build through the reversible per-user installer. Existing
Gwenview windows must be reopened to load the updated sidebar.

### Gwenview recent-image spacing — 2026-10-01

Changed only the recent sidebar's preview cells from square to 4:3: at the
existing 144-pixel preview width, the image area is 108 pixels tall instead of
144. This removes 36 pixels from each row. Thumbnails retain their aspect ratio
and fit within the shorter cell without cropping. Rebuilt Gwenview and used the
sidebar preview with two recent images to inspect spacing and tab operation.
The two-thumbnail capture confirmed the tighter layout; all three sidebar tabs
remained usable. Installed through the existing reversible per-user installer.

### Gwenview rounded preview grid — 2026-10-01

Replaced the recent sidebar's full-image/caption delegate with a square preview
grid. Tiles are center-cropped without stretching, clipped to 7-pixel rounded
corners, and separated by 8-pixel gaps. Columns respond to viewport width;
filenames and paths remain in tooltips and model labels. Native thumbnail
loading, recent history, activation, and context-menu actions are reused.

Built successfully. Light and dark sidebar verification passed using six images
covering portrait, landscape, square, panoramic, and small inputs. Verified
matching square cell sizes, two columns at default width, additional columns
when widened, native mouse activation opening the full image, and all sidebar
tab buttons. Inspected grid and expanded-sidebar captures. Installed through
the existing reversible per-user installer.

### Gwenview rounded viewer and expanding toolbar — 2026-10-01

Rounded the main document-view container with a 16-pixel mask and 8-pixel inset.
Auto background mode uses the theme's raised surface, while explicit image
background choices remain native. Fullscreen removes the inset and mask.
Moved the existing status-bar controls into a rounded floating panel anchored
inside the lower-right viewer. A 44-pixel circular toolbar button expands the
panel leftward with a 220 ms OutCubic animation and collapses it on the next
click. Initial state is collapsed; the image layout never changes on expansion.
Native zoom, Fit, sidebar, thumbnail, count, and synchronization controls are
reused. The panel can scroll horizontally when its controls exceed available
width. Hidden controls cannot take focus; collapse restores focus to the button.

Build passed. Isolated GUI checks in light and dark themes verified rounded
viewer corners, initial collapsed state, fixed right anchor during expansion,
unchanged image geometry, native zoom interaction, complete collapse, animation
reversal, sidebar tabs/grid, narrow-window containment and scrollable overflow,
and fullscreen entry/exit through the native action. A test initially invoked
the fullscreen slot directly and bypassed its action state; corrected the harness
to use the native action after tracing the resulting reentrant transition.
Inspected collapsed, animated, expanded, and narrow captures under
`preview/gwenview-sidebar-light/` and `preview/gwenview-sidebar-dark/`.
Installed with the reversible per-user installer; reopen Gwenview to load it.

### Gwenview toolbar opacity and repeated animation — 2026-10-02

Fixed the floating controls' retained `WA_TranslucentBackground` and
`WA_NoSystemBackground` attributes, which suppressed background painting after
the originally parentless widget was embedded in the flyout. The panel and
button now use opaque window chrome, with an opaque viewport and controls
background. This remains solid when the surrounding theme is translucent.

Snapshot the visible animation progress and block intermediate signals while
resetting the animation's values, duration, and time. This prevents the retained
end time from changing progress during setup and reducing later runs to zero
duration. Pointer input is suppressed during movement without toggling enabled
state and triggering the style's separate fade animation.

The new repeated-slide regression check failed on the previous build. After
the fixes, the rebuilt application passed isolated GUI checks in both light
and dark transparent themes: fully opaque interior pixels, three consecutive
expansion/collapse cycles with intermediate widths and a fixed right anchor,
reversal without jumping, native zoom, narrow-window scrolling, sidebar tabs,
and fullscreen transitions. Inspected the captures in
`preview/gwenview-sidebar-{light,dark}-transparent/`. Installed through the
existing reversible per-user installer; reopen Gwenview to load the update.

### Gwenview matching toolbar and toggle shape — 2026-10-02

Set the expanded toolbar to the toggle's 44-pixel height and matched its painted
inset and semicircular end radius. Removed redundant vertical padding from the
controls so they fit the shorter panel. The fixed height also applies in narrow
windows. Rebuilt successfully and passed the existing isolated transparent-light
GUI checks for opacity, repeated slides, reversal, zoom, scrolling, and fullscreen.
Inspected expanded and narrow-window captures to confirm aligned top/bottom
edges, matching rounded ends, and visible controls.

### Gwenview separate rounded thumbnail area — 2026-10-02

Wrapped the existing thumbnail view in a theme-colored island with 16-pixel
rounded corners, 8-pixel inner padding, and a transparent 12-pixel splitter gap.
The thumbnails retain their own layout space as requested. Replaced the old
gradient tile separators with clean rounded selection and hover backgrounds.
Show/hide, fullscreen, and spotlight transitions now affect the whole wrapper.
The native view, model, selection, orientation, row count, and splitter sizing
remain in use.

Build passed. Isolated GUI checks passed for horizontal thumbnails in the light
transparent theme and vertical thumbnails in the dark transparent theme.
Verified clear panel corners, inset content, separation from the image, native
thumbnail selection, resizing, hiding/restoring the entire island at the saved
size, and fullscreen restoration. Existing toolbar and theme checks also passed.
Inspected both layout captures in `preview/gwenview-sidebar-*-transparent/`.
Installed with the existing reversible per-user installer.

### Gwenview floating main toolbar — 2026-10-02

Image view now hides and disables the docked main toolbar and exposes its
actions in an upper-right glass island. The toggle uses the bottom island's
220 ms flyout, blur, theme handling, and scrolling. Shared actions preserve
native navigation, menus, and enabled states; layout-only spacers are excluded.
Start and Browse restore the docked toolbar. Returning to image view starts
collapsed, and fullscreen/spotlight retain the floating controls.

The native build and isolated GUI checks passed. Added checks cover suppression
of the docked toolbar, expanding without resizing the viewer, native Next and
hamburger-menu clicks, action customization and enabled-state synchronization,
Start/Browse restoration, restart, fullscreen, and spotlight transitions.
Checked the light theme with horizontal thumbnails and the final dark transparent
theme with vertical thumbnails. Existing open/edit/save, history persistence,
disabled history, private-library loading, upgrade fallback, and reversible
installation checks passed. Visually inspected the collapsed viewer and expanded
main toolbar captures. Installed with the reversible per-user installer.

### Gwenview toolbar toggle proximity fades — 2026-10-03

Both collapsed islands now hide their toggles away from the pointer and reveal
them with a 180 ms fade when the pointer approaches within 48 pixels. A 64-pixel
exit margin prevents edge flicker. Expanded controls remain visible. Fully
transparent islands pass mouse input through to the image, and keyboard focus
reveals the toggle. Animation reversals retain their current opacity; disabled
desktop animations use immediate transitions.

Restored the cleared temporary build SDK from development RPMs into a persistent
workspace cache without changing system packages, then rebuilt successfully.
The isolated dark-transparent GUI suite passed with vertical thumbnails,
including intermediate opacity, fade reversal, expanded visibility, input
transparency, and keyboard-focus checks for both toggles. The offscreen platform
does not deliver cursor-warp movement, so proximity tests send viewport mouse
events directly and explicitly activate the test window for keyboard checks.
Inspected `preview/gwenview-sidebar-dark-transparent/viewer-toggles-hidden.png`.
Existing toolbar, fullscreen, image editing, persistence, library loading,
upgrade fallback, and reversible-installation checks passed. Updated the
installed per-user override.

### Gwenview rounded start page — 2026-10-04

The welcome page now paints an inset panel with the viewer's 8-pixel outer gap,
16-pixel corners, and opaque theme Button surface. The surrounding area shows
the window background, and the welcome content retains its inner padding.
Add files and drag-and-drop use their existing handlers.

Build and standard isolated GUI/install checks passed, including picker,
drag-and-drop, editing, persistence, and reversible installation. The dark
transparent sidebar/toolbar suite also passed. Visually inspected the light
welcome capture and dark themed start page to confirm rounded corners, matching
sidebar surfaces, and the floating inset. Updated the installed per-user override.

### Gwenview rounded Browse view — 2026-10-04

Browse now shares the start page/viewer's 8-pixel inset, 16-pixel rounded corners,
and opaque theme surface. Folder navigation, thumbnails, filters, and status
controls sit inside the panel with inner padding; rectangular background fills
and frames were removed where they interrupted that surface. Fullscreen uses
edge-to-edge layout and its existing palette.

The build and existing dark-transparent GUI suite passed, including Browse/View
transitions, toolbar restoration, thumbnail selection, and fullscreen transitions.
Added a Browse capture to the existing preview flow and visually inspected
`preview/gwenview-sidebar-dark-transparent/browse-rounded.png`. Updated the
installed per-user override.

### Gwenview Browse status-bar button shadows — 2026-10-04

Added soft 8-pixel shadows with a 2-pixel downward offset to the Browse status-bar
buttons. Shadow opacity follows the light/dark palette, and the status bar
reserves at least 6 pixels of padding around its controls.

The incremental build and existing light and dark-transparent GUI suites passed.
Visually inspected both Browse captures for button separation and shadow clipping.
Updated the installed per-user override.

### Gwenview fullscreen main toolbar suppression — 2026-10-04

Fullscreen now collapses and hides the floating main toolbar, including its
toggle. Leaving fullscreen restores the collapsed toolbar in image view.
The shared visibility update also prevents toolbar synchronization from showing
the island again while fullscreen is active.

The incremental build and dark-transparent GUI suite passed. Updated the
fullscreen regression checks to cover entering with the flyout expanded or
collapsed, hiding the entire island, and opening it again after returning to
windowed mode. Installed the tested per-user override.

### Centered widget picker — 2026-10-13

The Plasma-style shell layer now places Add Widgets in a floating, rounded
window centered on the owning monitor's strict available rectangle. Frame size
is included in the 86% width / 82% height limits and the 1080×760 logical-pixel
cap. Adaptive columns, smaller controls at narrow widths, and a scrolling grid
preserve usable content on scaled laptop displays. The native widget backend,
drag handlers, installation actions and activity sidebar are retained. Opening
from the global desktop-menu action now supplies the desktop as the add target.

`tools/verify-widget-picker.py` runs the actual shell in private Wayland/D-Bus
sessions with temporary desktop configuration and the installed catalog read-only.
Passed at 800×600 (light), 1280×720 (dark), 1280×720 at 200% scale (640×360
logical), 3840×2160, and on the second of two 1920×1080 outputs. The second
monitor's picker was positioned at x=2340, correctly incorporating its x=1920
origin. All 89 available catalog entries loaded. Runtime checks cover centering,
size bounds, grid geometry, search, running-widget filtering, click and Return
adding, Escape, focus dismissal, and switching to/from the activity sidebar.
Drag lifecycle checks exercise opacity, input pass-through, focus restoration,
and hide prevention; an actual pointer-driven drag/drop was not automated.
Software-rendered light/dark captures are in `preview/widget-picker-*.png`;
compositor blur and shadows are not represented in those captures.

Six `tools/verify-shell-overrides.py` tests passed, including selective removal,
lock-screen retention, and a picker-only update preserving unrelated local
edits and their original receipt hashes. `tools/install-widget-picker.py --check`
validated the live installation. The picker-only update was installed with a
backup of the previous files and receipt, and Plasma Shell was reloaded.

## Approved motion palette (2026-10-13)

- Applied the reviewed 200 / 370 / 160 / 200 / 240 / 200 / 450 ms families
  using `tools/install-motion-palette.py`, with per-user backups and targeted
  configuration restoration. Menus exit in 112 ms; windows exit in 168 ms.
- `tools/verify-motion-palette.py` passed in a private Wayland compositor:
  all six separately named effects loaded, Overview reported 200 ms, real window
  open/close/maximize/fullscreen/minimize and menu transitions ran, and the dock
  and 200 ms Plasma switch loaded without QML runtime errors.
- `tools/verify-dock-motion.py` passed, including 370 ms baseline, finite launch,
  cancellation, reduced-motion behavior, direction, hover modes and speed scaling.
- `tools/verify-start-menu.py` passed navigation, Back transitions, settings,
  launching, keyboard interaction, reduced motion, and placement checks.
- Live verification confirmed the six Sequoia motion effects replaced their stock
  counterparts, Plasma remained active, and Overview reported 200 ms after a
  KConfig change notification. All 24 targeted settings matched the installation
  plan. Existing application windows were retained.
- KWin desktop slide is spring-driven and remains gesture-controlled; its speed
  uses the workspace baseline rather than an exact duration. Magic Lamp retains
  its native shape at 240 ms. Third-party internal motion and operation progress
  are outside the theme's control. Qt Quick applications need to be reopened to
  discard cached QML; Qt Widgets receive the native style reload signal.


### Integrated Sequoia Taskbar — 2026-10-16

- Built the independent panel factory, shared settings QML plugin, and native
  Appearance → Taskbar KCM against Plasma 6.7.5, Qt 6.11.2, and Frameworks 6.31.
- CTest passed persistence, discard/defaults, concurrent pin preservation, input
  normalization, fixed/fill/panel sizing, and overflow calculation checks.
- A private KWin Wayland/Plasma session loaded the native KCM and verified Apply
  state, then created the taskbar with zero child applets. It rendered the default
  launcher icons and application menu, launched a temporary test application,
  minimized/restored its window, and rendered an added digital-clock widget.
- Native checks passed fixed-size behavior across panel-height changes, fill-mode
  button expansion without icon growth, custom-length overflow, live KConfig
  updates, and vertical orientation. Test desktop configuration used isolated XDG
  directories and a private session bus.
- Installation tests in a temporary prefix passed install/update, original-file
  restoration, protection of edited managed files, and modified-stage rejection.
- The stage is `.staging/taskbar-complete`; it has not been installed into the
  current desktop. Use `tools/install-taskbar.py` to install it. Sources, build
  instructions, compatibility checks, and remaining interactive-test limitations
  are documented in `plasma/taskbar/README.md`.

### Taskbar dock mode and Sequoia motion — 2026-10-16

- Reused the existing icons-only task manager's `DockMotion.qml` in the shared
  taskbar QML module. Added hover zoom/lift, press feedback, finite launch bounce
  and attention pulse, and animated task displacement, with live KCM controls
  and a preview. Transforms leave hit geometry fixed and stay within each button.
- Corrected the original motion guard and widget generator to recognize KDE's
  1 ms disabled-animation duration. Offscreen tests passed hover/press behavior,
  all inward directions, extreme motion bounds, attention completion/cancellation,
  and an actual `AnimationDurationFactor=0` configuration. Existing dock motion
  tests also passed.
- Native checks in a private Wayland/Plasma session exercised Panel ↔ Dock,
  growth/shrink after pin changes, fill sizing without a length feedback loop,
  vertical docks with an added clock, preservation of floating/alignment/hiding/
  thickness, editor length overrides across unrelated edits and shell restart,
  and new panels inheriting the Dock preset. Pointer hover and real mouse clicks
  verify the icon motion and minimize/restore interaction. Tooltips use Plasma's
  native window so they do not cover icons inside a narrow panel.
- CTest passed the expanded settings/persistence and natural-length checks. The
  native KCM loaded the mode and animation controls and rendered its preview.
  The generic offscreen test supplies icon search paths normally supplied by KDE's
  platform theme. Installer update/restore/tamper-protection tests passed.
- Updated payload: `.staging/taskbar-motion-final`. This update is staged, not
  installed into the current desktop. Native captures and logs are under
  `.staging/taskbar-verification/`, including `dock.png`, `vertical-dock.png`,
  `settings.png`, and `native.log`.
