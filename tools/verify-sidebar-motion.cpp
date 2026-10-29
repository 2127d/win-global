#include "../apps/gwenview/gwenview-26.08.1/app/sidebarmotion.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QTest>
#include <QProxyStyle>
#include <QStyleFactory>
#include <cstdio>
#include <cstdlib>
using namespace Gwenview;
void require(bool ok, const char *message) { if (!ok) { fprintf(stderr, "FAIL: %s\n", message); exit(1); } }
class NoMotion : public QProxyStyle {
public:
    NoMotion() : QProxyStyle(QStyleFactory::create("Fusion")) {}
    int styleHint(StyleHint h, const QStyleOption *o, const QWidget *w, QStyleHintReturn *r) const override {
        return h == SH_Widget_Animation_Duration ? 0 : QProxyStyle::styleHint(h, o, w, r);
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QSplitter splitter; splitter.resize(900, 600);
    QWidget *sidebar = new QWidget(&splitter); sidebar->setMinimumWidth(180);
    auto layout = new QVBoxLayout(sidebar); layout->addWidget(new QLabel("Recent images", sidebar));
    layout->addStretch(); layout->addWidget(new QLabel("Recent     Information     Tools", sidebar));
    QWidget *content = new QWidget(&splitter);
    auto row = new QVBoxLayout(content); auto toggle = new QToolButton(content); row->addWidget(toggle); row->addStretch();
    QAction action("Sidebar", &splitter); action.setCheckable(true); action.setChecked(true);
    splitter.setChildrenCollapsible(false); splitter.setStretchFactor(0,0); splitter.setStretchFactor(1,1); splitter.show(); splitter.setSizes({280, 620}); QTest::qWait(50);
    SidebarMotion motion(&splitter, sidebar, &action, {toggle});
    auto animation = splitter.findChild<QVariantAnimation *>("sidebarVisibilityAnimation");
    const int width = sidebar->width(), min = sidebar->minimumWidth(), handle = splitter.handleWidth();
    const int childWidth = sidebar->findChild<QLabel *>()->width();
    const auto expandedArrow = toggle->icon().pixmap(24,24).toImage();
    const auto expandedButton = toggle->grab().toImage();
    motion.setVisible(false, true); QTest::qWait(45);
    require(motion.isRunning() && sidebar->width() > 0 && sidebar->width() < width, "hiding passes through intermediate sidebar widths");
    require(animation->duration() == 180, "sidebar slide stays short");
    require(sidebar->findChild<QLabel *>()->width() == childWidth, "live sidebar contents keep their full width during motion");
    require(toggle->icon().pixmap(24,24).toImage() != expandedArrow, "shared arrow visibly morphs during slide");
    require(toggle->grab().toImage() != expandedButton, "native button repaints the changing arrow");
    splitter.grab().save(".staging/sidebar-hiding.png");
    const int intermediate = sidebar->width();
    motion.setVisible(true, true);
    require(sidebar->width() == intermediate, "reversal does not jump");
    QTest::qWait(230);
    require(!motion.isRunning() && sidebar->isVisible() && sidebar->width() == width, "reversal restores exact sidebar width");
    for (int i=0; i<4; ++i) {
        motion.setVisible(false, true); QTest::qWait(230);
        require(!sidebar->isVisible() && !motion.isRunning(), "hide completes");
        require(sidebar->minimumWidth() == min && splitter.handleWidth() == handle && !splitter.isCollapsible(0), "native layout constraints restored");
        motion.setVisible(true, true); QTest::qWait(40);
        require(motion.isRunning() && sidebar->width() < width, "show animates");
        QTest::qWait(200);
        require(sidebar->width() == width && sidebar->isVisible(), "repeated toggles preserve width");
    }
    splitter.grab().save(".staging/sidebar-expanded.png");
    motion.setVisible(false, true); QTest::qWait(40); splitter.resize(800,500); QTest::qWait(30);
    require(!motion.isRunning() && !sidebar->isVisible(), "resize finishes transition cleanly");
    motion.setVisible(true, false);
    require(!motion.isRunning() && sidebar->isVisible(), "programmatic mode changes stay immediate");
    NoMotion noMotion; splitter.setStyle(&noMotion);
    motion.setVisible(false, true);
    require(!motion.isRunning() && !sidebar->isVisible(), "reduced motion switches immediately");
    motion.setVisible(true, true); splitter.setStyle(nullptr);
    auto left = toggle->icon().pixmap(24,24).toImage();
    splitter.setLayoutDirection(Qt::RightToLeft); QTest::qWait(20);
    require(toggle->icon().pixmap(24,24).toImage() != left, "arrow follows right-to-left layout");
    motion.setVisible(false, true); QTest::qWait(30); motion.setVisible(true, true); QTest::qWait(220);
    require(sidebar->width() == width && !motion.isRunning(), "right-to-left rapid toggle restores width");
    splitter.setSizes({320, splitter.width() - 320 - splitter.handleWidth()});
    const int manualWidth = sidebar->width();
    motion.setVisible(true, false);
    require(sidebar->width() == manualWidth, "mode changes preserve a manually resized sidebar");
    motion.setVisible(false, false); motion.setVisible(true, false);
    require(sidebar->width() == manualWidth, "programmatic hide/show preserves manual width");
    fprintf(stderr,"PASS: coordinated arrow/slide, intermediate widths, reversal, repeated toggles, sizing, reduced motion, RTL\n");
}
