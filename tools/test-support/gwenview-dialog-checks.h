// SPDX-License-Identifier: MIT
// Exercises the actual actions against temporary images/configuration only.
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QListWidget>
#include <QScreen>

static void verifyDialogShellSurface(QDialog *dialog)
{
    dialog->grab(); // Resolve any deferred layout/entrance cancellation before sampling.
    QTest::qWait(240);
    QImage surface(dialog->size(), QImage::Format_ARGB32_Premultiplied); surface.fill(Qt::transparent);
    dialog->render(&surface, QPoint(), QRegion(), QWidget::RenderFlags());
    const int alpha = qAlpha(surface.pixel(dialog->width()/2, 28));
    if (alpha < 175) fprintf(stderr, "MISSING MATERIAL: %s pixel alpha=%d palette alpha=%d\n", dialog->metaObject()->className(), alpha, dialog->palette().color(QPalette::Window).alpha());
    if (alpha < 175) { surface.save(qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY") + "/missing-surface.png"); dialog->grab().save(qEnvironmentVariable("SEQUOIA_GWENVIEW_VERIFY") + "/missing-dialog.png"); }
    require(alpha >= 175, "dialog paints visible material independently of blur and child content");
    require(qAlpha(surface.pixel(0,0)) == 0, "dialog shadow retains clear outer corners");
    auto child = dialog->findChild<QWidget *>("dialogShellHeader");
    if (dialog->minimumWidth() < dialog->maximumWidth()) {
        QMouseEvent edge(QEvent::MouseMove, QPointF(16, dialog->height()/2), QPointF(dialog->mapToGlobal(QPoint(16, dialog->height()/2))), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(dialog, &edge);
        require(dialog->cursor().shape() == Qt::SizeHorCursor, "dialog edge offers horizontal resize");
        QEnterEvent enter(QPointF(40,20), QPointF(40,20), QPointF(child->mapToGlobal(QPoint(40,20))));
        QApplication::sendEvent(child, &enter);
        require(dialog->cursor().shape() == Qt::ArrowCursor, "entering child content clears inherited resize cursor");
        QApplication::sendEvent(dialog, &edge);
        QEvent leave(QEvent::Leave); QApplication::sendEvent(dialog, &leave);
        require(dialog->cursor().shape() == Qt::ArrowCursor, "leaving dialog clears resize cursor");
    }
    for (auto box : dialog->findChildren<QDialogButtonBox *>()) if (box->isVisible()) {
        QImage footer(box->size(), QImage::Format_ARGB32_Premultiplied); footer.fill(Qt::transparent);
        box->render(&footer, QPoint(), QRegion(), QWidget::RenderFlags());
        require(footer.pixel(box->width()/2,0) == footer.pixel(box->width()/2,2), "dialog footer paints no separator line");
    }
    for (auto frame : dialog->findChildren<QFrame *>())
        require(!frame->isVisible() || frame->frameShape() != QFrame::HLine, "dialog has no visible horizontal separators");
}

static void verifyLanguagePopup(QDialog *dialog, const QString &output)
{
    QPushButton *button = nullptr;
    for (auto candidate : dialog->findChildren<QPushButton *>())
        if (candidate->menu() && candidate->menu()->objectName() == "gwenviewLanguageMenu") { button = candidate; break; }
    if (!button) {
        fprintf(stderr, "LANG DIALOG: %s | %s\n", dialog->metaObject()->className(),qPrintable(dialog->windowTitle()));
        for (auto child : dialog->findChildren<QWidget *>()) fprintf(stderr,"LANG CHILD: %s | %s\n",child->metaObject()->className(),qPrintable(child->objectName()));
    }
    require(button, "language selection uses bounded list presentation");
    auto menu = button->menu();
    menu->popup(button->mapToGlobal(QPoint(0, button->height()))); QTest::qWait(240);
    auto list = menu->findChild<QListWidget *>("gwenviewLanguageList");
    require(list && list->count() > 20 && list->height() <= 360, "all languages are present in a bounded viewport");
    require(!list->verticalScrollBar()->isVisible() && list->verticalScrollBar()->maximum() > 0, "language list scrolls with scrollbar hidden");
    require(menu->screen()->availableGeometry().contains(menu->geometry()), "language popup stays within available screen");
    list->setFocus();
    const QPoint wheelPoint = list->viewport()->rect().center();
    list->verticalScrollBar()->setValue(0);
    QWheelEvent wheel(wheelPoint, list->viewport()->mapToGlobal(wheelPoint), QPoint(), QPoint(0,-120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(list->viewport(), &wheel); QTest::qWait(50);
    require(list->verticalScrollBar()->value() > 0, "hidden language scrollbar still accepts wheel scrolling");
    QTest::keyClick(menu, Qt::Key_End); QTest::qWait(80);
    const auto itemRect = list->visualItemRect(list->currentItem());
    require(list->currentRow() == list->count()-1 && list->viewport()->rect().contains(itemRect.center()), "keyboard reaches last language without clipping");
    require(menu->grab().save(output + "/language-list.png"), "capture bounded language list");
    const QString selected = list->currentItem()->text();
    QTest::keyClick(menu, Qt::Key_Return); QTest::qWait(80);
    require(!menu->isVisible() && button->text().remove('&') == selected, "language list selection invokes original KDE action");
    menu->popup(button->mapToGlobal(QPoint(0,button->height()))); QTest::qWait(240);
    QTest::keyClick(menu, Qt::Key_Escape); QTest::qWait(40);
    require(!menu->isVisible() && dialog->isVisible(), "Escape closes only language list");
}

class FilePickerResizeProbe : public QObject
{
public:
    QList<int> widths;
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::Resize) widths << static_cast<QWidget *>(object)->width();
        return false;
    }
};

static void verifyFilePickerSidebar(QWidget *window, const QString &output)
{
    auto open = [&]() {
        findAction(window, "file_open")->trigger(); QTest::qWait(350);
        QDialog *dialog = nullptr;
        for (auto widget : QApplication::topLevelWidgets())
            if (auto candidate = qobject_cast<QDialog *>(widget); candidate && candidate->inherits("KDEPlatformFileDialog") && candidate->isVisible()) dialog = candidate;
        require(dialog, "Open File dialog opens");
        return dialog;
    };
    auto findPlaces = [](QDialog *dialog) {
        for (auto widget : dialog->findChildren<QWidget *>()) if (widget->inherits("KFilePlacesView")) return widget;
        return static_cast<QWidget *>(nullptr);
    };
    auto dialog = open();
    auto places = findPlaces(dialog);
    require(places, "Open File has native Places sidebar");
    QSplitter *splitter = nullptr;
    for (QWidget *pane = places; pane && pane != dialog; pane = pane->parentWidget())
        if ((splitter = qobject_cast<QSplitter *>(pane->parentWidget()))) break;
    require(splitter && splitter->count() == 2, "Places and file list share a splitter");
    const int width = places->width();
    FilePickerResizeProbe probe; places->installEventFilter(&probe);
    dialog->resize(1100, 700); QTest::qWait(150);
    const int wideFiles = splitter->widget(1)->width();
    require(dialog->grab().save(output + "/file-picker-wide.png"), "capture wide picker");
    dialog->resize(720, 500); QTest::qWait(150);
    require(splitter->widget(1)->width() < wideFiles, "file list absorbs the window resize");
    require(dialog->grab().save(output + "/file-picker-narrow.png"), "capture narrow picker");
    for (const int target : {640, 560, 480, 400, 320, 1100, 800, 700, 1100}) {
        dialog->resize(target,500); QTest::qWait(30);
        require(places->width() == width, "Places width stays fixed at each supported window size");
    }
    for (const int intermediate : probe.widths)
        require(intermediate == width, "Places never squeezes even during intermediate resize events");
    probe.widths.clear();
    // Drag the real divider: keeping window resizes stable must not lock the
    // user's ability to choose and persist a different sidebar width.
    auto handle = splitter->handle(1);
    const QPoint start = handle->rect().center(), end = start + QPoint(40, 0);
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, start);
    QMouseEvent move(QEvent::MouseMove, QPointF(end), QPointF(handle->mapToGlobal(end)), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(handle, &move);
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, handle->rect().center());
    QTest::qWait(150);
    const int chosenWidth = places->width();
    require(chosenWidth > width, "divider can still enlarge the Places sidebar manually");
    probe.widths.clear();
    dialog->resize(720,500); QTest::qWait(150);
    dialog->resize(1100,650); QTest::qWait(150);
    require(places->width() == chosenWidth, "manual sidebar width survives window resizes");
    for (const int intermediate : probe.widths) require(intermediate == chosenWidth, "manual width has no resize flicker");
    places->removeEventFilter(&probe);
    dialog->reject(); QTest::qWait(150);
    dialog = open(); places = findPlaces(dialog);
    require(places && places->width() == chosenWidth, "chosen Places width survives closing and reopening the file picker");
    dialog->reject(); QTest::qWait(100);
    fprintf(stderr, "PASS: Places width stays fixed throughout resize, file list flexes, divider adjusts manually, chosen width persists\n");
}

static void verifySettingsBehavior(QWidget *window, const QString &output)
{
    auto open = [&]() {
        findAction(window, "options_configure")->trigger(); QTest::qWait(200);
        auto dialog = window->findChild<QDialog *>("Settings");
        require(dialog && dialog->isVisible(), "settings retains its registered object name");
        return dialog;
    };
    auto dialog = open();
    auto tabs = dialog->findChild<QTabWidget *>();
    require(tabs && tabs->count() == 3, "all three preferences pages preserved");
    auto bar = tabs->tabBar();
    auto pill = bar->findChild<QVariantAnimation *>("dialogPillAnimation");
    auto page = tabs->findChild<QVariantAnimation *>("dialogPageAnimation");
    require(pill && page && pill->duration() == 200 && page->duration() == 200, "pill and page motion share app timing");
    tabs->setCurrentIndex(1); QTest::qWait(40);
    require(pill->state() == QAbstractAnimation::Running && page->state() == QAbstractAnimation::Running,
            "pill and content visibly animate together");
    require(dialog->grab().save(output + "/settings-switching.png"), "capture in-flight settings transition");
    tabs->setCurrentIndex(2); QTest::qWait(20); tabs->setCurrentIndex(0); QTest::qWait(240);
    require(page->state() == QAbstractAnimation::Stopped && pill->state() == QAbstractAnimation::Stopped, "rapid page changes settle");
    bar->setFocus(); QTest::keyClick(bar, Qt::Key_Right); QTest::qWait(240);
    require(tabs->currentIndex() == 1, "keyboard pill navigation");
    tabs->setCurrentIndex(0); QTest::qWait(30); dialog->resize(560, 440); QTest::qWait(240);
    require(page->state() == QAbstractAnimation::Stopped, "resize releases cached page");
    require(dialog->width() == 560 && dialog->height() == 440, "settings actually fits the requested compact size");
    auto scrolls = dialog->findChildren<QScrollArea *>("gwenviewSettingsScroll");
    require(scrolls.size() == 3, "every preference page is scrollable");
    for (int i = 0; i < tabs->count(); ++i) {
        tabs->setCurrentIndex(i); QTest::qWait(240);
        for (auto scroll : scrolls) if (scroll->isVisible()) {
            require(scroll->verticalScrollBar()->maximum() > 0, "compact settings can reach offscreen controls");
            scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
            require(scroll->horizontalScrollBar()->maximum() == 0, "compact settings avoid sideways scrolling");
            require(dialog->grab().save(output + "/settings-compact-bottom-" + QString::number(i) + ".png"), "capture compact page bottom");
        }
    }
    NoSidebarMotionStyle noMotion; dialog->setStyle(&noMotion);
    tabs->setCurrentIndex(0); QTest::qWait(20);
    require(pill->state() == QAbstractAnimation::Stopped && page->state() == QAbstractAnimation::Stopped, "reduced motion disables pill and page transitions");
    dialog->setStyle(nullptr);
    auto videos = dialog->findChild<QCheckBox *>("kcfg_ListVideos");
    auto autoplay = dialog->findChild<QCheckBox *>("kcfg_AutoplayVideos");
    auto quality = dialog->findChild<QSpinBox *>("jpegQualitySpinner");
    auto slider = dialog->findChild<QSlider *>("kcfg_JPEGQuality");
    require(videos && autoplay && quality && slider, "bound preference controls retained");
    const bool original = videos->isChecked();
    videos->click();
    require(autoplay->isEnabled() == videos->isChecked(), "dependent playback preference updates");
    const int oldQuality = quality->value(); quality->setValue(oldQuality == 80 ? 90 : 80);
    require(slider->value() == quality->value(), "quality spinner and slider remain synchronized");
    const int savedQuality = quality->value();
    auto box = dialog->findChild<QDialogButtonBox *>();
    QTest::qWait(40);
    require(box && box->button(QDialogButtonBox::Apply)->isEnabled(), "Apply tracks changed controls");
    box->button(QDialogButtonBox::Apply)->click(); QTest::qWait(120);
    dialog->reject(); QTest::qWait(100);
    dialog = open();
    videos = dialog->findChild<QCheckBox *>("kcfg_ListVideos");
    require(videos->isChecked() != original && dialog->findChild<QSpinBox *>("jpegQualitySpinner")->value() == savedQuality,
            "Apply persists settings across dialog reopen");
    videos->click(); dialog->reject(); QTest::qWait(100);
    dialog = open();
    require(dialog->findChild<QCheckBox *>("kcfg_ListVideos")->isChecked() != original, "Cancel discards pending settings");
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdeglobals";
    QFile current(configPath); require(current.open(QIODevice::ReadOnly), "read isolated theme");
    const QByteArray originalTheme = current.readAll(); current.close();
    const bool wasDark = dialog->palette().color(QPalette::Window).lightness() < 128;
    QFile alternate(qEnvironmentVariable("SEQUOIA_GWENVIEW_THEMES") + (wasDark ? "/SequoiaElevenLightSolid.colors" : "/SequoiaElevenDarkTransparent.colors"));
    require(alternate.open(QIODevice::ReadOnly), "load alternate theme");
    auto writeTheme = [&](const QByteArray &data) {
        QSaveFile file(configPath); require(file.open(QIODevice::WriteOnly), "write isolated palette");
        require(file.write(data) == data.size() && file.commit(), "atomically update isolated palette");
        QTest::qWait(300);
    };
    writeTheme(alternate.readAll());
    require((dialog->palette().color(QPalette::Window).lightness() < 128) != wasDark, "open dialog updates theme without restart");
    require(dialog->grab().save(output + "/settings-live-theme.png"), "capture live dialog theme");
    writeTheme(originalTheme);
    require((dialog->palette().color(QPalette::Window).lightness() < 128) == wasDark, "dialog restores original material live");
    dialog->reject(); QTest::qWait(100);
    fprintf(stderr, "PASS: settings binding, Apply/Cancel persistence, keyboard pill, page motion, rapid reversal, resize, scrolling and reduced motion\n");
}

static void verifySecondarySurfaces(QWidget *window, const QString &output)
{
    findAction(window, "file_show_properties")->trigger(); QTest::qWait(200);
    auto properties = window->findChild<QDialog *>("gwenviewPropertiesDialog");
    auto tabs = properties->findChild<QTabWidget *>();
    for (int i = 0; i < tabs->count(); ++i) if (tabs->tabText(i).contains("Permissions")) tabs->setCurrentIndex(i);
    QTest::qWait(240);
    QPushButton *advanced = nullptr;
    for (auto button : properties->findChildren<QPushButton *>())
        if (button->text().contains("Advanced Permissions")) advanced = button;
    require(advanced, "advanced file permissions remains available");
    bool nested = false;
    QTimer::singleShot(250, properties, [&] {
        auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        require(dialog && dialog != properties && dialog->property("gwenviewPolishedDialog").toBool(), "nested permission dialog shares polish");
        require(dialog->grab().save(output + "/properties-advanced-permissions.png"), "capture advanced permissions");
        dialog->reject(); nested = true;
    });
    advanced->click(); QTest::qWait(300); require(nested, "nested permission dialog exercised");
    properties->reject(); QTest::qWait(100);
    for (const char *name : {"crop", "brightness_contrast_gamma", "red_eye_reduction"}) {
        findAction(window, name)->trigger(); QTest::qWait(200);
        auto panel = window->findChild<QWidget *>("gwenviewEditingTools");
        require(panel && panel->isVisible() && panel->height() > 0, "menu opens native inline editing tool");
        if (QString::fromLatin1(name) == "crop") {
            for (auto check : panel->findChildren<QCheckBox *>()) if (check->text().contains("Advanced")) check->setChecked(true);
            QTest::qWait(200);
        }
        QTest::qWait(100);
        for (auto content : panel->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) if (content->isVisible())
            require(content->x() >= 16 && content->y() >= 20 && content->geometry().bottom() <= panel->height()-12,
                    "inline editing controls stay inside padded card");
        QImage card(panel->size(), QImage::Format_ARGB32_Premultiplied); card.fill(Qt::transparent);
        panel->render(&card, QPoint(), QRegion(), QWidget::RenderFlags());
        require(qAlpha(card.pixel(0,0)) == 0 && qAlpha(card.pixel(panel->width()/2, 12)) >= 175,
                "inline editing surface has a clear gap and visible rounded material");
        require(window->grab().save(output + '/' + name + ".png"), "capture inline editing surface");
        QPushButton *cancel = nullptr;
        for (auto box : panel->findChildren<QDialogButtonBox *>()) if (box->isVisible()) {
            cancel = box->button(QDialogButtonBox::Cancel);
            if (!cancel) cancel = box->button(QDialogButtonBox::Close);
            if (cancel) break;
        }
        require(cancel, "inline tool exposes native cancel/close"); cancel->click(); QTest::qWait(260);
        if (panel->height() != 0) {
            fprintf(stderr,"TOOL CLOSE: %s h=%d\n",name,panel->height());
            for (auto anim : panel->findChildren<QVariantAnimation *>()) fprintf(stderr,"TOOL ANIM: state=%d time=%d duration=%d end=%d\n",int(anim->state()),anim->currentTime(),anim->duration(),anim->endValue().toInt());
        }
        require(panel->height() == 0, "inline editing tool closes cleanly");
    }
    QApplication::setActiveWindow(window);
    findAction(window, "open_kcommand_bar")->trigger(); QTest::qWait(240);
    QPointer<QWidget> commands;
    for (auto widget : QApplication::allWidgets()) if (widget->inherits("KCommandBar") && widget->isVisible()) commands = widget;
    require(commands && commands->property("gwenviewFloatingPopup").toBool(), "Find Action uses shared floating popup");
    auto search = commands->findChild<QLineEdit *>();
    require(search, "Find Action retains search input");
    QTest::keyClicks(search, "Properties"); QTest::qWait(150);
    require(!commands->testAttribute(Qt::WA_TranslucentBackground) && !commands->isWindow(), "Find Action is a normal child overlay, not a desktop-transparent surface");
    const QImage backing = window->grab().toImage();
    const QPoint searchPoint = commands->mapTo(window, QPoint(commands->width()/2, 5));
    require(backing.pixelColor(searchPoint * window->devicePixelRatioF()).alpha() > 175, "command search margin does not punch a hole through the application");
    require(commands->grab().save(output + "/find-action.png"), "capture command search");
    QTest::keyClick(search, Qt::Key_Escape); QTest::qWait(100);
    require(!commands || !commands->isVisible(), "command palette cancels through keyboard");
    fprintf(stderr, "PASS: advanced permissions, crop/adjust/red-eye panels, command search and cancellation\n");
}

static void verifyFolderChooser(QWidget *window, const QString &output)
{
    bool inspected = false;
    QTimer::singleShot(450, window, [&] {
        auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        require(dialog, "Create New Folder opens");
        for (auto widget : dialog->findChildren<QWidget *>())
            if (widget->inherits("KCollapsibleGroupBox")) widget->setProperty("expanded", true);
        QTest::qWait(350);
        QTest::mouseMove(dialog, QPoint(5, 5));
        QList<QToolButton *> choices;
        QToolButton *bookmark = nullptr;
        for (auto button : dialog->findChildren<QToolButton *>()) {
            const auto name = button->toolTip();
            if (!name.startsWith("folder-")) continue;
            choices << button;
            // Test every idle choice before moving the pointer over any icon.
            // This fails if a compact symbolic sample flattens colored artwork.
            const auto size = button->iconSize();
            const auto dpr = button->devicePixelRatioF();
            require(button->isVisible(), "expanded folder choice is visible");
            require(button->icon().pixmap(size, dpr).toImage() == QIcon::fromTheme(name).pixmap(size, dpr).toImage(),
                    qPrintable("idle folder artwork retains theme colors: " + name));
            if (name == "folder-tar" || name == "folder-bookmark") {
                const auto icon = button->icon().pixmap(size, dpr).toImage();
                int blue = 0;
                for (int y = 0; y < icon.height(); ++y) for (int x = 0; x < icon.width(); ++x) {
                    auto color = icon.pixelColor(x, y);
                    if (color.alpha() > 100 && color.blue() > color.red() + 50) ++blue;
                }
                require(blue > icon.width() * icon.height() / 4, qPrintable("full-color folder artwork: " + name));
            }
            if (name == "folder-bookmark") bookmark = button;
        }
        require(choices.size() == 19 && bookmark, "all nineteen folder variants are present");
        require(dialog->grab().save(output + "/folder-chooser.png"), "capture idle colored folders");
        for (auto button : choices) {
            const auto before = button->icon().pixmap(button->iconSize(), button->devicePixelRatioF()).toImage();
            QTest::mouseMove(button, button->rect().center()); QTest::qWait(30);
            QTest::mouseMove(dialog, QPoint(5, 5)); QTest::qWait(30);
            require(before == button->icon().pixmap(button->iconSize(), button->devicePixelRatioF()).toImage(), "folder color survives hover and leave");
        }
        bookmark->click(); QTest::qWait(150);
        auto edit = dialog->findChild<QLineEdit *>();
        require(edit, "new folder name field is present");
        edit->setText("Sequoia bookmark check");
        require(dialog->grab().save(output + "/folder-chooser-selected.png"), "capture selected bookmark folder");
        auto box = dialog->findChild<QDialogButtonBox *>();
        require(box && box->button(QDialogButtonBox::Ok), "new folder confirmation is present");
        inspected = true;
        box->button(QDialogButtonBox::Ok)->click();
    });
    findAction(window, "file_create_folder")->trigger();
    for (int i = 0; i < 50 && !inspected; ++i) QTest::qWait(100);
    require(inspected, "folder chooser inspection completed");
    const QString folder = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/Gallery/Sequoia bookmark check";
    for (int i = 0; i < 30 && !QFileInfo::exists(folder + "/.directory"); ++i) QTest::qWait(100);
    QFile metadata(folder + "/.directory");
    require(metadata.open(QIODevice::ReadOnly) && metadata.readAll().contains("Icon=folder-bookmark"), "created folder retains selected bookmark icon");
    fprintf(stderr, "PASS: nineteen idle folder choices, colored archive/bookmark art, hover and selected folder creation\n");
}

static void verifyMenuDialogs(QWidget *window, const QString &output)
{
    const auto fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png"))), "open dialog fixture");
    QTest::qWait(650);
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_PROPERTIES_ONLY")) { verifyProperties(window, output); return; }
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_FOLDER_ONLY")) { verifyFolderChooser(window, output); return; }
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_FILE_PICKER_ONLY")) { verifyFilePickerSidebar(window, output); return; }
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_SETTINGS_ONLY")) { verifySettingsBehavior(window, output); return; }
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_SECONDARY_ONLY")) { verifySecondarySurfaces(window, output); return; }
    // Record every exported command, including standard KDE merged actions.
    std::function<void(QMenu *, const QString &)> inventory = [&](QMenu *menu, const QString &path) {
        QMetaObject::invokeMethod(menu, "aboutToShow");
        for (auto action : menu->actions()) {
            if (action->isSeparator()) continue;
            fprintf(stderr, "MENU: %s | %s | %s | %s\n", qPrintable(path), qPrintable(action->objectName()), qPrintable(action->text()), action->isEnabled() ? "enabled" : "disabled");
            if (action->menu()) inventory(action->menu(), path + '/' + action->text());
        }
    };
    for (auto action : window->findChild<QMenuBar *>()->actions()) if (action->menu()) inventory(action->menu(), action->text());
    verifyProperties(window, output);
    auto inspect = [&](QAction *action, const QString &name) {
        require(action && action->isEnabled(), qPrintable("available dialog action: " + name));
        int captured = 0;
        bool languageChecked = false;
        bool checking = false;
        QTimer poll;
        poll.setInterval(220);
        QObject::connect(&poll, &QTimer::timeout, window, [&] {
            if (checking) return;
            QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) for (auto top : QApplication::topLevelWidgets())
                if (auto candidate = qobject_cast<QDialog *>(top); candidate && candidate->isVisible()) dialog = candidate;
            if (!dialog) return;
            checking = true;
            require(dialog->property("gwenviewPolishedDialog").toBool(), qPrintable("shared polish: " + name));
            require(dialog->findChild<QWidget *>("dialogShellHeader") && dialog->findChild<QToolButton *>("dialogCloseButton"),
                    qPrintable("Weather shell and close control: " + name));
            QTest::qWait(170);
            if (name == QLatin1String("file_print")) {
                for (auto button : dialog->findChildren<QPushButton *>())
                    if (button->text().contains("Options")) { button->click(); QTest::qWait(240); break; }
            }
            verifyDialogShellSurface(dialog);
            if (name == QLatin1String("switch_application_language")) {
                bool hasLanguages = false;
                for (auto child : dialog->findChildren<QWidget *>()) if (child->inherits("KLanguageButton")) hasLanguages = true;
                if (hasLanguages) { verifyLanguagePopup(dialog, output); languageChecked = true; }
                else for (auto label : dialog->findChildren<QLabel *>()) fprintf(stderr,"LANG NOTICE: %s\n",qPrintable(label->text()));
            }
            const QString captureName = name + (captured ? "-followup-" + QString::number(captured) : QString());
            require(dialog->grab().save(output + '/' + captureName + ".png"), "capture dialog");
            fprintf(stderr, "DIALOG: %s | %s | %s | %dx%d\n", qPrintable(captureName), dialog->metaObject()->className(), qPrintable(dialog->windowTitle()), dialog->width(), dialog->height());
            for (auto tabs : dialog->findChildren<QTabWidget *>()) {
                require(tabs->tabBar()->property("gwenviewDialogPill").toBool(), "dialog tabs use pill");
                for (int i = 0; i < tabs->count(); ++i) {
                    tabs->setCurrentIndex(i); QTest::qWait(240);
                    require(dialog->grab().save(output + '/' + captureName + "-page-" + QString::number(i) + ".png"), "capture dialog page");
                }
                tabs->setCurrentIndex(0); QTest::qWait(240);
            }
            ++captured;
            dialog->reject();
            checking = false;
        });
        poll.start();
        action->trigger();
        for (int i = 0; i < 60 && !captured; ++i) QTest::qWait(100);
        QTest::qWait(700); // Catch dialogs created after an error/message box exits exec().
        poll.stop();
        require(captured > 0, qPrintable("inspected dialog: " + name));
        if (name == QLatin1String("switch_application_language")) require(languageChecked, "language selector was exercised");
        for (auto top : QApplication::topLevelWidgets())
            require(!qobject_cast<QDialog *>(top) || !top->isVisible(), "no dialog left from previous action");
        QApplication::setActiveWindow(window);
    };
    const char *actions[] = {"file_open", "file_save_as", "file_rename", "file_copy_to", "file_move_to", "file_link_to",
        "file_create_folder", "deletefile", "file_show_properties", "resize", "annotate", "edit_tags", "file_print", "file_print_preview", "options_configure_keybinding",
        "options_configure_toolbars", "options_configure", "help_about_app", "help_about_kde", "help_report_bug", "switch_application_language"};
    const auto onlyActions = qEnvironmentVariable("SEQUOIA_DIALOG_ACTIONS").split(',', Qt::SkipEmptyParts);
    for (auto name : actions) if (onlyActions.isEmpty() || onlyActions.contains(QString::fromLatin1(name))) inspect(findAction(window, name), QString::fromLatin1(name));
    if (!onlyActions.isEmpty()) { fprintf(stderr, "PASS: requested dialog regression checks\n"); return; }
    auto openWith = findAction(window, "file_open_with")->menu();
    QMetaObject::invokeMethod(openWith, "aboutToShow");
    for (auto action : openWith->actions()) if (action->text().contains("Other")) inspect(action, "open-with-other");
    verifySecondarySurfaces(window, output);
    verifySettingsBehavior(window, output);
    verifyFilePickerSidebar(window, output);
    fprintf(stderr, "PASS: global-menu inventory and dialog/page walkthrough\n");
}
