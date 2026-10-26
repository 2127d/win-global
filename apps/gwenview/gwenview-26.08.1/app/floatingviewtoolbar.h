// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QWidget>
#include <QSharedPointer>
#include <QHash>
#include <QPointer>
#include <QIcon>
class QLayout;
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
    ~FloatingViewToolbar() override;
    void collapse();
    // A parent docks the original controls in its layout; nullptr restores the pill.
    void setControlsDocked(QWidget *parent);
protected:
    bool eventFilter(QObject *, QEvent *) override;
private:
    void updateGeometryAndMask();
    void refreshBackdrop();
    void refreshTheme();
    void updateToggleReveal();
    void updateShadow();
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
    QPointer<QWidget> mShadow;
    QScrollArea *mPanel;
    QToolButton *mToggle;
    QVariantAnimation *mAnimation;
    QVariantAnimation *mRevealAnimation;
    QHash<QLayout *, int> mNativeLayoutSpacing;
    QString mNativeStyleSheet;
    bool mControlsDocked = false;
    bool mPointerNear = false;
    bool mKeyboardFocus = false;
    qreal mProgress = 0;
    bool mFullScreen = false;
    Position mPosition;
};
}
