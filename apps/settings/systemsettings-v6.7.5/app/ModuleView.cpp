/*
 *   SPDX-FileCopyrightText: 2009 Ben Cooksley <bcooksley@kde.org>
 *   SPDX-FileCopyrightText: 2009 Mathias Soeken <msoeken@informatik.uni-bremen.de>
 *   SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ModuleView.h"
#include "ExternalAppModule.h"
#include "MenuItem.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFrame>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QMap>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWhatsThis>

#include <KAboutData>
#include <KAuth/Action>
#include <KAuth/ExecuteJob>
#include <KAuthorized>
#include <KCModule>
#include <KCModuleLoader>
#include <KColorScheme>
#include <KMessageDialog>
#include <KPageWidget>
#include <KPluginMetaData>
#include <KSharedConfig>
#include <KStandardGuiItem>
#include <KTitleWidget>
#include <PlasmaActivities/ResourceInstance>

// Apply a consistent presentation to every hosted settings page. Installed KCMs
// and their controls stay intact; transparent page layers reveal one QWidget surface
// behind the title, scrollable content, and native action buttons.
static void prepareFloatingSurface(QQuickWidget *quick)
{
    quick->setClearColor(Qt::transparent);
    quick->setAutoFillBackground(false);
    auto root = quick->rootObject();
    if (!root) {
        return;
    }
    QList<QQuickItem *> items{root};
    for (qsizetype i = 0; i < items.size(); ++i) {
        auto item = items[i];
        items.append(item->childItems());
        // KCMs can push subpages or create header/footer delegates after loading.
        // Coalesce those changes and style new page layers without polling.
        if (!item->property("_sequoia_surfaceObserved").toBool()) {
            item->setProperty("_sequoia_surfaceObserved", true);
            QObject::connect(item, &QQuickItem::childrenChanged, quick, [quick] {
                if (quick->property("_sequoia_surfacePending").toBool()) return;
                quick->setProperty("_sequoia_surfacePending", true);
                QTimer::singleShot(0, quick, [quick] {
                    quick->setProperty("_sequoia_surfacePending", false);
                    prepareFloatingSurface(quick);
                });
            });
        }
        const QByteArray type(item->metaObject()->className());
        if (type.startsWith("Separator_") || type.startsWith("FormSeparator_")) {
            // Keep their layout space, including the gap between form groups.
            item->setOpacity(0);
        }
        bool pageLayer = item == root;
        bool abstractKcm = false;
        for (auto meta = item->metaObject(); meta; meta = meta->superClass()) {
            const QByteArray base(meta->className());
            abstractKcm |= base.startsWith("AbstractKCM_");
            pageLayer |= base == "QQuickPage" || base.startsWith("PageRow_") || base.startsWith("Page_")
                || base.startsWith("ScrollablePage_") || base.startsWith("AbstractKCM_")
                || base.startsWith("ToolBarPageHeader_") || base.startsWith("PageRowGlobalToolBarUI_");
        }
        if (pageLayer) {
            if (auto background = item->property("background").value<QQuickItem *>()) {
                background->setOpacity(0);
            }
            if (item->metaObject()->indexOfProperty("separatorVisible") >= 0) {
                item->setProperty("separatorVisible", false);
            }
        }
        if (abstractKcm) {
            // AbstractKCM and a few modules add full-content background
            // rectangles separate from Page.background. Preserve rectangles
            // containing controls; the theme overlay uses a named Canvas.
            if (auto content = item->property("contentItem").value<QQuickItem *>()) {
                for (auto child : content->childItems()) {
                    auto anchors = child->property("anchors").value<QObject *>();
                    const bool fillsContent = anchors && anchors->property("fill").value<QQuickItem *>() == content;
                    if ((QByteArray(child->metaObject()->className()) == "QQuickRectangle"
                         && child->childItems().isEmpty() && fillsContent)
                        || child->objectName() == QLatin1String("sequoiaCanvas")) {
                        child->setOpacity(0);
                    }
                }
            }
        }
    }
}

static void hideWidgetSeparators(QWidget *host)
{
    // Retain spacing and interactive frames, such as lists and text fields.
    for (auto frame : host->findChildren<QFrame *>()) {
        const auto shape = frame->frameShape();
        if (shape == QFrame::HLine) {
            frame->setMinimumHeight(qMax(frame->minimumHeight(), frame->sizeHint().height()));
        } else if (shape == QFrame::VLine) {
            frame->setMinimumWidth(qMax(frame->minimumWidth(), frame->sizeHint().width()));
        } else {
            continue;
        }
        frame->setFrameShape(QFrame::NoFrame);
    }
}

class CustomTitle : public KTitleWidget
{
    Q_OBJECT
public:
    explicit CustomTitle(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void colorsChanged();
};

CustomTitle::CustomTitle(QWidget *parent)
    : KTitleWidget(parent)
{
    setTextSelectable(false);
    // Use the same left margin as QML titles for consistency (Kirigami/AbstractPageHeader.qml)
    // 18px is Standard Kirigami gridUnit for 10pt Noto Sans.
    // TODO: make this use a real gridUnit so it will adjust to the user's font,
    // once we have a QmlEngine object such that  using it won't risk crashes!
    setContentsMargins(18,
                       style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                       style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                       style()->pixelMetric(QStyle::PM_LayoutBottomMargin));

    colorsChanged();
}

void CustomTitle::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        colorsChanged();
        return; // deliberately intercept
    }
    return KTitleWidget::changeEvent(event);
}

void CustomTitle::colorsChanged()
{
    auto active = KColorScheme(QPalette::Active, KColorScheme::Header);
    auto inactive = KColorScheme(QPalette::Inactive, KColorScheme::Header);
    auto disabled = KColorScheme(QPalette::Disabled, KColorScheme::Header);

    QPalette palette = QWidget::palette();

    palette.setBrush(QPalette::Active, QPalette::Window, active.background());
    palette.setBrush(QPalette::Active, QPalette::WindowText, active.foreground());
    palette.setBrush(QPalette::Disabled, QPalette::Window, disabled.background());
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, disabled.foreground());
    palette.setBrush(QPalette::Inactive, QPalette::Window, inactive.background());
    palette.setBrush(QPalette::Inactive, QPalette::WindowText, inactive.foreground());

    setPalette(palette);
}

class ModuleView::Private
{
public:
    Private()
    {
    }
    QMap<KPageWidgetItem *, KCModule *> mPages;
    QMap<KPageWidgetItem *, QString> mPagesPluginIdMap;
    KPageWidget *mPageWidget = nullptr;
    CustomTitle *mCustomHeader = nullptr;
    QVBoxLayout *mLayout = nullptr;
    QDialogButtonBox *mButtons = nullptr;
    QPushButton *mApply = nullptr;
    QPushButton *mReset = nullptr;
    QPushButton *mDefault = nullptr;
    QPushButton *mHelp = nullptr;
    std::shared_ptr<QQmlEngine> engine;
    QIcon mApplyIcon;
    KMessageDialog *mResolvingChangesDialog = nullptr;
    bool pageChangeSupressed = false;
    bool mSaveStatistics = true;
    bool mDefaultsIndicatorsVisible = false;
    KCModule::Buttons mButtonMask = ~KCModule::Buttons(KCModule::NoAdditionalButton);
    KAuth::Action authAction;
};

ModuleView::ModuleView(const std::shared_ptr<QQmlEngine> &engine, QWidget *parent)
    : QWidget(parent)
    , d(new Private())
{
    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(0);
    // Configure a layout first
    d->mLayout = new QVBoxLayout();
    // Create the Page Widget
    d->mPageWidget = new KPageWidget(this);
    d->mCustomHeader = new CustomTitle(this);
    d->mCustomHeader->setVisible(false);

    rootLayout->addWidget(d->mCustomHeader);
    rootLayout->addItem(d->mLayout);
    // d->mPageWidget->setPageHeader(d->mCustomHeader);
    d->mPageWidget->layout()->setContentsMargins(0, 0, 0, 0);

    // Zero out only the horizontal spacing (the vertical spacing is fine)
    auto gridLayout = qobject_cast<QGridLayout *>(d->mPageWidget->layout());

    gridLayout->setHorizontalSpacing(0);

    d->mLayout->addWidget(d->mPageWidget);
    // Create the dialog
    d->mButtons = new QDialogButtonBox(Qt::Horizontal, this);
    d->mButtons->setProperty("_breeze_force_frame", false);
    d->mLayout->addWidget(d->mButtons);

    // Create the buttons in it
    d->mApply = d->mButtons->addButton(QDialogButtonBox::Apply);
    d->mApplyIcon = d->mApply->icon();
    KGuiItem::assign(d->mApply, KStandardGuiItem::apply());
    d->mDefault = d->mButtons->addButton(QDialogButtonBox::RestoreDefaults);
    KGuiItem::assign(d->mDefault, KStandardGuiItem::defaults());
    d->mReset = d->mButtons->addButton(QDialogButtonBox::Reset);
    KGuiItem::assign(d->mReset, KStandardGuiItem::reset());
    d->mHelp = d->mButtons->addButton(QDialogButtonBox::Help);
    KGuiItem::assign(d->mHelp, KStandardGuiItem::help());
    // Set some more sensible tooltips
    d->mReset->setToolTip(i18n("Reset all current changes to previous values"));
    // Set Auto-Default mode ( KDE Bug #211187 )
    d->mApply->setAutoDefault(true);
    d->mDefault->setAutoDefault(true);
    d->mReset->setAutoDefault(true);
    d->mHelp->setAutoDefault(true);
    // Prevent the buttons from being used
    d->mApply->setEnabled(false);
    d->mDefault->setEnabled(false);
    d->mReset->setEnabled(false);
    d->mHelp->setEnabled(false);
    // Connect up the buttons
    connect(d->mApply, &QPushButton::clicked, this, &ModuleView::saveActiveModule);
    connect(d->mReset, &QAbstractButton::clicked, this, &ModuleView::moduleLoad);
    connect(d->mHelp, &QAbstractButton::clicked, this, &ModuleView::moduleHelp);
    connect(d->mDefault, &QAbstractButton::clicked, this, &ModuleView::moduleDefaults);
    // clang-format off
    connect(d->mPageWidget, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)),
             this, SLOT(activeModuleChanged(KPageWidgetItem*,KPageWidgetItem*)));
    // clang-format on

    connect(d->mApply, &QPushButton::clicked, this, [this] {
        if (d->authAction.isValid()) {
            KAuth::ExecuteJob *job = d->authAction.execute(KAuth::Action::AuthorizeOnlyMode);
            connect(job, &KAuth::ExecuteJob::statusChanged, this, [this](KAuth::Action::AuthStatus status) {
                authStatusChanged(status);
            });
            job->start();
        }
    });

    d->engine = engine;
}

ModuleView::~ModuleView()
{
    closeModules();
    delete d;
}

void ModuleView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    const QRectF surface = QRectF(rect()).adjusted(12, 12, -12, -12);
    const bool dark = palette().color(QPalette::WindowText).lightnessF() > 0.5;
    for (int spread = 4; spread > 0; --spread) {
        painter.setBrush(QColor(0, 0, 0, dark ? 7 : 3));
        painter.drawRoundedRect(surface.adjusted(-spread, -spread, spread, spread).translated(0, 2), 18 + spread, 18 + spread);
    }
    const QColor canvas = palette().color(QPalette::Window);
    const qreal lift = dark ? 0.065 : 0.8;
    const QColor fill = QColor::fromRgbF(canvas.redF() + (1 - canvas.redF()) * lift,
                                        canvas.greenF() + (1 - canvas.greenF()) * lift,
                                        canvas.blueF() + (1 - canvas.blueF()) * lift);
    painter.setBrush(fill);
    painter.drawRoundedRect(surface, 18, 18);
}

QString ModuleView::activeModuleName() const
{
    return d->mPageWidget->currentPage() ? d->mPageWidget->currentPage()->name() : QString();
}

void ModuleView::loadModule(const QModelIndex &menuItem, const QStringList &args)
{
    if (!menuItem.isValid()) {
        return;
    }

    auto item = menuItem.data(Qt::UserRole).value<MenuItem *>();

    // if module has a main page (like in Appearance > Global Theme) we'll load that
    if (item->isLibrary() || item->isExternalAppModule()) {
        addModule(item, args);
    }
    // if module doesn't have a main page, we'll load the first subpage
    else if (menuItem.model()->rowCount(menuItem) > 0) {
        auto subpageItem = menuItem.model()->index(0, 0, menuItem).data(Qt::UserRole).value<MenuItem *>();
        addModule(subpageItem, args);
    }
}

void ModuleView::addModule(MenuItem *item, const QStringList &args)
{
    const KPluginMetaData data = item->metaData();
    if (!KAuthorized::authorizeControlModule(data.pluginId())) {
        qWarning() << "Not authorised to load module";
        return;
    }
    if (data.isHidden()) {
        return;
    }

    if (KPageWidgetItem *page = d->mPagesPluginIdMap.key(data.pluginId())) {
        activeModuleChanged(page, d->mPageWidget->currentPage());
        return;
    }

    // Create the scroller
    auto moduleScroll = new QScrollArea(this);
    // Prepare the scroll area
    moduleScroll->setWidgetResizable(true);
    moduleScroll->setFrameStyle(QFrame::NoFrame);
    moduleScroll->viewport()->setAutoFillBackground(false);
    moduleScroll->horizontalScrollBar()->installEventFilter(this);
    moduleScroll->verticalScrollBar()->installEventFilter(this);
    // Create the page
    auto page = new KPageWidgetItem(moduleScroll, data.name());
    // Provide information to the users

    // set accessible name, or screen reader users will have a cryptic "LayeredPane" tabstop
    moduleScroll->setAccessibleName(i18ndc("systemsettings", "@info:whatsthis", "Scrollable area"));

    if (item->isExternalAppModule()) {
        auto externalWidget = new ExternalAppModule(KService::Ptr(new KService(item->metaData().fileName())));
        moduleScroll->setWidget(externalWidget);
        externalWidget->setAutoFillBackground(false);
        d->mCustomHeader->setText(item->metaData().name()); // We have to set this manually, BUG: 448672
        page->setName(QString());
    } else { // It must be a normal module then
        auto kcm = KCModuleLoader::loadModule(data, moduleScroll, QVariantList(args.begin(), args.end()), d->engine);
        moduleScroll->setWidget(kcm->widget());
        kcm->widget()->setAutoFillBackground(false);
        kcm->load();
        hideWidgetSeparators(kcm->widget());
        auto quickWidgets = kcm->widget()->findChildren<QQuickWidget *>();
        if (auto quick = qobject_cast<QQuickWidget *>(kcm->widget())) {
            quickWidgets.prepend(quick);
        }
        for (auto quick : quickWidgets) {
            prepareFloatingSurface(quick);
            // Header delegates finish constructing at the next event turn.
            QTimer::singleShot(0, quick, [quick] { prepareFloatingSurface(quick); });
        }
        connect(kcm, &KCModule::needsSaveChanged, this, &ModuleView::stateChanged);
        connect(kcm, &KCModule::buttonsChanged, this, &ModuleView::updateButtons);
        connect(kcm, &KCModule::representsDefaultsChanged, this, [this, kcm]() {
            if (kcm == d->mPages.value(d->mPageWidget->currentPage()) && kcm->buttons() & d->mButtonMask & KCModule::Default) {
                d->mDefault->setEnabled(!kcm->representsDefaults());
            }
        });

        d->mPages.insert(page, kcm);
    }

    d->mPagesPluginIdMap.insert(page, data.pluginId());
    updatePageIconHeader(page);
    // Add the new page
    d->mPageWidget->addPage(page);
}

void ModuleView::updatePageIconHeader(KPageWidgetItem *page)
{
    if (!page) {
        // Page is invalid. Probably means we have a race condition during closure of everyone so do nothing
        return;
    }

    KCModule *kcm = d->mPages.value(page);
    if (!kcm) {
        // Seems like we have some form of a race condition going on here...
        return;
    }

    const bool isQml = kcm->inherits("KCModuleQml");

    if (!kcm->metaData().isValid()) {
        // KCModule was (incorrectly) created with a constructor that didn't store metadata
        // Never use the custom header here because we don't know the module name
        page->setHeaderVisible(!isQml);
        if (d->mPageWidget->currentPage() == page) {
            d->mCustomHeader->hide();
        }
        if (!page->name().isEmpty()) {
            d->mCustomHeader->setText(page->name());
            page->setHeaderVisible(false);
        }
        return;
    }

    const QString &moduleName = kcm->metaData().name();
    page->setHeader(moduleName);
    page->setIcon(QIcon::fromTheme(kcm->metaData().iconName()));

    page->setHeaderVisible(false);

    // Use the custom header only for QWidgets KCMs on Sidebar mode
    // Only affect visibility if it's the current page
    if (d->mPageWidget->currentPage() == page) {
        if (!isQml) {
            d->mCustomHeader->setText(moduleName); // also includes show()
        } else {
            d->mCustomHeader->hide();
        }
    }
}

bool ModuleView::resolveChanges()
{
    KCModule *kcm = d->mPages.value(d->mPageWidget->currentPage());
    return resolveChanges(kcm);
}

bool ModuleView::resolveChanges(KCModule *kcm)
{
    if (!kcm || !kcm->needsSave()) {
        return true;
    }

    // if we are already resolving changes handle it like a cancel
    if (d->mResolvingChangesDialog) {
        d->mResolvingChangesDialog->reject();
    }

    // Let the user decide
    d->mResolvingChangesDialog = new KMessageDialog(KMessageDialog::WarningTwoActionsCancel,
                                                    i18n("The current page has unsaved changes.\n"
                                                         "Apply the changes or discard them?"),
                                                    this);
    d->mResolvingChangesDialog->setAttribute(Qt::WA_DeleteOnClose);
    d->mResolvingChangesDialog->setButtons(KStandardGuiItem::apply(), KStandardGuiItem::discard(), KStandardGuiItem::cancel());
    d->mResolvingChangesDialog->setCaption(i18n("Apply Settings"));
    d->mResolvingChangesDialog->setIcon(QIcon()); // Use default message box warning icon.
    int result = d->mResolvingChangesDialog->exec();
    d->mResolvingChangesDialog = nullptr;

    switch (result) {
    case KMessageDialog::PrimaryAction:
        moduleSave(kcm);
        return true;
    case KMessageDialog::SecondaryAction:
        kcm->load();
        return true;
    case KMessageDialog::Cancel:
        return false;
    default:
        Q_ASSERT(false);
        return false;
    }
}

void ModuleView::closeModules()
{
    d->pageChangeSupressed = true;
    d->authAction = KAuth::Action();
    for (auto page = d->mPagesPluginIdMap.cbegin(); page != d->mPagesPluginIdMap.cend(); ++page) {
        // Delete the KCM first, because e.g. the KFontInst KCM accesses it's widgets in the destructor
        delete d->mPages.value(page.key());
        d->mPageWidget->removePage(page.key());
    }

    d->mPages.clear();
    d->mPagesPluginIdMap.clear();
    d->pageChangeSupressed = false;
}

void ModuleView::saveActiveModule()
{
    KCModule *moduleProxy = d->mPages.value(d->mPageWidget->currentPage());
    Q_ASSERT(moduleProxy);
    moduleSave(moduleProxy);
}

void ModuleView::moduleSave(KCModule *module)
{
    module->save();
    Q_EMIT moduleSaved();
}

void ModuleView::moduleLoad()
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (activeModule) {
        activeModule->load();
    }
}

void ModuleView::moduleDefaults()
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (activeModule) {
        activeModule->defaults();
    }
}

void ModuleView::moduleHelp()
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (!activeModule) {
        return;
    }

    const QString docPath = activeModule->metaData().value(QStringLiteral("X-DocPath"));
    if (docPath.isEmpty()) {
        return;
    }

    // UrlHandler from KGUIAddons sets a handler for help:/ urls, which opens khelpcenter
    // if it's available or falls back to opening the relevant page at docs.kde.org
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/") + docPath));
}

void ModuleView::activeModuleChanged(KPageWidgetItem *current, KPageWidgetItem *previous)
{
    d->mPageWidget->blockSignals(true);
    d->mPageWidget->setCurrentPage(previous);
    KCModule *previousModule = d->mPages.value(previous);
    if (resolveChanges(previousModule)) {
        d->mPageWidget->setCurrentPage(current);
    }
    d->mPageWidget->blockSignals(false);
    if (d->pageChangeSupressed) {
        return;
    }

    // We need to get the state of the now active module
    stateChanged();

    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (!activeModule) {
        return;
    }

    // TODO: if we'll ever need statistics for kinfocenter modules, save them with an URL like "kinfo:"
    if (d->mSaveStatistics && activeModule->metaData().pluginId() != QStringLiteral("kcm_landingpage")) {
        KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("kcm:") + activeModule->metaData().pluginId()),
                                                      QStringLiteral("org.kde.systemsettings"));
    }

    d->mLayout->setContentsMargins(0, 0, 0, 0);
    d->mLayout->setSpacing(0);
    d->mButtons->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                    style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                    style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                    style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    d->mPageWidget->layout()->setSpacing(0);
    if (auto titleWidget = qobject_cast<KTitleWidget *>(d->mPageWidget->pageHeader())) {
        // 18px is Standard Kirigami gridUnit for 10pt Noto Sans.
        // TODO: make this use a real gridUnit so it will adjust to the user's font,
        // once we have a QmlEngine object such that  using it won't risk crashes!
        titleWidget->layout()->setContentsMargins(18,
                                                  style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                  style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                                  style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    }

    updatePageIconHeader(current);
    updateScrollAreaFocusPolicy();
    moduleShowDefaultsIndicators(d->mDefaultsIndicatorsVisible);
}

void ModuleView::stateChanged()
{
    updatePageIconHeader(d->mPageWidget->currentPage());
    updateButtons();

    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    Q_EMIT moduleChanged(activeModule && activeModule->needsSave());
}

void ModuleView::updateButtons()
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    bool change = false;
    bool defaulted = false;
    KCModule::Buttons buttons = KCModule::NoAdditionalButton;

    if (activeModule) {
        buttons = activeModule->buttons() & d->mButtonMask;
        change = activeModule->needsSave();
        defaulted = activeModule->representsDefaults();

        d->authAction = KAuth::Action(activeModule->authActionName());
        authStatusChanged(d->authAction.status());

        // Do not display Help button if there is no docPath available
        if (activeModule->metaData().value(QStringLiteral("X-DocPath")).isEmpty()) {
            buttons &= ~KCModule::Help;
        }
    } else {
        d->authAction = KAuth::Action();
    }

    d->mDefault->setEnabled(!defaulted);
    d->mDefault->setVisible(buttons & KCModule::Default);
    d->mApply->setEnabled(change);
    d->mApply->setVisible(buttons & KCModule::Apply);
    d->mReset->setEnabled(change);
    d->mReset->setVisible(buttons & KCModule::Apply);
    d->mHelp->setEnabled(buttons & KCModule::Help);
    d->mHelp->setVisible(buttons & KCModule::Help);

    d->mButtons->setVisible(buttons != KCModule::NoAdditionalButton);
}

void ModuleView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F1 && d->mHelp->isVisible() && d->mHelp->isEnabled()) {
        d->mHelp->animateClick();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Escape) {
        event->accept();
        Q_EMIT closeRequest();
        return;
    } else if (event->key() == Qt::Key_F1 && event->modifiers() == Qt::ShiftModifier) {
        QWhatsThis::enterWhatsThisMode();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void ModuleView::setFaceType(KPageView::FaceType type)
{
    d->mPageWidget->setFaceType(type);
}

void ModuleView::setSaveStatistics(bool save)
{
    d->mSaveStatistics = save;
}

bool ModuleView::saveStatistics() const
{
    return d->mSaveStatistics;
}

void ModuleView::setApplyVisible(bool visible)
{
    d->mButtonMask.setFlag(KCModule::Apply, visible);
    updateButtons();
}

bool ModuleView::isApplyVisible() const
{
    return d->mApply->isVisible();
}

void ModuleView::setDefaultsVisible(bool visible)
{
    d->mButtonMask.setFlag(KCModule::Default, visible);
    updateButtons();
}

bool ModuleView::isDefaultsVisible() const
{
    return d->mDefault->isVisible();
}

bool ModuleView::isResetVisible() const
{
    return d->mReset->isVisible();
}

void ModuleView::moduleShowDefaultsIndicators(bool show)
{
    d->mDefaultsIndicatorsVisible = show;
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (activeModule) {
        activeModule->setDefaultsIndicatorsVisible(show);
    }
}

void ModuleView::setHeaderHeight(qreal height)
{
    if (height == d->mCustomHeader->minimumHeight()) {
        return;
    }

    d->mCustomHeader->setMinimumHeight(height);
}

qreal ModuleView::headerHeight() const
{
    return d->mCustomHeader->minimumHeight();
}

void ModuleView::setActiveModule(const QString &pluginId)
{
    const auto pageList = d->mPagesPluginIdMap.keys();
    for (const auto page : pageList) {
        if (d->mPagesPluginIdMap.value(page) == pluginId) {
            d->mPageWidget->setCurrentPage(page);
            break;
        }
    }
}

void ModuleView::requestActivation(const QVariantList &args)
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (activeModule) {
        Q_EMIT activeModule->activationRequested(args);
    }
}

KPluginMetaData ModuleView::activeModuleMetadata() const
{
    KCModule *activeModule = d->mPages.value(d->mPageWidget->currentPage());
    if (!activeModule) {
        return {};
    }
    return activeModule->metaData();
}

void ModuleView::authStatusChanged(KAuth::Action::AuthStatus status)
{
    switch (status) {
    case KAuth::Action::AuthorizedStatus:
        d->mApply->setEnabled(true);
        d->mApply->setIcon(d->mApplyIcon);
        break;
    case KAuth::Action::AuthRequiredStatus:
        d->mApply->setEnabled(true);
        d->mApply->setIcon(QIcon::fromTheme(QStringLiteral("dialog-password")));
        break;
    default:
        d->mApply->setEnabled(false);
        d->mApply->setIcon(d->mApplyIcon);
    }
}

void ModuleView::updateScrollAreaFocusPolicy()
{
    KPageWidgetItem *item = d->mPageWidget->currentPage();
    if (!item) {
        return;
    }
    QScrollArea *moduleScroll = qobject_cast<QScrollArea *>(item->widget());
    if (moduleScroll) {
        bool scrollbarVisible = moduleScroll->horizontalScrollBar()->isVisible() || moduleScroll->verticalScrollBar()->isVisible();
        moduleScroll->setFocusPolicy(scrollbarVisible ? Qt::FocusPolicy::StrongFocus : Qt::FocusPolicy::NoFocus);
    }
}

bool ModuleView::eventFilter(QObject *watched, QEvent *event)
{
    if ((event->type() == QEvent::Show || event->type() == QEvent::Hide) && d->mPageWidget->currentPage()) {
        QScrollArea *moduleScroll = qobject_cast<QScrollArea *>(d->mPageWidget->currentPage()->widget());
        if (moduleScroll && (watched == moduleScroll->horizontalScrollBar() || watched == moduleScroll->verticalScrollBar())) {
            updateScrollAreaFocusPolicy();
        }
    }
    return QWidget::eventFilter(watched, event);
}

#include "moc_ModuleView.cpp"
#include "ModuleView.moc"
