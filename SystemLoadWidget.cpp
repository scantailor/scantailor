/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "SystemLoadWidget.h"
#include "SystemLoadWidget.h.moc"
#include "ThreadPriority.h"
#include <QToolTip>

SystemLoadWidget::SystemLoadWidget(QWidget* parent)
:	QWidget(parent)
{
	ui.setupUi(this);

	ThreadPriority const prio(ThreadPriority::load("settings/batch_processing_priority"));
	ui.slider->setRange(ThreadPriority::Minimum, ThreadPriority::Maximum);
	ui.slider->setValue(prio.value());

	connect(ui.slider, SIGNAL(sliderPressed()), SLOT(sliderPressed()));
	connect(ui.slider, SIGNAL(sliderMoved(int)), SLOT(sliderMoved(int)));
	connect(ui.slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
	connect(ui.minusBtn, SIGNAL(clicked()), SLOT(decreasePriority()));
	connect(ui.plusBtn, SIGNAL(clicked()), SLOT(increasePriority()));
}

void
SystemLoadWidget::sliderPressed()
{
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::sliderMoved(int prio)
{
	showHideToolTip(prio);
}

void
SystemLoadWidget::valueChanged(int prio)
{
	ThreadPriority((ThreadPriority::Priority)prio).save("settings/batch_processing_priority");
}

void
SystemLoadWidget::decreasePriority()
{
	ui.slider->setValue(ui.slider->value() - 1);
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::increasePriority()
{
	ui.slider->setValue(ui.slider->value() + 1);
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::showHideToolTip(int prio)
{
	QString const tooltip_text(tooltipText(prio));
	if (tooltip_text.isEmpty()) {
		QToolTip::hideText();
		return;
	}
	
	// Show the tooltip immediately.
	QPoint const center(ui.slider->rect().center());
	QPoint tooltip_pos(ui.slider->mapFromGlobal(QCursor::pos()));
	if (tooltip_pos.x() < 0 || tooltip_pos.x() >= ui.slider->width()) {
		tooltip_pos.setX(center.x());
	}
	if (tooltip_pos.y() < 0 || tooltip_pos.y() >= ui.slider->height()) {
		tooltip_pos.setY(center.y());
	}
	tooltip_pos = ui.slider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, ui.slider);
}

QString
SystemLoadWidget::tooltipText(int prio)
{
	if (prio == ThreadPriority::Minimum) {
		return tr("Minimal");
	} else if (prio == ThreadPriority::Normal) {
		return tr("Normal");
	} else {
		return QString();
	}
}
