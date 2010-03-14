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


#ifndef OUTPUT_DESPECKLE_VIEW_H_
#define OUTPUT_DESPECKLE_VIEW_H_

#include "DespeckleLevel.h"
#include "DespeckleState.h"
#include "IntrusivePtr.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include <QStackedWidget>
#include <QImage>

class DebugImages;
class ProcessingIndicationWidget;

namespace output
{

class DespeckleVisualization;

class DespeckleView : public QStackedWidget
{
	Q_OBJECT
public:
	/**
	 * \param despeckle_state Describes a particular despeckling.
	 * \param visualization Optional despeckle visualization.
	 *        If null, it will be reconstructed from \p despeckle_state
	 *        when this widget becomes visible.
	 * \param debug Indicates whether debugging is turned on.
	 */
	DespeckleView(DespeckleState const& despeckle_state,
		DespeckleVisualization const& visualization, bool debug);

	virtual ~DespeckleView();
public slots:
	void despeckleLevelChanged(DespeckleLevel level, bool* handled);
protected:
	virtual void hideEvent(QHideEvent* evt);

	virtual void showEvent(QShowEvent* evt);
private:
	class TaskCancelException;
	class TaskCancelHandle;
	class DespeckleTask;
	class DespeckleResult;

	enum AnimationAction { RESET_ANIMATION, RESUME_ANIMATION };

	void initiateDespeckling(AnimationAction anim_action);

	void despeckleDone(DespeckleState const& despeckle_state,
		DespeckleVisualization const& visualization, DebugImages* dbg);

	void cancelBackgroundTask();

	void removeImageViewWidget();

	DespeckleState m_despeckleState;
	IntrusivePtr<TaskCancelHandle> m_ptrCancelHandle;
	ProcessingIndicationWidget* m_pProcessingIndicator;
	DespeckleLevel m_despeckleLevel;
	bool m_debug;
};

} // namespace output

#endif
