// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PineconeMC Offline - Minecraft Launcher
 *  Copyright (C) 2026 PineconeMC Offline Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "GuideWindow.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
const QUrl guideUrl("qrc:/documents/Guide.html");

// Serves Guide.html adapted for Qt's rich text. Loading it by URL (instead of setHtml) keeps the browser's own link
// handling: "#section" links, Back and Forward work as in a web browser.
class GuideBrowser : public QTextBrowser {
   public:
    using QTextBrowser::QTextBrowser;
    QVariant loadResource(int type, const QUrl& name) override
    {
        if (type == QTextDocument::HtmlResource && name.path() == guideUrl.path()) {
            QFile file(":/documents/Guide.html");
            if (file.open(QIODevice::ReadOnly))
                return OfflineGuide::forTextBrowser(QString::fromUtf8(file.readAll()));
        }
        return QTextBrowser::loadResource(type, name);
    }
};

// The page's own CSS is for web browsers (dark mode, flexbox); this one follows the launcher's theme, so dark themes
// and high contrast apply to the guide too.
QColor mix(const QColor& a, const QColor& b, double amount)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * amount, a.greenF() + (b.greenF() - a.greenF()) * amount,
                            a.blueF() + (b.blueF() - a.blueF()) * amount);
}

QString guideStyleSheet(const QPalette& palette)
{
    // Boxes are a tint of the page towards the text colour, so they stay readable in any theme (some themes' alternate
    // background is white even when the page is dark).
    const QColor page = palette.color(QPalette::Base);
    const QColor text = palette.color(QPalette::Text);
    const QString box = mix(page, text, 0.12).name();
    const QString border = mix(page, text, 0.4).name();
    const QString link = palette.color(QPalette::Link).name();
    return QString(
               "body { color: %4; }"
               "a { color: %1; }"
               "h1 { font-size: xx-large; }"
               "h2 { font-size: x-large; margin-top: 24px; }"
               "h3 { font-size: large; margin-top: 16px; }"
               "h4 { font-size: medium; margin-top: 14px; margin-bottom: 4px; }"
               "p.lead { font-size: large; }"
               ".ui { font-weight: 600; }"
               "kbd { font-family: monospace; background-color: %2; }"
               "code { font-family: monospace; }"
               ".tip, .warn { background-color: %2; padding: 8px; margin-top: 8px; margin-bottom: 8px; }"
               "table { border-color: %3; border-style: solid; border-collapse: collapse; margin-top: 8px; margin-bottom: 8px; }"
               "th { background-color: %2; }"
               "th, td { padding: 6px; }"
               "table.flow { border-style: none; }"
               "td.step { background-color: %2; padding: 10px; }"
               "td.arrow { font-size: x-large; padding: 6px; }")
        .arg(link, box, border, text.name());
}

QPointer<GuideWindow> s_window;
}  // namespace

GuideWindow::GuideWindow(QWidget* parent) : QDialog(parent)
{
    setWindowFlag(Qt::Window);
    setWindowFlag(Qt::WindowMinMaxButtonsHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowTitle(tr("PineconeMC Offline guide"));
    setAttribute(Qt::WA_DeleteOnClose);

    auto* back = new QToolButton(this);
    back->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    back->setToolTip(tr("Back (Alt+Left)"));
    back->setAccessibleName(tr("Back"));
    back->setEnabled(false);
    auto* forward = new QToolButton(this);
    forward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    forward->setToolTip(tr("Forward (Alt+Right)"));
    forward->setAccessibleName(tr("Forward"));
    forward->setEnabled(false);
    auto* contents = new QPushButton(tr("&Contents"), this);
    contents->setToolTip(tr("Back to the list of sections at the top"));

    m_find = new QLineEdit(this);
    m_find->setPlaceholderText(tr("Find in the guide"));
    m_find->setAccessibleName(tr("Find in the guide"));
    m_find->setClearButtonEnabled(true);
    auto* findLabel = new QLabel(tr("&Find:"), this);
    findLabel->setBuddy(m_find);
    auto* previous = new QToolButton(this);
    previous->setArrowType(Qt::UpArrow);
    previous->setToolTip(tr("Previous match (Shift+F3)"));
    previous->setAccessibleName(tr("Previous match"));
    auto* next = new QToolButton(this);
    next->setArrowType(Qt::DownArrow);
    next->setToolTip(tr("Next match (F3 or Enter)"));
    next->setAccessibleName(tr("Next match"));

    auto* bar = new QHBoxLayout();
    bar->addWidget(back);
    bar->addWidget(forward);
    bar->addWidget(contents);
    bar->addStretch(1);
    bar->addWidget(findLabel);
    bar->addWidget(m_find, 2);
    bar->addWidget(previous);
    bar->addWidget(next);

    m_browser = new GuideBrowser(this);
    m_browser->setOpenExternalLinks(true);
    m_browser->setAccessibleName(tr("Guide"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(bar);
    layout->addWidget(m_browser, 1);
    layout->addWidget(buttons);

    connect(back, &QToolButton::clicked, m_browser, &QTextBrowser::backward);
    connect(forward, &QToolButton::clicked, m_browser, &QTextBrowser::forward);
    connect(m_browser, &QTextBrowser::backwardAvailable, back, &QToolButton::setEnabled);
    connect(m_browser, &QTextBrowser::forwardAvailable, forward, &QToolButton::setEnabled);
    connect(contents, &QPushButton::clicked, this, [this] { goTo(OfflineGuide::Section::Start); });
    connect(m_find, &QLineEdit::returnPressed, this, [this] { find(false); });
    connect(next, &QToolButton::clicked, this, [this] { find(false); });
    connect(previous, &QToolButton::clicked, this, [this] { find(true); });
    connect(new QShortcut(QKeySequence::Find, this), &QShortcut::activated, this, [this] {
        m_find->setFocus();
        m_find->selectAll();
    });
    connect(new QShortcut(QKeySequence::FindNext, this), &QShortcut::activated, this, [this] { find(false); });
    connect(new QShortcut(QKeySequence::FindPrevious, this), &QShortcut::activated, this, [this] { find(true); });

    loadGuide();
    m_browser->setFocus();

    const QRect screen = (parent ? parent->screen() : QApplication::primaryScreen())->availableGeometry();
    resize(qMin(960, screen.width() * 8 / 10), qMin(800, screen.height() * 8 / 10));
}

void GuideWindow::showSection(OfflineGuide::Section section)
{
    // A modal dialog (an error message, a wizard) blocks input to every other window, so while one is open the guide
    // opens as that dialog's own window; otherwise one guide window is reused.
    GuideWindow* window = nullptr;
    if (QWidget* modal = QApplication::activeModalWidget()) {
        window = new GuideWindow(modal);
    } else {
        if (!s_window)
            s_window = new GuideWindow(nullptr);
        window = s_window;
    }
    window->show();
    window->raise();
    window->activateWindow();
    window->goTo(section);
}

void GuideWindow::loadGuide()
{
    m_browser->document()->setDefaultStyleSheet(guideStyleSheet(palette()));
    if (m_browser->source().isEmpty()) {
        m_browser->setSource(guideUrl);
        return;
    }
    // Restyling reloads the page: stay where the reader was.
    const int position = m_browser->verticalScrollBar()->value();
    m_browser->reload();
    QTimer::singleShot(0, m_browser, [browser = m_browser, position] { browser->verticalScrollBar()->setValue(position); });
}

void GuideWindow::goTo(OfflineGuide::Section section)
{
    QUrl url = guideUrl;
    url.setFragment(OfflineGuide::anchor(section));
    m_browser->setSource(url);
    // Also when the URL didn't change, and after the page has been laid out.
    QTimer::singleShot(0, m_browser, [browser = m_browser, section] { browser->scrollToAnchor(OfflineGuide::anchor(section)); });
}

void GuideWindow::find(bool backwards)
{
    const QString text = m_find->text();
    if (text.isEmpty())
        return;
    const QTextDocument::FindFlags flags = backwards ? QTextDocument::FindBackward : QTextDocument::FindFlags();
    if (m_browser->find(text, flags))
        return;
    // Not found from here on: wrap around once.
    m_browser->moveCursor(backwards ? QTextCursor::End : QTextCursor::Start);
    if (!m_browser->find(text, flags))
        QApplication::beep();
}

void GuideWindow::changeEvent(QEvent* event)
{
    // Theme, high contrast or text size changed while the guide is open: restyle it.
    if (m_browser && (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange))
        loadGuide();
    QDialog::changeEvent(event);
}
