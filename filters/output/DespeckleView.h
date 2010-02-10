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
#include "IntrusivePtr.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include <QStackedWidget>

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
	 * \param pre_despeckle_img The image before despeckling.
	 *        This one is mandatory, as it will be used for
	 *        re-despeckling with different parameters.
	 * \param speckles_img Speckles detected by the de-speckler.
	 *        This one may be null.  It will only be used if
	 *        \p visualization is null as well, to speed up
	 *        its reconstruction.
	 * \param dpi Dots-per-image of \p pre_despeckle_img.
	 * \param visualization Despeckle visualization.  May be null,
	 *        in which case its reconstruction will be initiated when
	 *        this widget becomes visible.  Passing a null visualization
	 *        is currently used if Despeckling tab is not the currently
	 *        selected one.
	 * \param despeckle_level Is necessary to reconstruct the despeckling.
	 * \param debug Indicates whether debugging is turned on.
	 */
	DespeckleView(imageproc::BinaryImage const& pre_despeckle_img,
		imageproc::BinaryImage const& speckles_img,
		Dpi const& dpi, DespeckleVisualization const& visualization,
		DespeckleLevel despeckle_level, bool debug);

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

	void despeckleDone(DespeckleVisualization const& visualization, DebugImages* dbg);

	void cancelBackgroundTask();

	void removeImageViewWidget();

	imageproc::BinaryImage m_preDespeckleImage;
	imageproc::BinaryImage m_initialSpeckles;
	IntrusivePtr<TaskCancelHandle> m_ptrCancelHandle;
	ProcessingIndicationWidget* m_pProcessingIndicator;
	Dpi m_dpi;
	DespeckleLevel m_despeckleLevel;
	bool m_debug;
};

} // namespace output

#endif
