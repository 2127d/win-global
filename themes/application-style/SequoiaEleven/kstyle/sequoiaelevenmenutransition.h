// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEQUOIAELEVEN_MENUTRANSITION_H
#define SEQUOIAELEVEN_MENUTRANSITION_H

#include <QObject>
#include <QSize>
#include <QTimer>
class QMenu;

namespace SequoiaEleven {
// Keep a mapped popup's contents and backing-store size in step. The popup
// stays mapped throughout, preserving Qt/Wayland's native input grab.
class MenuTransition : public QObject
{
    Q_OBJECT
public:
    static void install(QMenu *menu);
    static void remove(QMenu *menu);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    explicit MenuTransition(QMenu *menu);
    void restore();
    void commit();
    QMenu *m_menu;
    QTimer m_commit;
    QSize m_minimum, m_maximum, m_frozenSize;
    bool m_frozen = false;
    bool m_committing = false;
    bool m_painted = false;
};
}
#endif
