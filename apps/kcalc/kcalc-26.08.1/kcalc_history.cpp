/*
    SPDX-FileCopyrightText: 2021 Antonio Prcela <antonio.prcela@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcalc_history.h"
#include "kcalc_settings.h"
#include <QTextBlockFormat>

//------------------------------------------------------------------------------
// Name: KCalcHistory
// Desc: constructor
//------------------------------------------------------------------------------
KCalcHistory::KCalcHistory(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
    setLayoutDirection(Qt::LeftToRight);
    setAlignment(Qt::AlignLeft);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::LeftEdge}));
}

//------------------------------------------------------------------------------
// Name: KCalcHistory
// Desc: destructor
//------------------------------------------------------------------------------
KCalcHistory::~KCalcHistory() = default;

//------------------------------------------------------------------------------
// Name: addToHistory
// Desc: Adds the latest calculations to  history window
//------------------------------------------------------------------------------
void KCalcHistory::addToHistory(const QString &str, bool setNewLine)
{
    // Each calculation gets its own paragraph, including when it wraps. Always
    // append fragments to the newest entry regardless of the user's selection.
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::Start);
    if (m_addNewLine) {
        cursor.insertBlock();
        cursor.movePosition(QTextCursor::Start);
    }
    QTextBlockFormat format;
    format.setAlignment(Qt::AlignLeft);
    format.setLayoutDirection(Qt::LeftToRight);
    format.setBottomMargin(12);
    cursor.setBlockFormat(format);
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertHtml(str);
    m_addNewLine = setNewLine;
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
    ensureCursorVisible();
}

//------------------------------------------------------------------------------
// Name: addResultToHistory
// Desc: Used mostly for functions that are not in CalcEngine::Operation
//       adds "=" and the result with newline endings
//------------------------------------------------------------------------------
void KCalcHistory::addResultToHistory(const QString &displayContent)
{
    addToHistory(/*QStringLiteral("&nbsp;=&nbsp;") +*/ displayContent, true);
}

//------------------------------------------------------------------------------
// Name: addFuncToHistory
// Desc: Adds the current function symbol the history window
//------------------------------------------------------------------------------
void KCalcHistory::addFuncToHistory(const QString &func)
{
    QString textToHistory = QStringLiteral("&nbsp;") + func + QStringLiteral("&nbsp;");
    addToHistory(textToHistory, false);
}

//------------------------------------------------------------------------------
// Name: clearHistory
// Desc: Clears the content of the history window
//------------------------------------------------------------------------------
void KCalcHistory::clearHistory()
{
    clear();
    m_addNewLine = false;
}

//------------------------------------------------------------------------------
// Name: changeSettings
// Desc:
//------------------------------------------------------------------------------
void KCalcHistory::changeSettings()
{
    KColorScheme schemeView(QPalette::Active, KColorScheme::View);
    QPalette pal = palette();

    pal.setColor(QPalette::Text, KCalcSettings::followSystemTheme() ? schemeView.foreground().color() : KCalcSettings::foreColor());
    pal.setColor(QPalette::Base, KCalcSettings::followSystemTheme() ? schemeView.background().color() : KCalcSettings::backColor());

    setPalette(pal);

    setFont(KCalcSettings::historyFont());
}

//------------------------------------------------------------------------------
// Name: setFont
// Desc: Apply the configured history font
//------------------------------------------------------------------------------
void KCalcHistory::setFont(const QFont &font)
{
    // Overwrite current baseFont
    m_baseFont = font;
    updateFont();
}

//------------------------------------------------------------------------------
// Name: updateFont
// Desc: Update font using baseFont to better fit
//------------------------------------------------------------------------------
void KCalcHistory::updateFont()
{
    QTextEdit::setFont(baseFont());
}

//------------------------------------------------------------------------------
// Name: baseFont
// Desc: Returns current baseFont
//------------------------------------------------------------------------------
const QFont &KCalcHistory::baseFont() const
{
    return m_baseFont;
}

//------------------------------------------------------------------------------
// Name: resizeEvent
// Desc: resize history while keeping text at the configured size
//------------------------------------------------------------------------------
void KCalcHistory::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);

    updateGeometry();
}

#include "moc_kcalc_history.cpp"
