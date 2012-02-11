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

#ifndef PAYLOAD_EVENT_H_
#define PAYLOAD_EVENT_H_

#include <QEvent>

template<typename T>
class PayloadEvent : public QEvent
{
public:
	PayloadEvent(T const& payload) : QEvent(User), m_payload(payload) {}
	
	T const& payload() const { return m_payload; }
	
	T& payload() { return m_payload; } 
private:
	T m_payload;
};

#endif
