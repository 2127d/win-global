// SPDX-License-Identifier: GPL-2.0-or-later
#include "sequoiaelevenstyleconfig.h"
#include "../../../../../desktop/glass-material/tooltippolicy.h"
#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QScrollArea>
#include <QTabBar>
#include <QVBoxLayout>
#include <KLocalizedString>

namespace SequoiaEleven {
class TooltipSample final : public QWidget {
public:
    QString shape;
    explicit TooltipSample(QWidget *parent) : QWidget(parent) { setFixedHeight(70); setMinimumWidth(180); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        QRectF r(5,9,width()-10,52);
        qreal radius=SequoiaGlass::tooltipRadius(shape,r.width(),r.height());
        p.setBrush(palette().color(QPalette::AlternateBase));
        p.setPen(QPen(palette().color(QPalette::Mid),1));
        p.drawRoundedRect(r,radius,radius);
        p.setPen(QPen(QColor(255,255,255,60),1));p.drawRoundedRect(r.adjusted(1,1,-1,-1),qMax(0.,radius-1),qMax(0.,radius-1));
        p.setPen(palette().color(QPalette::Text));p.drawText(r,Qt::AlignCenter,i18n("Tooltip preview"));
    }
};
void StyleConfig::setupHub() {
    setObjectName(QStringLiteral("sequoiaStyleHub"));
    setMinimumSize(820,560);
    auto tooltipPage = new QWidget;
    auto tips = new QVBoxLayout(tooltipPage); tips->setContentsMargins(20,20,20,20); tips->setSpacing(14);
    auto intro = new QLabel(i18n("Choose a tooltip shape for each part of Sequoia.")); intro->setWordWrap(true);tips->addWidget(intro);
    const QStringList sections={QStringLiteral("Applications"),QStringLiteral("Settings"),QStringLiteral("Desktop"),QStringLiteral("RunningApps")};
    const QStringList titles={i18n("Qt applications"),i18n("Appearance Hub and Sequoia settings"),i18n("Desktop and panel"),i18n("Running-app previews")};
    for(int i=0;i<sections.size();++i) {
        auto card=new QFrame;card->setObjectName(QStringLiteral("tooltipChoiceCard"));
        auto row=new QHBoxLayout(card);row->setContentsMargins(16,8,16,8);
        auto label=new QLabel(titles[i]);label->setWordWrap(true);row->addWidget(label,1);
        auto combo=new QComboBox; combo->setObjectName(QStringLiteral("tooltipShape")+sections[i]);
        combo->addItem(i18n("Pill"),QStringLiteral("pill"));combo->addItem(i18n("Rounded rectangle"),QStringLiteral("rounded"));combo->addItem(i18n("Square"),QStringLiteral("square"));
        combo->setToolTip(i18n("Choose the shape used in this section. Apply to save your choice."));
        combo->setCurrentIndex(-1);
        row->addWidget(combo);
        auto sample=new TooltipSample(card); row->addWidget(sample);
        _tooltipChoices.insert(sections[i],combo);
        connect(combo,&QComboBox::currentIndexChanged,this,[this,combo,sample] {sample->shape=combo->currentData().toString();sample->update();updateChanged();});
        tips->addWidget(card);
    }
    auto note=new QLabel(i18n("Running-app previews stay 12 px away from the panel. These choices apply to Sequoia Qt and Plasma surfaces; applications that draw their own tooltips keep their own styling."));
    note->setWordWrap(true);note->setObjectName(QStringLiteral("hubDescription"));tips->addWidget(note);tips->addStretch();
    tabWidget->addTab(tooltipPage,i18n("Tooltips"));
    // Keep every existing page and its native settings lifecycle; replace only navigation.
    tabWidget->tabBar()->hide();tabWidget->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    auto outer=qobject_cast<QVBoxLayout*>(layout());outer->removeWidget(tabWidget);outer->setContentsMargins(12,12,12,12);outer->setSpacing(14);
    auto title=new QLabel(i18n("Sequoia Eleven"));title->setObjectName(QStringLiteral("hubTitle"));outer->insertWidget(0,title);
    auto subtitle=new QLabel(i18n("Application appearance, motion, surfaces and tooltips"));subtitle->setObjectName(QStringLiteral("hubDescription"));outer->insertWidget(1,subtitle);
    auto body=new QHBoxLayout;body->setSpacing(16);
    auto navigation=new QListWidget;navigation->setObjectName(QStringLiteral("styleNavigation"));navigation->setFixedWidth(174);navigation->setSpacing(4);navigation->setFrameShape(QFrame::NoFrame);
    for(int i=0;i<tabWidget->count();++i) {
        const auto title=tabWidget->tabText(i);
        auto item=new QListWidgetItem(title,navigation);item->setSizeHint(QSize(150,38));
        item->setToolTip(title);
        auto page=tabWidget->widget(i);
        if(page->layout()) {page->layout()->setContentsMargins(20,18,20,18);page->layout()->setSpacing(14);}
    }
    connect(navigation,&QListWidget::currentRowChanged,tabWidget,&QTabWidget::setCurrentIndex);
    navigation->setCurrentRow(0);body->addWidget(navigation);body->addWidget(tabWidget,1);outer->addLayout(body,1);
    applyHubPalette();
}
void StyleConfig::applyHubPalette() {
    const auto accent=palette().color(QPalette::Highlight);
    auto muted=palette().color(QPalette::WindowText);muted.setAlpha(180);
    const auto mutedCss=QStringLiteral("rgba(%1,%2,%3,180)").arg(muted.red()).arg(muted.green()).arg(muted.blue());
    const auto tint=[&](int a){return QStringLiteral("rgba(%1,%2,%3,%4)").arg(accent.red()).arg(accent.green()).arg(accent.blue()).arg(a);};
    setStyleSheet(QStringLiteral(R"(
        QWidget#sequoiaStyleHub { background: palette(window); }
        QLabel#hubTitle { font-size: 22px; font-weight: 600; }
        QLabel#hubDescription { color: %4; }
        QListWidget#styleNavigation { background: palette(window); border: 1px solid palette(midlight); border-radius: 22px; padding: 8px; }
        QListWidget#styleNavigation::item { border-radius: 18px; padding-left: 10px; border: 1px solid transparent; }
        QListWidget#styleNavigation::item:selected { color: palette(text); border: 1px solid %1; background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %2,stop:0.5 %3,stop:1 %2); }
        QListWidget#styleNavigation::item:hover { background: %3; }
        QTabWidget::pane { border: 1px solid palette(midlight); border-radius: 22px; background: palette(window); }
        QGroupBox { border: 1px solid palette(midlight); border-radius: 16px; margin-top: 16px; padding: 16px 10px 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }
        QFrame#tooltipChoiceCard { border: 1px solid palette(midlight); border-radius: 16px; background: palette(window); }
        QComboBox { min-height: 28px; padding: 2px 10px; }
        QSpinBox { min-height: 28px; }
        QCheckBox { spacing: 10px; min-height: 28px; }
        QPushButton { min-height: 28px; }
    )").arg(tint(115),tint(65),tint(38),mutedCss));
}
void StyleConfig::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);
    if(event->type()==QEvent::PaletteChange && !_updatingPalette) { _updatingPalette=true;applyHubPalette();_updatingPalette=false; }
}
void StyleConfig::loadTooltips(bool defaults) {
    const bool wasLoading=_loading;_loading=true;
    for(auto it=_tooltipChoices.cbegin();it!=_tooltipChoices.cend();++it) {
        const auto shape=defaults ? SequoiaGlass::tooltipDefault(it.key()) : SequoiaGlass::tooltipShape(it.key());
        it.value()->setCurrentIndex(it.value()->findData(shape));
        if(!defaults) _savedTooltips[it.key()]=shape;
    }
    _loading=wasLoading;
}
void StyleConfig::saveTooltips() {
    QSettings config(SequoiaGlass::tooltipConfigPath(),QSettings::IniFormat);
    for(auto it=_tooltipChoices.cbegin();it!=_tooltipChoices.cend();++it) {
        const auto shape=it.value()->currentData().toString();config.setValue(QStringLiteral("Tooltips/")+it.key(),shape);_savedTooltips[it.key()]=shape;
    }
    config.sync();
}
}
