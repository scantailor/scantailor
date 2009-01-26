/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NewOpenProjectPanel.h.moc"
#include "RecentProjects.h"
#include "Utils.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QVBoxLayout>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QColor>
#include <QRect>
#include <QPoint>
#include <QRegion>
#include <QSettings>
#include <QString>
#include <QFileInfo>
#include <QLabel>
#include <Qt>
#include <QDebug>

NewOpenProjectPanel::NewOpenProjectPanel(QWidget* parent)
:	QWidget(parent)
{
	setupUi(this);
	recentProjectsGroup->setLayout(new QVBoxLayout);
	newProjectLabel->setText(
		Utils::richTextForLink(newProjectLabel->text())
	);
	openProjectLabel->setText(
		Utils::richTextForLink(openProjectLabel->text())
	);
	
	int left = 0, top = 0, right = 0, bottom = 0;
	layout()->getContentsMargins(&left, &top, &right, &bottom);
	left += MARGIN;
	top += MARGIN;
	right += MARGIN;
	bottom += MARGIN;
	layout()->setContentsMargins(left, top, right, bottom);
	
	RecentProjects rp;
	rp.read();
	if (!rp.validate()) {
		// Some project files weren't found.
		// Write the list without them.
		rp.write();
	}
	if (rp.isEmpty()) {
		recentProjectsGroup->setVisible(false);
	} else {
		rp.enumerate(
			boost::lambda::bind(
				&NewOpenProjectPanel::addRecentProject,
				this, boost::lambda::_1
			)
		);
	}
	
	connect(
		newProjectLabel, SIGNAL(linkActivated(QString const&)),
		this, SIGNAL(newProject())
	);
	connect(
		openProjectLabel, SIGNAL(linkActivated(QString const&)),
		this, SIGNAL(openProject())
	);
}

void
NewOpenProjectPanel::addRecentProject(QString const& file_path)
{
	QFileInfo const file_info(file_path);
	QString const base_name(file_info.baseName());
	QLabel* label = new QLabel(recentProjectsGroup);
	label->setWordWrap(true);
	label->setTextFormat(Qt::RichText);
	label->setText(Utils::richTextForLink(base_name, file_path));
	label->setToolTip(file_path);
	recentProjectsGroup->layout()->addWidget(label);
	
	connect(
		label, SIGNAL(linkActivated(QString const&)),
		this, SIGNAL(openRecentProject(QString const&))
	);
}

void
NewOpenProjectPanel::paintEvent(QPaintEvent* event)
{
	QRect const widget_rect(rect());
	
	if (m_lastPaintSize == widget_rect.size()) {
		setMask(m_mask);
	} else {
		QRegion reg(
			widget_rect.adjusted(
				MARGIN, MARGIN, -MARGIN, -MARGIN
			)
		);
		setMask(reg);
		m_mask = reg;
	}
	
	QWidget::paintEvent(event);
	
	clearMask();
	
	int const border = 1;
	int const shadow = MARGIN - border;
	
	QPainter painter(this);
	
	// Draw the border.
	{
		painter.setPen(QPen(palette().windowText(), border));
		QRect rect(widget_rect);
		rect.adjust(MARGIN, MARGIN, -MARGIN, -MARGIN);
		// Note that we specify the coordinates excluding
		// pen width.
		painter.drawRect(rect);
	}
	
	QColor const dark(Qt::darkGray);
	QColor const light(Qt::transparent);
	
	// Right shadow.
	{
		QRect rect(widget_rect);
		rect.setWidth(shadow);
		rect.moveRight(widget_rect.right());
		rect.adjust(0, MARGIN + shadow, 0, -shadow);
		QLinearGradient grad(rect.topLeft(), rect.topRight());
		grad.setColorAt(0, dark);
		grad.setColorAt(1, light);
		painter.fillRect(rect, grad);
	}
	
	// Down shadow.
	{
		QRect rect(widget_rect);
		rect.setHeight(shadow);
		rect.moveBottom(widget_rect.bottom());
		rect.adjust(MARGIN + shadow, 0, -shadow, 0);
		QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
		grad.setColorAt(0, dark);
		grad.setColorAt(1, light);
		painter.fillRect(rect, grad);
	}
	
	// Bottom-right corner.
	{
		QRect rect(0, 0, shadow, shadow);
		rect.moveBottomRight(widget_rect.bottomRight());
		QRadialGradient grad(rect.topLeft(), shadow);
		grad.setColorAt(0, dark);
		grad.setColorAt(1, light);
		painter.fillRect(rect, grad);
	}
	
	// Top-right corner.
	{
		QRect rect(0, 0, shadow, shadow);
		rect.moveTopRight(QPoint(widget_rect.right(), MARGIN));
		QRadialGradient grad(rect.bottomLeft(), shadow);
		grad.setColorAt(0, dark);
		grad.setColorAt(1, light);
		painter.fillRect(rect, grad);
	}
	
	// Bottom-left corner.
	{
		QRect rect(0, 0, shadow, shadow);
		rect.moveBottomLeft(QPoint(MARGIN, widget_rect.bottom()));
		QRadialGradient grad(rect.topRight(), shadow);
		grad.setColorAt(0, dark);
		grad.setColorAt(1, light);
		painter.fillRect(rect, grad);
	}
}
