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

#ifndef STAGELISTVIEW_H_
#define STAGELISTVIEW_H_

#include "IntrusivePtr.h"
#include <QTableView>
#include <QPointer>
#include <QPixmap>
#include <vector>

class StageSequence;

class StageListView : public QTableView
{
	Q_OBJECT
public:
	StageListView(QWidget* parent);
	
	virtual ~StageListView();
	
	void setStages(IntrusivePtr<StageSequence> const& stages);
	
	virtual QSize sizeHint() const { return m_sizeHint; }
signals:
	void launchBatchProcessing();
public slots:
	void setBatchProcessingInProgress(bool in_progress);
protected slots:
	virtual void selectionChanged(
		QItemSelection const& selected,
		QItemSelection const& deselected);
private slots:
	void placeLaunchButton();
	
	void ensureSelectedRowVisible();
protected:
	virtual void timerEvent(QTimerEvent* event);
private:
	class Model;
	class RightColDelegate;
	
	void initiateBatchAnimationFrameRendering();
	
	void createBatchAnimationSequence(int square_side);
	
	QSize m_sizeHint;
	Model* m_pModel;
	QPointer<QWidget> m_ptrLaunchBtn;
	std::vector<QPixmap> m_batchAnimationPixmaps;
	int m_curBatchAnimationFrame;
	int m_timerId;
	bool m_batchProcessingInProgress;
};

#endif
