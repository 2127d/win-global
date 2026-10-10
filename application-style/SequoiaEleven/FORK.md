# Sequoia Eleven application style and decoration

Derived from the user's `/home/jeffreygustafson/Darkly` checkout on 2026-09-19.
The upstream README, AUTHORS, license, and source copyright notices are retained.
This fork is GPL-2.0-or-later; it is not covered by the original artwork's MIT license.

The fork uses independent `SequoiaEleven` style keys, `org.kde.sequoiaeleven`
decoration and configuration plugins, and `sequoiaelevenrc` settings. It adds
rounded controls, restrained translucent menus, slim scrollbars, compact centered
title bars, and fixed-size red/yellow/green window controls. It uses the separate
Sequoia Eleven light/dark color schemes and Plasma assets from the parent project.
The Solid and Transparent presets add independent decoration plugins under
`org.kde.sequoiaeleven.solid` and `org.kde.sequoiaeleven.transparent`, together
with alpha-preserving application surfaces and Qt Quick compositor blur support.
Their runtime defaults leave the original customizable settings intact.

Build with the parent project's `tools/build-desktop.sh`; do not run the copied
upstream installation script, which is retained only for provenance.
