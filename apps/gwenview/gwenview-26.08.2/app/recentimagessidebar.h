// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QWidget>
#include <QUrl>
namespace Gwenview {
class GvCore;
class RecentImagesSidebar : public QWidget {
    Q_OBJECT
public:
    explicit RecentImagesSidebar(GvCore *, QWidget *parent = nullptr);
Q_SIGNALS:
    void urlSelected(const QUrl &);
    void recentFileRemoved(const QUrl &);
    void recentFilesCleared();
};
}
