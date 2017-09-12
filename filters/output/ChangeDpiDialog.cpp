/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ChangeDpiDialog.h"
#include "ChangeDpiDialog.h.moc"
#include "PageSelectionAccessor.h"
#include "Dpi.h"
#include <QButtonGroup>
#include <QVariant>
#include <QIntValidator>
#include <QMessageBox>
#include <QLineEdit>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <algorithm>

namespace output
{

ChangeDpiDialog::ChangeDpiDialog(
	QWidget* parent, Dpi const& dpi, PageId const& cur_page,
	PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
	m_pages(page_selection_accessor.allPages()),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_curPage(cur_page),
	m_pScopeGroup(new QButtonGroup(this))
{
	setupUi(this);
	m_pScopeGroup->addButton(thisPageRB);
	m_pScopeGroup->addButton(allPagesRB);
	m_pScopeGroup->addButton(thisPageAndFollowersRB);
	m_pScopeGroup->addButton(selectedPagesRB);
	if (m_selectedPages.size() <= 1) {
		selectedPagesWidget->setEnabled(false);
	}
	
	dpiSelector->setValidator(new QIntValidator(dpiSelector));
	
	static int const common_dpis[] = {
		300, 400, 600
	};
	
	int const requested_dpi = std::max(dpi.horizontal(), dpi.vertical());
	m_customDpiString = QString::number(requested_dpi);
	
	int selected_index = -1;
	BOOST_FOREACH(int const cdpi, common_dpis) {
		if (cdpi == requested_dpi) {
			selected_index = dpiSelector->count();
		}
		QString const cdpi_str(QString::number(cdpi));
		dpiSelector->addItem(cdpi_str, cdpi_str);
	}
	
	m_customItemIdx = dpiSelector->count();
	dpiSelector->addItem(tr("Custom"), m_customDpiString);
	
	if (selected_index != -1) {
		dpiSelector->setCurrentIndex(selected_index);
	} else {
		dpiSelector->setCurrentIndex(m_customItemIdx);
		dpiSelector->setEditable(true);
		dpiSelector->lineEdit()->setText(m_customDpiString);
		// It looks like we need to set a new validator
		// every time we make the combo box editable.
		dpiSelector->setValidator(
			new QIntValidator(0, 9999, dpiSelector)
		);
	}
	
	connect(
		dpiSelector, SIGNAL(activated(int)),
		this, SLOT(dpiSelectionChanged(int))
	);
	connect(
		dpiSelector, SIGNAL(editTextChanged(QString const&)),
		this, SLOT(dpiEditTextChanged(QString const&))
	);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDpiDialog::~ChangeDpiDialog()
{
}

void
ChangeDpiDialog::dpiSelectionChanged(int const index)
{
	dpiSelector->setEditable(index == m_customItemIdx);
	if (index == m_customItemIdx) {
		dpiSelector->setEditText(m_customDpiString);
		dpiSelector->lineEdit()->selectAll();
		// It looks like we need to set a new validator
		// every time we make the combo box editable.
		dpiSelector->setValidator(
			new QIntValidator(0, 9999, dpiSelector)
		);
	}
}

void
ChangeDpiDialog::dpiEditTextChanged(QString const& text)
{
	if (dpiSelector->currentIndex() == m_customItemIdx) {
		m_customDpiString = text;
	}
}

void
ChangeDpiDialog::onSubmit()
{
	QString const dpi_str(dpiSelector->currentText());
	if (dpi_str.isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is not set.")
		);
		return;
	}
	
	int const dpi = dpi_str.toInt();
	if (dpi < 72) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too low!")
		);
		return;
	}
	
	if (dpi > 1200) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too high!")
		);
		return;
	}
	
	std::set<PageId> pages;
	
	if (thisPageRB->isChecked()) {
		pages.insert(m_curPage);
	} else if (allPagesRB->isChecked()) {
		m_pages.selectAll().swap(pages);
	} else if (thisPageAndFollowersRB->isChecked()) {
		m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
	} else if (selectedPagesRB->isChecked()) {
		emit accepted(m_selectedPages, Dpi(dpi, dpi));
		accept();
		return;
	}
	
	emit accepted(pages, Dpi(dpi, dpi));
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
