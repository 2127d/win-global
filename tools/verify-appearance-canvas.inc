        if (qEnvironmentVariableIsSet("SEQUOIA_CANVAS_ONLY")) {
            auto find = [window](const QString &name) -> QQuickItem * {
                for (auto quick : window->findChildren<QQuickWidget *>()) if (quick->rootObject())
                    for (auto item : items(quick->quickWindow()->contentItem())) if (item->objectName() == name) return item;
                return nullptr;
            };
            auto click = [&](const QString &name) {
                auto item = find(name); require(item, qPrintable("Missing " + name));
                require(QMetaObject::invokeMethod(item, "clicked"), "Cannot activate theme action"); QTest::qWait(200);
            };
            auto check = [&](const QString &name, const QColor &expected) {
                QTest::qWait(250);
                const QImage image = window->grab().toImage();
                const QPoint logical(window->width()-5, window->height()/2);
                const QColor actual = image.pixelColor(logical * image.devicePixelRatio());
                fprintf(stderr, "CANVAS %s actual=%s alpha=%d expected=%s alpha=%d\n", qPrintable(name),
                        qPrintable(actual.name()), actual.alpha(), qPrintable(expected.name()), expected.alpha());
                capture("canvas-" + name);
                require(qAbs(actual.alpha()-expected.alpha()) <= 1, "Shared canvas opacity differs after a global-theme/style change");
                require(qAbs(actual.red()-expected.red()) <= 2 && qAbs(actual.green()-expected.green()) <= 2
                        && qAbs(actual.blue()-expected.blue()) <= 2, "Shared canvas tint differs after a global-theme/style change");
            };
            for (const QString &mode : {QStringLiteral("dark"), QStringLiteral("light")}) {
                open(nav, "kcm_lookandfeel"); QTest::qWait(400);
                auto page = find("globalThemeHub"); require(page, "Missing global theme page");
                auto backend = page->property("backend").value<QObject *>(); require(backend, "Missing global theme backend");
                const QString package = "org.sequoia.eleven." + mode + ".solid";
                require(QMetaObject::invokeMethod(page, "choosePackage", Q_ARG(QVariant, package)), "Cannot choose solid global theme");
                QTest::qWait(200); click("globalThemeMoreOptions");
                int flags = 0;
                for (auto quick : window->findChildren<QQuickWidget *>()) if (quick->rootObject())
                    for (auto item : items(quick->quickWindow()->contentItem()))
                        if (item->objectName().startsWith("globalThemeOption-")
                            && (item->property("text").toString() == "Colors" || item->property("text").toString() == "Application Style"))
                            flags |= item->objectName().section('-',-1).toInt();
                require(flags != 0 && backend->setProperty("selectedContents", flags), "Cannot select appearance-only theme options");
                click("globalThemeConfirmApply"); QTest::qWait(900);
                auto appearance = window->findChild<QObject *>("appearancePreview"); require(appearance, "Missing appearance controller");
                require(appearance->property("activeMaterial").toString() == "solid", "Global theme did not select solid material");
                const QColor expected = appearance->property("canvasColor").value<QColor>();
                require(expected.alpha() == 255, "Solid global theme has translucent canvas color");
                // A style supplied by another global theme must not be responsible
                // for painting this permanently alpha-enabled application window.
                for (const QString &style : {QStringLiteral("Fusion"), QStringLiteral("SequoiaEleven")}) {
                    require(QApplication::setStyle(style), "Missing test widget style");
                    for (const QString &id : {QStringLiteral("kcm_lookandfeel"), QStringLiteral("kcm_style"), QStringLiteral("kcm_desktoptheme"), QStringLiteral("kcm_colors"), QStringLiteral("kcm_landingpage")}) {
                        open(nav, id); check(mode + '-' + style + '-' + id, expected);
                    }
                    require(call(nav, "browse", "appearance"), "Cannot open Appearance overview");
                    check(mode + '-' + style + "-overview", expected);
                }
            }
            // Palette-only transitions keep their exact alpha rather than
            // accumulating over a stale solid background.
            for (int alpha : {184, 148, 255}) {
                QPalette palette = qApp->palette(); QColor color("#607080"); color.setAlpha(alpha);
                palette.setColor(QPalette::Window, color); qApp->setPalette(palette);
                open(nav, "kcm_style"); check(QString::number(alpha), color);
            }
            fprintf(stderr, "PASS: appearance canvas across native global theme application, all customized pages, style replacement and alpha transitions\n");
            QCoreApplication::exit(0); return;
        }
