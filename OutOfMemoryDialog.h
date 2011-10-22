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

#ifndef OUT_OF_MEMORY_DIALOG_H_
#define OUT_OF_MEMORY_DIALOG_H_

#include "ui_OutOfMemoryDialog.h"
#include "OutputFileNameGenerator.h"
#include "IntrusivePtr.h"
#include "StageSequence.h"
#include "ProjectPages.h"
#include "SelectedPage.h"
#include <QString>
#include <QDialog>

class OutOfMemoryDialog : public QDialog
{
	Q_OBJECT
public:
	OutOfMemoryDialog(QWidget* parent = 0);

	void setParams(
		QString const& project_file, // may be empty
		IntrusivePtr<StageSequence> const& stages,
		IntrusivePtr<ProjectPages> const& pages,
		SelectedPage const& selected_page,
		OutputFileNameGenerator const& out_file_name_gen);
private slots:
	void saveProject();

	void saveProjectAs();
private:
	bool saveProjectWithFeedback(QString const& project_file);

	void showSaveSuccessScreen();

	Ui::OutOfMemoryDialog ui;
	QString m_projectFile;
	IntrusivePtr<StageSequence> m_ptrStages;
	IntrusivePtr<ProjectPages> m_ptrPages;
	SelectedPage m_selectedPage;
	OutputFileNameGenerator m_outFileNameGen;
};

#endif
