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

#ifndef BOOST_MULTI_INDEX_FOREACH_FIX_H_
#define BOOST_MULTI_INDEX_FOREACH_FIX_H_

#include <boost/foreach.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/multi_index/sequenced_index.hpp>

// BOOST_FOREACH() in boost >= 1.47 has problems with gcc >= 4.6
// These problems aren't specific to boost::multi_index,
// but the code below only deals with it.
// In future versions of boost, they might include equivalent
// code in boost::multi_index itself, which will lead to build problems.
// If / when this happens, conditional compilation will be necessary.

namespace boost 
{
namespace foreach
{

template<typename SuperMeta, typename TagList>
struct is_noncopyable<boost::multi_index::detail::sequenced_index<SuperMeta, TagList> > : mpl::true_
{
};

} // namespace foreach
} // namespace boost

#endif
