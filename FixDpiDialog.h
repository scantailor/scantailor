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

#ifndef FIXDPIDIALOG_H_
#define FIXDPIDIALOG_H_

#include "ui_FixDpiDialog.h"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "Dpi.h"
#include <QDialog>
#include <QString>
#include <QItemSelection>
#include <QPalette>
#include <QSize>
#include <vector>
#include <memory>

class QItemSelection;

class FixDpiDialog : public QDialog, private Ui::FixDpiDialog
{
	Q_OBJECT
public:
	FixDpiDialog(std::vector<ImageFileInfo> const& files, QWidget* parent = 0);
	
	virtual ~FixDpiDialog();
	
	std::vector<ImageFileInfo> const& files() const;
private slots:
	void tabChanged(int tab);
	
	void selectionChanged(QItemSelection const& selection);
	
	void dpiComboChangedByUser(int index);
	
	void dpiValueChanged();
	
	void applyClicked();
private:
	class DpiCounts;
	class SizeGroup;
	class TreeModel;
	class FilterModel;
	
	enum Scope { ALL, NOT_OK };
	
	void enableDisableOkButton();
	
	void updateDpiFromSelection(QItemSelection const& selection);
	
	void resetDpiForm();
	
	void setDpiForm(ImageMetadata const& metadata);
	
	void updateDpiCombo();
	
	void decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpi_status) const;
	
	std::auto_ptr<TreeModel> m_ptrPages;
	std::auto_ptr<FilterModel> m_ptrUndefinedDpiPages;
	QString m_xDpiInitialValue;
	QString m_yDpiInitialValue;
	QSize m_selectedItemPixelSize;
	QPalette m_normalPalette;
	QPalette m_errorPalette;
};

#endif
