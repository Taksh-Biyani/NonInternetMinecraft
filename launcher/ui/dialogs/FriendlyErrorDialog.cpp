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
#include <QToolButton>
#include <QVBoxLayout>

FriendlyErrorDialog::FriendlyErrorDialog(QWidget* parent, const QString& title, const QString& message, const QString& details)
    : QDialog(parent)
{
    setWindowTitle(title);
    auto* layout = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    auto* icon = new QLabel(this);
    icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32));
    icon->setAlignment(Qt::AlignTop);
    auto* text = new QLabel(message, this);
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse);
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
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    bottom->addWidget(buttons, 1);
    layout->addLayout(bottom);
    setMinimumWidth(460);
}

void FriendlyErrorDialog::show(QWidget* parent, const QString& title, const QString& message, const QString& details)
{
    FriendlyErrorDialog dialog(parent, title, message, details);
    dialog.exec();
}
