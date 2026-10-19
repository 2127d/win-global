#ifndef sequoiaeleven_h
#define sequoiaeleven_h

/*
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "sequoiaelevensettings.h"
#define KDECORATION_VERSION KDECORATION_VERSION_CHECK(KDECORATION_VERSION_MAJOR, KDECORATION_VERSION_MINOR, KDECORATION_VERSION_PATCH)
#define KDECORATION_VERSION_CHECK(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

#include <QList>
#include <QSharedPointer>

namespace SequoiaEleven
{
//* convenience typedefs
using InternalSettingsPtr = QSharedPointer<InternalSettings>;
using InternalSettingsList = QList<InternalSettingsPtr>;
using InternalSettingsListIterator = QListIterator<InternalSettingsPtr>;

//* metrics
namespace Metrics
{
//* corner radius (pixels)
static constexpr qreal Frame_FrameRadius = 5;

//* titlebar metrics, in units of small spacing
// One extra small-spacing unit above the caption and traffic-light controls.
static constexpr int TitleBar_TopMargin = 2;
static constexpr int TitleBar_BottomMargin = 1;
static constexpr int TitleBar_SideMargin = 3;
static constexpr int TitleBar_ButtonSpacing = 2;

// shadow dimensions (pixels)
static constexpr int Shadow_Overlap = 3;
}

//* standard pen widths
namespace PenWidth
{
/* Using 1 instead of ssequoiaeleven more than 1 causes symbols drawn with
 * pen strokes to look skewed. The exact amount added does not matter
 * as long as it isn't too visible.
 */
// The standard pen stroke width for symbols.
static constexpr qreal Symbol = 1.01;
}

//* exception
enum ExceptionMask {
    None = 0,
    BorderSize = 1 << 4
};
}

#endif
