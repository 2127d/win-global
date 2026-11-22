/*
    SPDX-FileCopyrightText: 2023 Gabriel Barrantes <gabriel.barrantes.dev@outlook.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcalc_display_frame.h"
#include <QPainter>

//------------------------------------------------------------------------------
// Name: KCalcDisplayFrame
// Desc: constructor
//------------------------------------------------------------------------------
KCalcDisplayFrame::KCalcDisplayFrame(QWidget *parent)
    : QFrame(parent)
{
    setAutoFillBackground(false);
    setBackgroundRole(QPalette::Base);
    setFrameShape(QFrame::NoFrame);
}

void KCalcDisplayFrame::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().brush(QPalette::Button));
    painter.drawRoundedRect(rect(), 16, 16);
}

#include "moc_kcalc_display_frame.cpp"
