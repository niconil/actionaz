/*
	Actionaz
	Copyright (C) 2008-2013 Jonathan Mercier-Ganady

	Actionaz is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Actionaz is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

	Contact : jmgr@jmgr.info
*/

#include "parameter.h"

#include <QDebug>

namespace ActionTools
{
	QDataStream &operator << (QDataStream &s, const Parameter &parameter)
	{
		s << parameter.subParameters();

		return s;
	}

	QDataStream &operator >> (QDataStream &s, Parameter &parameter)
	{
		SubParameterHash subParameters;

		s >> subParameters;

		parameter.setSubParameters(subParameters);

		return s;
	}

	QDebug &operator << (QDebug &dbg, const Parameter &parameter)
	{
		foreach(const QString &subParameterName, parameter.subParameters().keys())
		{
			dbg.space() << subParameterName << "=" << parameter.subParameters().value(subParameterName);
		}

		return dbg.maybeSpace();
	}
}
