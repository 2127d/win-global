// Included in the isolated Settings test, using its native window/navigation.
if (qEnvironmentVariableIsSet("SEQUOIA_COLORS_ONLY")) {
    open(nav,QStringLiteral("kcm_colors"));QTest::qWait(700);
    auto find=[window](const QString &name)->QQuickItem* {
        for(auto quick:window->findChildren<QQuickWidget*>())if(quick->rootObject())for(auto item:items(quick->quickWindow()->contentItem()))if(item->objectName()==name)return item;
        return nullptr;
    };
    capture("colors-page");auto page=find("colorsHub");require(page,"Colors presentation missing");
    auto backend=page->property("backend").value<QObject*>();auto settings=page->property("settings").value<QObject*>();require(backend&&settings,"Colors native backend missing");
    auto model=qobject_cast<QAbstractItemModel*>(backend->property("filteredModel").value<QObject*>());require(model,"Colors filter model missing");
    auto nativeModel=backend->property("model").value<QObject*>();require(nativeModel,"Scheme model missing");
    auto viewport=page->property("flickable").value<QQuickItem*>();
    auto click=[&](QQuickItem *item){
        require(item,"Colors control missing");auto content=viewport->property("contentItem").value<QQuickItem*>();
        for(auto parent=item;parent;parent=parent->parentItem())if(parent==content){viewport->setProperty("contentY",qBound(0.,item->mapToItem(content,QPointF()).y()-20.,qMax(0.,viewport->property("contentHeight").toReal()-viewport->height())));break;}
        QTest::qWait(80);QTest::mouseClick(item->window(),Qt::LeftButton,Qt::NoModifier,item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint());QTest::qWait(200);
    };
    require(find("colorsBack")&&find("settingsBack"),"Back controls missing");
    const int all=model->rowCount();require(all>4,"Color schemes missing");
    auto search=find("colorsSearch");search->setProperty("text","No matching color scheme fixture");QTest::qWait(250);
    require(model->rowCount()==0&&find("colorsEmpty")->isVisible(),"Native Colors search failed");capture("colors-empty");click(find("colorsClearFilters"));require(model->rowCount()==all,"Clear filters failed");
    auto filter=find("colorsFilter");filter->forceActiveFocus();QTest::keyClick(filter->window(),Qt::Key_End);QTest::qWait(150);require(model->rowCount()>0&&model->rowCount()<all,"Dark scheme filter failed");QTest::keyClick(filter->window(),Qt::Key_Home);QTest::qWait(150);
    auto original=nativeModel->property("selectedScheme").toString();
    auto target=original=="SequoiaElevenDarkSolid"?QString("SequoiaElevenLightSolid"):QString("SequoiaElevenDarkSolid");
    click(find("colorsCard-"+target));require(nativeModel->property("selectedScheme").toString()==target&&find("colorsApply")->isEnabled(),"Scheme selection did not stage");
    click(find("colorsReset"));require(nativeModel->property("selectedScheme").toString()==original&&!backend->property("needsSave").toBool(),"Colors Reset failed");
    click(find("colorsCard-"+target));click(find("colorsApply"));QTest::qWait(300);
    KConfig config("kdeglobals");config.reparseConfiguration();require(config.group("General").readEntry("ColorScheme",QString())==target,"Native color scheme Apply did not persist");
    backend->setProperty("accentColorFromWallpaper",true);QTest::qWait(100);click(find("colorsAccent-e93a9a"));
    require(!backend->property("accentColorFromWallpaper").toBool()&&backend->property("accentColor").value<QColor>()==QColor("#e93a9a")&&find("colorsApply")->isEnabled(),"Swatch did not switch automatic accent to custom/stage");
    click(find("colorsApply"));QTest::qWait(300);config.reparseConfiguration();require(config.group("General").readEntry("AccentColor",QColor())==QColor("#e93a9a"),"Accent Apply did not persist");
    click(find("colorsCustomAccent"));auto dialog=page->findChild<QObject*>("colorsColorDialog");require(dialog&&dialog->property("visible").toBool(),"Shared color chooser did not open");capture("colors-custom-color");
    auto hex=find("colorsCustomAccentHex");require(hex,"Hex input missing");hex->setProperty("text","#12");QTest::qWait(100);require(!find("colorsCustomAccentApply")->isEnabled(),"Invalid color accepted");
    QMetaObject::invokeMethod(dialog,"chooseColor",Q_ARG(QVariant,QVariant("#123456")));click(find("colorsCustomAccentCancel"));require(backend->property("accentColor").value<QColor>()==QColor("#e93a9a"),"Cancel changed accent");
    click(find("colorsCustomAccent"));QMetaObject::invokeMethod(dialog,"chooseColor",Q_ARG(QVariant,QVariant("#123456")));click(find("colorsCustomAccentApply"));require(backend->property("accentColor").value<QColor>()==QColor("#123456"),"Custom color was not staged");click(find("colorsReset"));
    auto openActions=[&](const QString &id){auto card=find("colorsCard-"+id);require(card,"Color scheme card missing");click(card->property("actionsButton").value<QQuickItem*>());auto menu=card->findChild<QObject*>(card->objectName()+"Menu");require(menu&&menu->property("visible").toBool(),"Scheme menu missing");return menu;};
    auto action=[](QObject *menu,const QString &text)->QQuickItem* {for(int i=0;i<menu->property("count").toInt();++i){QQuickItem *item=nullptr;QMetaObject::invokeMethod(menu,"itemAt",Q_RETURN_ARG(QQuickItem*,item),Q_ARG(int,i));if(item&&item->property("text").toString()==text)return item;}return nullptr;};
    auto menu=openActions(target);require(!action(menu,"Remove color scheme")->isEnabled(),"Active scheme is removable");QMetaObject::invokeMethod(menu,"close");QTest::qWait(200);
    menu=openActions("SequoiaTestRemovable");capture("colors-actions");click(find("colorsCard-SequoiaTestRemovable")->property("actionsButton").value<QQuickItem*>());require(!menu->property("visible").toBool(),"Scheme menu reopened on second click");
    menu=openActions("SequoiaTestRemovable");click(action(menu,"Remove color scheme"));require(find("colorsCard-SequoiaTestRemovable")->property("pendingRemoval").toBool(),"Removal not staged");
    menu=openActions("SequoiaTestRemovable");click(action(menu,"Don’t delete this color scheme"));require(!find("colorsCard-SequoiaTestRemovable")->property("pendingRemoval").toBool(),"Undo removal failed");
    menu=openActions("SequoiaTestRemovable");click(action(menu,"Remove color scheme"));click(find("colorsApply"));QTest::qWait(250);require(!QFileInfo::exists(qEnvironmentVariable("XDG_DATA_HOME")+"/color-schemes/SequoiaTestRemovable.colors"),"Native scheme removal failed");
    const int beforeInstall=model->rowCount();QMetaObject::invokeMethod(backend,"installSchemeFromFile",Q_ARG(QUrl,QUrl::fromLocalFile(qEnvironmentVariable("SEQUOIA_COLORS_INSTALL_FIXTURE"))));
    for(int n=0;n<80&&model->rowCount()==beforeInstall;++n)QTest::qWait(100);require(model->rowCount()>beforeInstall,"Native scheme import failed");
    click(find("colorsAccent-3daee9"));QTimer::singleShot(150,[]{for(auto widget:QApplication::topLevelWidgets())if(auto dialog=qobject_cast<QDialog*>(widget))dialog->reject();});click(find("colorsBack"));require(nav->property("moduleOpen").toBool()&&backend->property("needsSave").toBool(),"Back bypassed unsaved changes");click(find("colorsReset"));
    auto theme=page->property("theme").value<QObject*>();viewport->setProperty("contentY",0);
    for(const QString &material:{"solid","transparent","glass"})for(const QString &mode:{"light","dark"}){theme->setProperty("material",material);theme->setProperty("mode",mode);QTest::qWait(150);capture("colors-"+material+"-"+mode);}
    for(int width:{1450,930}){window->resize(width,850);QTest::qWait(200);auto grid=find("colorsGrid");QQuickItem *first=nullptr;for(auto item:grid->childItems())if(item->objectName().startsWith("colorsCard-")){if(!first)first=item;require(item->width()<=361,"Color card exceeds width cap");}require(first&&qAbs(first->x())<1&&qAbs(first->y())<1,"Color gallery has empty leading cells");capture("colors-width-"+QString::number(width));}
    click(find("colorsBack"));require(!nav->property("moduleOpen").toBool(),"Round Back did not leave safely");
    fprintf(stderr,"PASS: Colors native schemes/search/filter, selection/Apply/Reset, accent persistence, automatic-to-custom, chooser validation/cancel/accept, protected removal/undo/import, menu toggle, safe Back, six materials/modes and bounded layouts\n");QCoreApplication::exit(0);return;
}
