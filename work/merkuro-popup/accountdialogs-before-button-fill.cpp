// SPDX-License-Identifier: GPL-3.0-or-later
// Keep Akonadi's live configuration widgets and save/cancel connections intact.
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QPushButton>
#include <QQuickWidget>
#include <QQuickItem>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QStyle>
#include <QListWidget>
#include <QKeyEvent>
#include <QPointer>
#include <QFocusEvent>
#include <QVBoxLayout>
#include <QWindow>
#include <QPainter>
#include <QQuickPaintedItem>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlProperty>

namespace {
class AccountAction final : public QQuickWidget {
public:
    AccountAction(QAbstractButton *button, const QString &module, QWidget *parent)
        : QQuickWidget(parent), button(button) {
        setObjectName(QStringLiteral("sequoiaNativeAccountActionHost"));
        setProperty("nativeButton", QVariant::fromValue(button));
        setResizeMode(QQuickWidget::SizeRootObjectToView);
        setClearColor(Qt::transparent);
        setAttribute(Qt::WA_AlwaysStackOnTop, true);
        setFocusPolicy(Qt::StrongFocus);
        setSource(QUrl::fromLocalFile(module + QStringLiteral("/qml/SequoiaNativeAccountAction.qml")));
        if (rootObject()) rootObject()->setProperty("nativeButton", QVariant::fromValue(button));
        sync();
        button->installEventFilter(this);
        connect(button, &QAbstractButton::toggled, this, [this] {sync();});
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    }
protected:
    void focusInEvent(QFocusEvent *event) override {
        QQuickWidget::focusInEvent(event);
        if (rootObject()) rootObject()->forceActiveFocus();
    }
    bool eventFilter(QObject *object, QEvent *event) override {
        if (!button) return QQuickWidget::eventFilter(object, event);
        if (object == button || object == button->parent()) {
            if (event->type() == QEvent::EnabledChange || event->type() == QEvent::StyleChange
                || event->type() == QEvent::PolishRequest || event->type() == QEvent::LayoutRequest) sync();
            if (object == button && event->type() == QEvent::ShowToParent) {sync();show();}
            if (object == button && event->type() == QEvent::HideToParent) hide();
        }
        return QQuickWidget::eventFilter(object, event);
    }
private:
    void sync() {
        auto root = rootObject();
        if (!root || !button) return;
        // Preserve literal ampersands while removing QWidget mnemonic markers.
        auto text = button->text();text.replace(QStringLiteral("&&"), QString(QChar(1)));
        text.remove('&');text.replace(QChar(1), QChar('&'));
        root->setProperty("buttonText", text);
        root->setProperty("buttonIcon", button->icon().name());
        root->setProperty("buttonEnabled", button->isEnabled());
        root->setProperty("buttonCheckable", button->isCheckable());
        root->setProperty("buttonChecked", button->isChecked());
        auto push = qobject_cast<QPushButton *>(button);
        root->setProperty("defaultButton", push && push->isDefault());
        setFixedSize(qCeil(root->implicitWidth()), qCeil(root->implicitHeight()));
        setEnabled(button->isEnabled());
    }
    QPointer<QAbstractButton> button;
};

class AccountCard final : public QWidget {
public:
    AccountCard(const QString &module, QWidget *parent) : QWidget(parent) {
        setObjectName(QStringLiteral("sequoiaNativeAccountCard"));
        // Keep the library's live Panel recipe, without a QQuickWidget texture
        // that punches a rectangular hole in the native window backing store.
        QQmlComponent component(&engine, QUrl::fromLocalFile(module + QStringLiteral("/qml/SequoiaNativeAccountSurface.qml")));
        surface = qobject_cast<QQuickItem *>(component.create());
        if (!surface) {qWarning() << "Account panel recipe failed:" << component.errors();return;}
        surface->setParent(this);
        surface->setObjectName(QStringLiteral("sequoiaNativeAccountBackground"));
        constexpr qreal panelRadius = 24.0;
        surface->setProperty("radius", panelRadius);
        for (const auto property : {"color", "radius", "border.color", "border.width"})
            QQmlProperty(surface, QString::fromLatin1(property)).connectNotifySignal(this, SLOT(update()));
    }
    ~AccountCard() override {delete surface;}
protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (surface) surface->setSize(size());
    }
    void paintEvent(QPaintEvent *) override {
        if (!surface) return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal panelRadius = surface->property("radius").toReal();
        const QRectF bounds(rect());
        painter.setPen(Qt::NoPen);
        painter.setBrush(surface->property("color").value<QColor>());
        painter.drawRoundedRect(bounds, panelRadius, panelRadius);

        const qreal borderWidth = QQmlProperty::read(surface, "border.width").toReal();
        const qreal inset = borderWidth / 2;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QQmlProperty::read(surface, "border.color").value<QColor>(), borderWidth));
        if (borderWidth > 0)
            painter.drawRoundedRect(bounds.adjusted(inset, inset, -inset, -inset),
                                    qMax(0., panelRadius - inset), qMax(0., panelRadius - inset));
        // Invoke the component library's actual rim painter, keeping its live
        // intensity setting and avoiding a second implementation of the edge.
        for (auto rim : surface->findChildren<QQuickPaintedItem *>()) {
            painter.save();
            rim->setSize(size());
            rim->paint(&painter);
            painter.restore();
        }
    }
private:
    QQmlEngine engine;
    QQuickItem *surface = nullptr;
};

class AccountDialogs final : public QObject {
public:
    explicit AccountDialogs(QString module) : QObject(qApp), module(std::move(module)) {}
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            auto dialog = qobject_cast<QDialog *>(object);
            auto key = static_cast<QKeyEvent *>(event);
            if (dialog && dialog->property("sequoiaAccountStyled").toBool()
                && (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
                && !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
                auto box = dialog->findChild<QDialogButtonBox *>(QStringLiteral("sequoiaNativeAccountButtons"));
                QPushButton *action = nullptr;
                for (auto button : dialog->findChildren<QPushButton *>())
                    if (button->isDefault() && button->isEnabled()) {action = button;break;}
                if (!action && box) action = box->button(QDialogButtonBox::Ok);
                if (action && action->isEnabled()) {action->click();key->accept();return true;}
            }
        }
        if (event->type() != QEvent::Polish && event->type() != QEvent::Show) return false;
        auto dialog = qobject_cast<QDialog *>(object);
        if (!dialog || !dialog->inherits("Akonadi::AgentConfigurationDialog")
            || dialog->property("sequoiaAccountStyled").toBool()) return false;
        auto layout = qobject_cast<QVBoxLayout *>(dialog->layout());
        QWidget *form = nullptr;
        for (auto child : dialog->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
            if (child->inherits("Akonadi::AgentConfigurationWidget")) form = child;
        auto buttons = dialog->findChild<QDialogButtonBox *>();
        if (!layout || !form || !buttons) return false;
        dialog->setProperty("sequoiaAccountStyled", true);
        dialog->setObjectName(QStringLiteral("sequoiaNativeAccountEditor"));
        // QML applications can retain a fallback QWidget style. Use the selected
        // Sequoia native style for the real plugin controls as well as its canvas.
        QSettings colors(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                         + QStringLiteral("/kdeglobals"), QSettings::IniFormat);
        const auto styleName = colors.value(QStringLiteral("KDE/widgetStyle")).toString();
        if (styleName.startsWith(QStringLiteral("SequoiaEleven"), Qt::CaseInsensitive)) {
            auto style = qApp->findChild<QStyle *>(QStringLiteral("merkuroAccountStyle-") + styleName);
            if (!style) {
                style = QStyleFactory::create(styleName);
                if (style) {style->setObjectName(QStringLiteral("merkuroAccountStyle-") + styleName);style->setParent(qApp);}
            }
            if (style) {
                auto palette = dialog->palette();
                auto channels = colors.value(QStringLiteral("Colors:Window/BackgroundNormal")).toStringList();
                if (channels.size() == 1) channels = channels.front().split(',');
                if (channels.size() >= 3)
                    palette.setColor(QPalette::Window, QColor(channels[0].toInt(), channels[1].toInt(), channels[2].toInt(), channels.size() > 3 ? channels[3].toInt() : 255));
                dialog->setPalette(palette);
                dialog->setStyle(style);
                for (auto widget : dialog->findChildren<QWidget *>()) widget->setStyle(style);
            }
        }
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(16);
        layout->removeWidget(form);
        auto card = new AccountCard(module, dialog);
        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        auto scroll = new QScrollArea(card);
        scroll->setObjectName(QStringLiteral("sequoiaNativeAccountScroll"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setAutoFillBackground(false);
        scroll->viewport()->setAutoFillBackground(false);
        form->setAutoFillBackground(false);
        scroll->setWidget(form);
        cardLayout->addWidget(scroll);
        layout->insertWidget(0, card, 1);
        buttons->setObjectName(QStringLiteral("sequoiaNativeAccountButtons"));
        buttons->setCenterButtons(false);
        for (auto button : buttons->buttons()) {
            button->setMinimumHeight(42);
            button->setMinimumWidth(88);
            button->setCursor(Qt::PointingHandCursor);
        }
        // Render the original actions with the canonical ActionPill. The hidden
        // native buttons still own validation, default actions and save signals.
        layout->removeWidget(buttons);
        buttons->hide();
        auto footer = new QWidget(dialog);
        footer->setObjectName(QStringLiteral("sequoiaNativeAccountFooter"));
        auto footerLayout = new QHBoxLayout(footer);
        footerLayout->setContentsMargins(0, 0, 0, 0);footerLayout->setSpacing(8);
        footerLayout->addStretch();
        for (auto button : buttons->buttons()) footerLayout->addWidget(new AccountAction(button, module, footer));
        layout->addWidget(footer);
        for (auto button : form->findChildren<QPushButton *>()) {
            auto parent = button->parentWidget();
            if (!parent || !parent->layout()) continue;
            auto action = new AccountAction(button, module, parent);
            auto old = parent->layout()->replaceWidget(button, action);
            if (!old) {delete action;continue;}
            delete old;
            const bool visible = !button->isHidden();
            auto storage = new QWidget(parent);storage->hide();
            button->setParent(storage);button->setVisible(visible);
            storage->installEventFilter(action);
            action->setVisible(visible);
            parent->layout()->invalidate();
        }
        // Provider forms can carry fixed Designer minimum sizes calculated for
        // small native buttons. Recompute their layout minima for 42 px pills.
        for (auto list : form->findChildren<QListWidget *>()) list->setMinimumHeight(100);
        auto containers = form->findChildren<QWidget *>();containers.prepend(form);
        for (auto container : containers) if (container->layout()) {
            container->layout()->setSizeConstraint(QLayout::SetMinimumSize);
            container->layout()->invalidate();
        }
        dialog->setMinimumSize(440, 320);
        // A parentless QWidget dialog otherwise loses its relationship to settings.
        if (dialog->windowHandle())
            for (auto window : QGuiApplication::allWindows())
                if (window->objectName() == QStringLiteral("sequoiaMerkuroSettingsWindow") && window->isVisible()) {
                    dialog->windowHandle()->setTransientParent(window);
                    break;
                }
        return false;
    }
private:
    QString module;
};
}

void installMerkuroAccountDialogs(const QString &module) {
    if (qApp->property("sequoiaAccountDialogsInstalled").toBool()) return;
    qApp->setProperty("sequoiaAccountDialogsInstalled", true);
    qApp->installEventFilter(new AccountDialogs(module));
}
