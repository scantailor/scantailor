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

#include "SmartFilenameOrdering.h"
#include <QFileInfo>
#include <QString>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace Tests
{

BOOST_AUTO_TEST_SUITE(SmartFilenameOrderingTestSuite);

BOOST_AUTO_TEST_CASE(test_same_file)
{
	SmartFilenameOrdering const less;
	QFileInfo const somefile("/etc/somefile");
	BOOST_CHECK(!less(somefile, somefile));
}

BOOST_AUTO_TEST_CASE(test_dirs_different)
{
	SmartFilenameOrdering const less;
	QFileInfo const lhs("/etc/file");
	QFileInfo const rhs("/ect/file");
	BOOST_CHECK(less(lhs, rhs) == (lhs.absolutePath() < rhs.absolutePath()));
	BOOST_CHECK(less(rhs, lhs) == (rhs.absolutePath() < lhs.absolutePath()));
}

BOOST_AUTO_TEST_CASE(test_simple_case)
{
	SmartFilenameOrdering const less;
	QFileInfo const lhs("/etc/1.png");
	QFileInfo const rhs("/etc/2.png");
	BOOST_CHECK(less(lhs, rhs) == true);
	BOOST_CHECK(less(rhs, lhs) == false);
}

BOOST_AUTO_TEST_CASE(test_avg_case)
{
	SmartFilenameOrdering const less;
	QFileInfo const lhs("/etc/a_0002.png");
	QFileInfo const rhs("/etc/a_1.png");
	BOOST_CHECK(less(lhs, rhs) == false);
	BOOST_CHECK(less(rhs, lhs) == true);
}

BOOST_AUTO_TEST_CASE(test_compex_case)
{
	SmartFilenameOrdering const less;
	QFileInfo const lhs("/etc/a10_10.png");
	QFileInfo const rhs("/etc/a010_2.png");
	BOOST_CHECK(less(lhs, rhs) == false);
	BOOST_CHECK(less(rhs, lhs) == true);
}

BOOST_AUTO_TEST_CASE(test_almost_equal)
{
	SmartFilenameOrdering const less;
	QFileInfo const lhs("/etc/10.png");
	QFileInfo const rhs("/etc/010.png");
	BOOST_CHECK(less(lhs, rhs) == false);
	BOOST_CHECK(less(rhs, lhs) == true);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace Tests
