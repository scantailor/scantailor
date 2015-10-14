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

#ifndef INTERACTION_STATE_H_
#define INTERACTION_STATE_H_

#include "NonCopyable.h"
#include "Proximity.h"
#ifndef Q_MOC_RUN
#include <boost/intrusive/list.hpp>
#endif
#include <QCursor>
#include <QString>

class Proximity;

class InteractionState
{
	DECLARE_NON_COPYABLE(InteractionState)
public:
	class Captor :
		public boost::intrusive::list_base_hook<
			boost::intrusive::link_mode<boost::intrusive::auto_unlink>
		>
	{
		friend class InteractionState;
	private:
		struct CopyHelper
		{
			Captor* captor;

			CopyHelper(Captor* cap) : captor(cap) {}
		};
	public:
		Captor() {}

		Captor(Captor& other) { swap_nodes(other); }

		Captor(CopyHelper other) { swap_nodes(*other.captor); }

		Captor& operator=(Captor& other);

		Captor& operator=(CopyHelper other);

		operator CopyHelper() { return CopyHelper(this); }

		void release() { unlink(); }

		QCursor const& proximityCursor() const { return m_proximityCursor; }

		void setProximityCursor(QCursor const& cursor) { m_proximityCursor = cursor; }

		QCursor const& interactionCursor() const { return m_interactionCursor; }

		void setInteractionCursor(QCursor const& cursor) { m_interactionCursor = cursor; }

		QString const& proximityStatusTip() const { return m_proximityStatusTip; }

		void setProximityStatusTip(QString const& tip) { m_proximityStatusTip = tip; }

		QString const& interactionStatusTip() const { return m_interactionStatusTip; }

		void setInteractionStatusTip(QString const& tip) { m_interactionStatusTip = tip; }

		QString const& interactionOrProximityStatusTip() const {
			return m_interactionStatusTip.isNull() ? m_proximityStatusTip : m_interactionStatusTip;
		}
	private:
		QCursor m_proximityCursor;
		QCursor m_interactionCursor;
		QString m_proximityStatusTip;
		QString m_interactionStatusTip;
	};

	InteractionState();

	void capture(Captor& captor);

	bool captured() const { return !m_captorList.empty(); }

	bool capturedBy(Captor const& captor) const;

	void resetProximity();

	void updateProximity(
		Captor& captor, Proximity const& proximity,
		int priority = 0, Proximity proximity_threshold = Proximity());

	bool proximityLeader(Captor const& captor) const;

	Proximity const& proximityThreshold() const { return m_proximityThreshold; }

	QCursor cursor() const;

	QString statusTip() const;

	QString const& defaultStatusTip() const { return m_defaultStatusTip; }

	void setDefaultStatusTip(QString const& status_tip) { m_defaultStatusTip = status_tip; }

	bool redrawRequested() const { return m_redrawRequested; }

	void setRedrawRequested(bool requested) { m_redrawRequested = requested; }
private:
	typedef boost::intrusive::list<
		Captor, boost::intrusive::constant_time_size<false>
	> CaptorList;

	/**
	 * Returns true if the provided proximity is better than the stored one.
	 */
	bool betterProximity(Proximity const& proximity, int priority) const;

	QString m_defaultStatusTip;
	CaptorList m_captorList;
	CaptorList m_proximityLeader;
	Proximity m_bestProximity;
	Proximity m_proximityThreshold;
	int m_bestProximityPriority;
	bool m_redrawRequested;
};

#endif
