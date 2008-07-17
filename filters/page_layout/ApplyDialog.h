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

#ifndef PAGE_LAYOUT_APPLYDIALOG_H_
#define PAGE_LAYOUT_APPLYDIALOG_H_

#include "ui_PageLayoutApplyDialog.h"
#include "Scope.h"
#include <QDialog>

namespace page_layout
{

class ApplyDialog : public QDialog, private Ui::PageLayoutApplyDialog
{
	Q_OBJECT
public:
	ApplyDialog(QWidget* parent = 0);
	
	virtual ~ApplyDialog();
signals:
	void accepted(Scope scope);
private slots:
	void thisPageSelected();
	
	void allPagesSelected();
	
	void onSubmit();
private:
	Scope m_scope;
};

} // namespace page_layout

#endif
