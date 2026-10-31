// SPDX-License-Identifier: MIT
// Exercises the actual actions against temporary images/configuration only.
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QListWidget>
#include <QScreen>
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidget>

static void verifyDialogShellSurface(QDialog *dialog)
{
    require(dialog->findChildren<QWidget *>("dialogEntrance", Qt::FindDirectChildrenOnly).size() == 1,
            "dialog has one entrance animation even after native Show reentry");
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
    const QPoint fieldStart = button->mapToGlobal(QPoint(0, button->height()));
    const QPoint menuStart = menu->mapToGlobal(QPoint(14,14));
    fprintf(stderr, "DROPDOWN: field=%d,%d body=%d,%d\n", fieldStart.x(), fieldStart.y(), menuStart.x(), menuStart.y());
    require(qAbs(menuStart.x() - fieldStart.x()) <= 1 && qAbs(menuStart.y() - fieldStart.y() - 6) <= 1,
            "language dropdown material aligns with field and keeps a six-pixel gap");
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
    auto verifyTheme = [&](bool dark) {
        for (auto card : dialog->findChildren<QFrame *>()) {
            if (!card->property("gwenviewSettingsCard").toBool() || !card->isVisible()) continue;
            const auto pixels = card->grab().toImage();
            const qreal scale = pixels.devicePixelRatio();
            const QColor fill = pixels.pixelColor(qRound((card->width() - 30) * scale), qRound((card->height() - 30) * scale));
            require((fill.lightness() < 128) == dark, "settings cards render the current theme, not cached stylesheet colors");
            require((card->palette().color(QPalette::WindowText).lightness() > 128) == dark,
                    "settings text keeps contrast with the current card material");
        }
    };
    verifyTheme(!wasDark);
    writeTheme(originalTheme);
    require((dialog->palette().color(QPalette::Window).lightness() < 128) == wasDark, "dialog restores original material live");
    verifyTheme(wasDark);
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

static void verifyMouseMenuHover(QWidget *window, const QString &output)
{
#ifdef GWENVIEW_NATIVE_MENU_INPUT
    window->showMaximized(); window->activateWindow(); QTest::qWait(300);
    GwenviewMenuInput input;
    auto move = [&](QPoint target, QWidget *widget) {
        input.move(target, widget->devicePixelRatioF()); QTest::qWait(25);
        // Qt's nested Wayland window coordinates can omit the native titlebar.
        input.move(target + (target - QCursor::pos()), widget->devicePixelRatioF()); QTest::qWait(35);
    };
    int hovers = 0;
    for (int kind = 0; kind < 3; ++kind) {
        QWidget *target = nullptr;
        if (kind == 0) for (auto child : window->findChildren<QWidget *>()) {
            if (child->inherits("Gwenview::ViewMainPage")) target = child;
        }
        else if (kind == 1) target = window->findChild<QAbstractItemView *>("recentImagesView");
        else {
            require(QMetaObject::invokeMethod(window, "slotStartMainPageUrlSelected", Q_ARG(QUrl,
                QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES") + "/Gallery"))), "open thumbnail hover fixture");
            QTest::qWait(350);
            auto browser = window->findChild<QListView *>("mThumbnailView");
            require(browser && browser->model()->rowCount()>1,"thumbnail fixture loaded");
            browser->setCurrentIndex(browser->model()->index(1,0));
            browser->selectionModel()->select(browser->currentIndex(),QItemSelectionModel::ClearAndSelect);
            target = browser;
        }
        require(target, "context menu target exists");
        move(window->mapToGlobal(QPoint(420, 170)), window);
        bool inspected = false;
        QTimer::singleShot(50, window, [&] {
            auto menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            require(menu, "actual context menu opens");
            require(!menu->graphicsEffect(), "native menu never swaps a cached window effect on hover");
            const QSize size = menu->size();
            QHash<QAction *, QRect> rows;
            for (auto action : menu->actions()) rows.insert(action, menu->actionGeometry(action));
            for (int pass=0; pass<2; ++pass) for (auto action : menu->actions()) {
                if (action->isSeparator() || !action->isEnabled() || action->menu()) continue;
                move(menu->mapToGlobal(menu->actionGeometry(action).center()), menu);
                require(menu->activeAction() == action, "physical pointer selects the intended menu row");
                require(menu->size() == size, "hover never resizes menu");
                for (auto retained : menu->actions()) require(rows[retained] == menu->actionGeometry(retained), "hover never shifts menu rows");
                ++hovers;
            }
            // Capture the compositor first: QWidget::grab repaints and can hide
            // partial-update artifacts left on the real window surface.
            require(gwenviewCompositorCapture().save(output + QString("/mouse-context-%1.png").arg(kind)), "capture actual hovered window");
            const auto pixels = menu->grab().toImage(); const qreal scale = pixels.devicePixelRatio();
            for (QPoint point : {QPoint(0,0), QPoint(menu->width()-1,0), QPoint(0,menu->height()-1), QPoint(menu->width()-1,menu->height()-1)})
                require(pixels.pixelColor(qRound(point.x()*scale),qRound(point.y()*scale)).alpha()==0,"hover preserves transparent outer corners");
            QTest::keyClick(menu, Qt::Key_Escape); inspected = true;
        });
        if (kind == 1) require(QMetaObject::invokeMethod(target,"customContextMenuRequested",Q_ARG(QPoint,QPoint(40,40))),"open recent-image context menu");
        else require(QMetaObject::invokeMethod(target,"showContextMenu"),"open native context menu");
        for(int i=0;i<40 && !inspected;++i) QTest::qWait(50);
        require(inspected,"mouse hover walkthrough completed");
    }
    fprintf(stderr,"PASS: %d physical pointer hovers across viewer, sidebar and thumbnail menus; stable geometry and rounded corners\n",hovers);
#else
    require(false,"mouse hover check needs private native compositor");
#endif
}

static void verifyAnnotationTools(QDialog *dialog, const QString &output)
{
    QList<QDockWidget *> docks;
    for (auto dock : dialog->findChildren<QDockWidget *>()) if (dock->inherits("kImageAnnotator::AnnotationDockWidget")) docks << dock;
    require(docks.size()==4,"all native annotation toolbars retained");
    auto check = [&] {
        for (auto dock : docks) {
            auto handle = dock->titleBarWidget(); auto grip = handle ? handle->findChild<QLabel *>() : nullptr;
            require(grip && handle->inherits("kImageAnnotator::AnnotationDockWidgetDragHandle"),"original draggable title widget retained");
            const bool vertical = dock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
            require(grip->size() == (vertical ? QSize(4,18) : QSize(18,4)),"grip follows dock orientation");
            require((grip->geometry().center()-handle->rect().center()).manhattanLength()<=2,"grip centered inside native drag strip");
            require(dock->rect().contains(QRect(handle->pos()+grip->pos(),grip->size())),"grip stays within toolbar");
            for (auto button : dock->widget()->findChildren<QToolButton *>()) require(button->iconSize()==QSize(18,18),"annotation icons use balanced 18px size");
            for (auto host : dock->widget()->findChildren<QWidget *>()) {
                if (!host->isVisible() || (!host->inherits("kImageAnnotator::ToolPicker") && !host->inherits("kImageAnnotator::ZoomPicker"))) continue;
                QRect items;
                for (auto child : host->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
                    if (child->isVisible() && !child->isWindow()) items = items.united(child->geometry());
                if (items.isEmpty()) continue;
                const int offset = vertical ? items.center().y() - host->contentsRect().center().y()
                                            : items.center().x() - host->contentsRect().center().x();
                require(qAbs(offset) <= 1, "annotation picker items center on toolbar cross axis");
            }
        }
    };
    QTest::qWait(100); check();
    require(dialog->grab().save(output+"/annotation-tools-default.png"),"capture default annotation layout");
    auto main = qobject_cast<QMainWindow *>(docks.first()->parentWidget()); require(main,"native docking container retained");
    for (auto dock : docks) {
        const auto original = main->dockWidgetArea(dock);
        for (auto area : {Qt::TopDockWidgetArea,Qt::RightDockWidgetArea,Qt::BottomDockWidgetArea,original}) {
            main->addDockWidget(area,dock); QTest::qWait(100); check();
        }
    }
    dialog->resize(dialog->width()-100,dialog->height()-80); QTest::qWait(120); check();
    require(dialog->grab().save(output+"/annotation-tools-polished.png"),"capture aligned annotation toolbars");
    fprintf(stderr,"PASS: four native annotation docks, inset centered grips, 18px icons, redocking and window resize\n");
}

static void verifyFileAssociations(QWidget *window, const QString &output)
{
    findAction(window, "file_show_properties")->trigger(); QTest::qWait(350);
    auto properties = window->findChild<QDialog *>("gwenviewPropertiesDialog");
    require(properties && properties->isVisible(), "Properties opens for association test");
    auto change = properties->findChild<QPushButton *>("configureMimeBtn");
    require(change && change->isEnabled(), "file-association Change button available");
    change->click(); QTest::qWait(500);
    auto editor = properties->findChild<QDialog *>("gwenviewFileTypeDialog");
    require(editor && editor->isVisible() && editor->isModal(), "file associations open in Gwenview-owned modal dialog");
    verifyDialogShellSurface(editor);
    bool selected = false;
    for (auto tree : editor->findChildren<QTreeWidget *>()) {
        auto item = tree->currentItem();
        if (item && item->parent()) selected |= item->parent()->text(0) + '/' + item->text(0) == "image/png";
    }
    require(selected, "file associations select the Properties image MIME type");
    require(editor->grab().save(output + "/properties-file-associations.png"), "capture themed file associations");
    QPushButton *addApplication = nullptr;
    for (auto widget : editor->findChildren<QWidget *>()) {
        if (!widget->inherits("KServiceListWidget") || !widget->isVisible()) continue;
        for (auto button : widget->findChildren<QPushButton *>())
            if (button->text().remove('&').startsWith("Add")) addApplication = button;
    }
    require(addApplication, "native application preference Add action retained");
    bool inspected = false;
    QTimer::singleShot(350, editor, [&] {
        auto chooser = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        require(chooser && chooser != editor, "nested application chooser opens");
        verifyDialogShellSurface(chooser);
        require(chooser->grab().save(output + "/properties-choose-application.png"), "capture nested application chooser");
        chooser->reject(); inspected = true;
    });
    addApplication->click();
    require(inspected && editor->isVisible(), "cancel nested chooser preserves file-association editor");
    for (auto tabs : editor->findChildren<QTabWidget *>()) {
        require(tabs->tabBar()->property("gwenviewDialogPill").toBool(), "association views use shared pill selector");
        for (int i=0; i<tabs->count(); ++i) { tabs->setCurrentIndex(i); QTest::qWait(240); }
    }
    editor->reject(); QTest::qWait(80);
    require(properties->isVisible(), "cancelling associations returns to Properties");
    properties->reject();
    fprintf(stderr, "PASS: native file-association module, matching shell and pill tabs, selected MIME type and cancellation\n");
}

static void verifyNativeDropdowns(QWidget *surface, const QString &name, const QString &output)
{
#ifdef GWENVIEW_NATIVE_MENU_INPUT
    QList<QPointer<QWidget>> controls;
    for (auto child : surface->findChildren<QWidget *>()) {
        if (!child->isVisible() || !child->isEnabled() || child->window() != surface->window()) continue;
        if (auto combo = qobject_cast<QComboBox *>(child); combo && combo->count()) controls << child;
        if (name.startsWith("status-")) continue;
        if (auto button = qobject_cast<QPushButton *>(child); button && button->menu()) controls << child;
        if (auto button = qobject_cast<QToolButton *>(child); button && button->menu()) controls << child;
    }
    require(!controls.isEmpty(), qPrintable("dropdown controls found: " + name));
    GwenviewMenuInput input;
    int count = 0;
    for (auto field : controls) {
        if (!field || !field->isVisible()) continue;
        const qreal scale = field->devicePixelRatioF();
        const auto owner = gwenviewNativeRect(field->window());
        const QPoint local = field->mapTo(field->window(), QPoint());
        const QRect nativeField(owner.topLeft() + QPoint(qRound(local.x()*scale), qRound(local.y()*scale)), field->size()*scale);
        const QPoint click(nativeField.right()-qRound(8*scale),nativeField.center().y());
        const QString label = name + '-' + QString::number(count++);
        fprintf(stderr,"CLICK %s %s %s at=%d,%d owner=%d,%d,%d,%d\n",qPrintable(label),field->metaObject()->className(),qPrintable(field->objectName()),click.x(),click.y(),owner.x(),owner.y(),owner.width(),owner.height());
        bool inspected = false;
        input.move(click, 1); QTest::qWait(60);
        QTimer::singleShot(80, surface, [&] { input.button(false); });
        QTimer::singleShot(450, surface, [&] {
            auto popup = QApplication::activePopupWidget();
            if (!popup || !popup->isVisible()) {
                gwenviewCompositorCapture().save(output + "/native-dropdown-failed-" + label + ".png");
                fprintf(stderr,"FAILED WINDOWS %s\n",QJsonDocument(gwenviewNativeWindows()).toJson(QJsonDocument::Compact).constData());
            }
            require(popup && popup->isVisible(), qPrintable("physical click opens dropdown: " + label));
            QRect buffer;
            const auto windows = gwenviewNativeWindows();
            for (const auto &entry : windows) {
                const auto data = entry.toObject();
                if (data["title"].toString().isEmpty()
                    && qAbs(qRound(data["width"].toDouble())-popup->width()*scale)<2
                    && qAbs(qRound(data["height"].toDouble())-popup->height()*scale)<2)
                    buffer = QRect(qRound(data["x"].toDouble()),qRound(data["y"].toDouble()),qRound(data["width"].toDouble()),qRound(data["height"].toDouble()));
            }
            require(!buffer.isEmpty(), qPrintable("dropdown really mapped by compositor: " + label));
            const QRect body = QRectF(buffer).adjusted(14*scale,14*scale,-14*scale,-18*scale).toRect();
            const QRect screen = field->screen()->availableGeometry();
            const QRect available(screen.topLeft()*scale, screen.size()*scale);
            const int wantedX = field->isRightToLeft() ? nativeField.right()-body.width()+1 : nativeField.left();
            const int expectedX = qBound(available.left(), wantedX, available.right()-body.width()+1);
            const int gap = body.top() >= nativeField.bottom() ? body.top()-nativeField.bottom()-1 : nativeField.top()-body.bottom()-1;
            fprintf(stderr,"NATIVE DROPDOWN %s %s field=%d,%d,%d,%d body=%d,%d,%d,%d gap=%d dx=%d\n",qPrintable(label),field->metaObject()->className(),nativeField.x(),nativeField.y(),nativeField.width(),nativeField.height(),body.x(),body.y(),body.width(),body.height(),gap,body.x()-expectedX);
            require(gwenviewCompositorCapture().save(output + "/native-dropdown-" + label + ".png"), "capture rendered dropdown and owner");
            require(qAbs(body.left()-expectedX)<=1 && qAbs(gap-6*scale)<=1, qPrintable("compositor dropdown aligns to field with six-pixel gap: " + label));
            QTest::keyClick(popup, Qt::Key_Escape);
            if (popup->isVisible()) popup->close();
            inspected = true;
        });
        input.button(true);
        for (int i=0;i<60 && !inspected;++i) QTest::qWait(20);
        require(inspected, "physical dropdown inspection completes");
        QTest::qWait(70);
    }
    fprintf(stderr,"PASS: %d physically opened dropdowns in %s, compositor alignment and captures\n",count,qPrintable(name));
#else
    require(false,"dropdown walkthrough requires native compositor");
#endif
}

static void verifyMenuDialogs(QWidget *window, const QString &output)
{
    const auto fixtures = qEnvironmentVariable("SEQUOIA_GWENVIEW_FIXTURES");
    require(QMetaObject::invokeMethod(window, "openUrl", Q_ARG(QUrl, QUrl::fromLocalFile(fixtures + "/Gallery/scene-00.png"))), "open dialog fixture");
    QTest::qWait(650);
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_DROPDOWNS_ONLY")) {
        const QSize available = window->screen()->availableGeometry().size();
        const int width = qMin(1100, available.width()-200);
        for (auto size : {QSize(width,qMin(760,available.height()*2/3)), QSize(width,available.height()-100)}) {
            window->resize(size); QTest::qWait(150);
#ifdef GWENVIEW_NATIVE_MENU_INPUT
            gwenviewNativeWindows("workspace.windowList().filter(w=>w.caption.includes('Gwenview')).forEach(w=>{w.frameGeometry={x:150,y:40,width:w.frameGeometry.width,height:w.frameGeometry.height};});");
#endif
            QTest::qWait(150);
            verifyNativeDropdowns(window, "status-" + QString::number(size.height()), output);
        }
        return;
    }
    if (qEnvironmentVariableIsSet("SEQUOIA_DIALOG_ASSOCIATIONS_ONLY")) { verifyFileAssociations(window, output); return; }
    if (qEnvironmentVariableIsSet("SEQUOIA_MOUSE_HOVER_ONLY")) { verifyMouseMenuHover(window, output); return; }
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
    if (qEnvironmentVariable("SEQUOIA_DIALOG_ACTIONS").isEmpty()) verifyProperties(window, output);
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
            if (name == QLatin1String("annotate") && dialog->findChild<QObject *>(QStringLiteral("annotationToolsPolish")))
                verifyAnnotationTools(dialog, output);
            verifyDialogShellSurface(dialog);
            if (qEnvironmentVariableIsSet("SEQUOIA_DROPDOWN_WALKTHROUGH")) {
                auto nestedDropdowns = [&](QPushButton *button, const QString &label) {
                    require(button && button->isEnabled(), qPrintable("nested dropdown dialog available: " + label));
                    bool nestedDone = false;
                    QTimer::singleShot(400, dialog, [&] {
                        auto nested = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                        require(nested && nested != dialog, "nested dropdown dialog opens");
                        verifyNativeDropdowns(nested, label, output);
                        nested->reject(); nestedDone = true;
                    });
                    button->click();
                    for (int i=0;i<80 && !nestedDone;++i) QTest::qWait(50);
                    require(nestedDone, "nested dropdown inspection completes");
                };
                if (name == "file_create_folder") {
                    auto section = dialog->findChild<QWidget *>("chooseIconBox");
                    require(section, "folder icon section found"); section->setProperty("expanded", true); QTest::qWait(250);
                    nestedDropdowns(dialog->findChild<QPushButton *>("chooseIconButton"), "folder-icon-selector");
                } else {
                    verifyNativeDropdowns(dialog, name, output);
                    if (name == "file_print") {
                        QPushButton *properties = nullptr;
                        for (auto button : dialog->findChildren<QPushButton *>())
                            if (button->isVisible() && button->text().remove('&').contains("Properties")) properties = button;
                        nestedDropdowns(properties, "print-properties");
                    }
                }
            }
            if (name == QLatin1String("switch_application_language") && !qEnvironmentVariableIsSet("SEQUOIA_DROPDOWN_WALKTHROUGH")) {
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
        if (name == QLatin1String("switch_application_language") && !qEnvironmentVariableIsSet("SEQUOIA_DROPDOWN_WALKTHROUGH")) require(languageChecked, "language selector was exercised");
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
