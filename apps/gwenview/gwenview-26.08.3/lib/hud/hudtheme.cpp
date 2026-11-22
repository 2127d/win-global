// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include <hud/hudtheme.h>

#include <QApplication>
#include <lib/sequoiadesign.h>

namespace Gwenview::HudTheme {
RenderInfo renderInfo(WidgetType widget, State state)
{
    const QPalette palette = QApplication::palette();
    const QColor ink = palette.color(QPalette::WindowText);
    RenderInfo info;
    info.borderPen = QPen(Sequoia::border(palette), 1);
    info.textPen = QPen(ink);
    info.bgBrush = Sequoia::tint(palette.color(QPalette::Window), ink,
                               state == DownState ? 0.14 : state == MouseOverState ? 0.08 : 0);
    switch (widget) {
    case ButtonWidget: info.borderRadius = Sequoia::ControlRadius; info.padding = 6; break;
    case FrameWidget: info.borderRadius = Sequoia::SurfaceRadius; break;
    case CountDown: info.bgBrush = palette.color(QPalette::Highlight); break;
    case SliderWidgetHandle: info.borderRadius = 7; info.padding = 6; info.bgBrush = palette.color(QPalette::Base); break;
    case SliderWidgetGroove: info.borderRadius = 3; info.padding = 6; info.bgBrush = Sequoia::alpha(ink, 0.20); break;
    }
    return info;
}
}
