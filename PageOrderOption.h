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

#ifndef PAGE_ORDER_OPTION_H_
#define PAGE_ORDER_OPTION_H_

#include "IntrusivePtr.h"
#include "PageOrderProvider.h"
#include <QString>

class PageOrderOption
{
	// Member-wise copying is OK.
public:
	typedef IntrusivePtr<PageOrderProvider const> ProviderPtr;

	PageOrderOption(QString const& name, ProviderPtr const& provider)
		: m_name(name), m_ptrProvider(provider) {}

	QString const& name() const { return m_name; }

	/**
	 * Returns the ordering information provider.
	 * A null provider is OK and is to be interpreted as default order.
	 */
	ProviderPtr const& provider() const { return m_ptrProvider; }
private:
	QString m_name;
	ProviderPtr m_ptrProvider;
};

#endif
