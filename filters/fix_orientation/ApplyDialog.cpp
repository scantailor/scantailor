/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ApplyDialog.h.moc"
#include "Scope.h"
#include <QFontMetrics>
#include <QIntValidator>
#include <QMessageBox>
#include <assert.h>

namespace fix_orientation
{

ApplyDialog::ApplyDialog(
	QWidget* parent, int num_pages, int cur_page)
:	QDialog(parent),
	m_numPages(num_pages),
	m_curPage(cur_page)
{
	setupUi(this);
	
	QFontMetrics fm(rangeFrom->fontMetrics());
	int const width = fm.width('0') * 5;
	rangeFrom->setMaximumWidth(width);
	rangeTo->setMaximumWidth(width);
	rangeFrom->setMaxLength(4);
	rangeTo->setMaxLength(4);
	QIntValidator* fromValidator = new QIntValidator(rangeFrom);
	rangeFrom->setValidator(fromValidator);
	QIntValidator* toValidator = new QIntValidator(rangeTo);
	rangeTo->setValidator(toValidator);
	thisPageSelected();
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
	connect(thisPage, SIGNAL(pressed()), this, SLOT(thisPageSelected()));
	connect(everyPage, SIGNAL(pressed()), this, SLOT(everyPageSelected()));
	connect(everyOtherPage, SIGNAL(pressed()), this, SLOT(everyOtherPageSelected()));
}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::thisPageSelected()
{
	rangeFrom->setEnabled(false);
	rangeTo->setEnabled(false);
	
	QString const cur_page_str(QString::number(m_curPage + 1));
	rangeFrom->setText(cur_page_str);
	rangeTo->setText(cur_page_str);
}

void
ApplyDialog::everyPageSelected()
{
	rangeFrom->setEnabled(true);
	rangeTo->setEnabled(true);
	
	QString const cur_page_str(QString::number(m_curPage + 1));
	rangeFrom->setText("1");
	rangeTo->setText(QString::number(m_numPages));
}

void
ApplyDialog::everyOtherPageSelected()
{
	rangeFrom->setEnabled(true);
	rangeTo->setEnabled(true);
	
	QString const cur_page_str(QString::number(m_curPage + 1));
	rangeFrom->setText("1");
	rangeTo->setText(QString::number(m_numPages));
}

void
ApplyDialog::onSubmit()
{
	if (rangeFrom->text().isEmpty() || rangeTo->text().isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Range is required.")
		);
		return;
	}
	
	int const from = rangeFrom->text().toInt() - 1;
	int const to = rangeTo->text().toInt();
	int const step = everyOtherPage->isChecked() ? 2 : 1;
	
	if (everyPage->isChecked() || everyOtherPage->isChecked()) {	
		if (m_curPage < from || m_curPage > to) {
			QMessageBox::warning(
				this, tr("Error"),
				tr("Page %1 (the current page) must be inside the range.")
				.arg(m_curPage + 1)
			);
			return;
		}
	} else {
		assert(thisPage->isChecked());
	}
	
	emit accepted(Scope(from, to, m_curPage, step));
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace fix_orientation
