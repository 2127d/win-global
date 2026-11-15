// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QStyleOption>

class QPainter;
class QWidget;

namespace SequoiaEleven::FilePicker
{
// Deliberately requires a chooser dialog, not merely a KFileWidget or a
// KFilePlacesView: those widgets are also used by ordinary file explorers.
bool contains(const QWidget *widget);
int popupRadius(const QWidget *widget);
void paintNavigation(QPainter *painter, const QStyleOption *option);
QRect breadcrumbHoverRect(const QWidget *widget);
void polish(QWidget *widget);
void unpolish(QWidget *widget);
void paintPill(QPainter *painter, const QStyleOption *option,
               QPalette::ColorRole role = QPalette::Button, bool solid = false);
}
