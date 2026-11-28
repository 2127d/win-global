
/*************************************************************************
 * Copyright (C) 2014 by Hugo Pereira Da Costa <hugo.pereira@free.fr>    *
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the                         *
 * Free Software Foundation, Inc.,                                       *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 *************************************************************************/

#include "sequoiaelevenstyleplugin.h"
#include "sequoiaelevenstyle.h"
#include "sequoiaelevenquickstyle.h"

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QQuickStyle>
#endif

namespace SequoiaEleven
{
#ifdef SEQUOIA_GLASS_TRIAL
static constexpr auto styleName = "SequoiaElevenGlass";
#else
static constexpr auto styleName = "SequoiaEleven";
#endif

//_________________________________________________
QStyle *StylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String(styleName), Qt::CaseInsensitive) == 0) {
        initializeQuickStyle();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Standard KDE Quick controls already delegate their visual primitives
        // to QStyle. Enable that bridge even without a per-application makeover
        // or a validated Quick-islands overlay. Respect explicit other styles.
        const auto quickStyle = QQuickStyle::name();
        if (quickStyle.isEmpty() || quickStyle.compare(QStringLiteral("SequoiaEleven"), Qt::CaseInsensitive) == 0
            || quickStyle.compare(QStringLiteral("SequoiaElevenGlass"), Qt::CaseInsensitive) == 0) {
            QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
        }
#endif
        return new Style;
    }
    return nullptr;
}

//_________________________________________________
QStringList StylePlugin::keys() const
{
    return QStringList(QString::fromLatin1(styleName));
}

}
