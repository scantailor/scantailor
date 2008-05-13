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

#ifndef UTILS_H_
#define UTILS_H_

#include <QString>

class QPixmap;

class Utils
{
public:
	static bool loadAndCachePixmap(QPixmap& pixmap, QString const& file);
	
	template<typename M, typename K, typename V>
	static void mapSetValue(M& map, K const& key, V const& val);
};

template<typename M, typename K, typename V>
void
Utils::mapSetValue(M& map, K const& key, V const& val)
{
	typename M::iterator const it(map.lower_bound(key));
	if (it == map.end() || map.key_comp()(key, it->first)) {
		map.insert(it, typename M::value_type(key, val));
	} else {
		it->second = val;
	}
}

#endif
