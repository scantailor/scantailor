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

#ifndef QT_SIGNAL_FORWARDER_H_
#define QT_SIGNAL_FORWARDER_H_

#include "NonCopyable.h"
#include <QObject>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

/**
 * \brief Connects to a Qt signal and forwards it to a boost::function.
 *
 * Useful when you need to bind additional parameters to a slot
 * at connection time.
 */
class QtSignalForwarder : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(QtSignalForwarder)
public:
	/**
	 * \brief Constructor.
	 *
	 * \param emitter The object that will emit a signal.  The forwarder
	 *        will become its child.
	 * \param signal The signal specification in the form of SIGNAL(name()).
	 *        Signals with arguments may be specified, but the arguments
	 *        won't be forwarded.
	 * \param slot A boost::function to forward the signal to.
	 */
	QtSignalForwarder(
		QObject* emitter, char const* signal, boost::function<void()> const& slot);
private slots:
	void handleSignal();
private:
	boost::function<void()> m_slot;
};

#endif
