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

#include "FriendlyErrorDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QTextDocumentFragment>
#include <QToolButton>
#include <QVBoxLayout>

#include "ui/widgets/GuideButton.h"

FriendlyErrorDialog::FriendlyErrorDialog(QWidget* parent,
                                         const QString& title,
                                         const QString& message,
                                         const QString& details,
                                         std::optional<OfflineGuide::Section> guide)
    : QDialog(parent)
{
    setWindowTitle(title);
    setAccessibleDescription(QTextDocumentFragment::fromHtml(message).toPlainText());  // screen readers read it when the dialog opens
    auto* layout = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    auto* icon = new QLabel(this);
    icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32));
    icon->setAlignment(Qt::AlignTop);
    auto* text = new QLabel(message, this);
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    top->addWidget(icon);
    top->addWidget(text, 1);
    layout->addLayout(top);

    auto* bottom = new QHBoxLayout();
    if (!details.isEmpty()) {
        auto* toggle = new QToolButton(this);
        toggle->setText(tr("Details"));
        toggle->setCheckable(true);
        toggle->setArrowType(Qt::RightArrow);
        toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        auto* detailsBox = new QPlainTextEdit(details, this);
        detailsBox->setReadOnly(true);
        detailsBox->setVisible(false);
        connect(toggle, &QToolButton::toggled, this, [toggle, detailsBox](bool open) {
            toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
            detailsBox->setVisible(open);
        });
        layout->addWidget(toggle);
        layout->addWidget(detailsBox);

        // A plain button outside the button box, so copying never closes the dialog.
        auto* copy = new QPushButton(tr("Copy details"), this);
        connect(copy, &QPushButton::clicked, this, [message, details] { QApplication::clipboard()->setText(message + "\n\n" + details); });
        bottom->addWidget(copy);
    }
    if (guide) {
        auto* help = new QPushButton(tr("Open guide"), this);
        const auto section = *guide;
        connect(help, &QPushButton::clicked, this, [this, section] { GuideButton::openSection(section, this); });
        bottom->addWidget(help);
    }
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    bottom->addWidget(m_buttons, 1);
    layout->addLayout(bottom);
    setMinimumWidth(460);
}

QPushButton* FriendlyErrorDialog::addActionButton(const QString& text)
{
    auto* button = m_buttons->addButton(text, QDialogButtonBox::ActionRole);
    connect(button, &QPushButton::clicked, this, [this] { done(ActionResult); });
    button->setDefault(true);
    button->setFocus();
    return button;
}

void FriendlyErrorDialog::show(QWidget* parent,
                               const QString& title,
                               const QString& message,
                               const QString& details,
                               std::optional<OfflineGuide::Section> guide)
{
    FriendlyErrorDialog dialog(parent, title, message, details, guide);
    dialog.exec();
}
