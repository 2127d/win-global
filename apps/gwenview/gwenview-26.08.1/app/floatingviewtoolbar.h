// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QWidget>
#include <QSharedPointer>
#include <QHash>
#include <QIcon>
class QScrollArea;
class QToolButton;
class QTimer;
class QVariantAnimation;
namespace Gwenview {
struct ToolbarBackdrop;
class FloatingViewToolbar : public QWidget
{
public:
    enum Position { BottomRight, TopRight };
    FloatingViewToolbar(QWidget *controls, QWidget *viewer, Position position = BottomRight);
    void setFullScreen(bool fullScreen);
    void collapse();
protected:
    bool eventFilter(QObject *, QEvent *) override;
private:
    void updateGeometryAndMask();
    void refreshBackdrop();
    void refreshTheme();
    void updateToggleReveal();
    void updatePointerProximity(const QPoint &globalPosition);
    QSharedPointer<ToolbarBackdrop> mBackdrop;
    QTimer *mBackdropTimer;
    QTimer *mThemeTimer;
    QHash<QToolButton *, QIcon> mOriginalIcons;
    QHash<QToolButton *, qint64> mThemedIconKeys;
    QRect mCaptureRect;
    QWidget *mViewer;
    QWidget *mControls;
    QWidget *mSurface;
    QScrollArea *mPanel;
    QToolButton *mToggle;
    QVariantAnimation *mAnimation;
    QVariantAnimation *mRevealAnimation;
    bool mPointerNear = false;
    bool mKeyboardFocus = false;
    qreal mProgress = 0;
    bool mFullScreen = false;
    Position mPosition;
};
}
