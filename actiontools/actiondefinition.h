/*
	Actionaz
	Copyright (C) 2008-2010 Jonathan Mercier-Ganady

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

#ifndef ACTIONDEFINITION_H
#define ACTIONDEFINITION_H

#include "actiontools_global.h"
#include "version.h"
#include "actionexception.h"

#include <QString>
#include <QPixmap>
#include <QList>
#include <QDebug>

class QScriptEngine;

namespace ActionTools
{
	class ActionPack;
	class ActionInstance;
	class ElementDefinition;

	class ACTIONTOOLSSHARED_EXPORT ActionDefinition
	{
	public:
		typedef int Flag;

		enum Status
		{
			Alpha,
			Beta,
			Testing,
			Stable
		};
		enum Category
		{
			None = -1,
			Windows,
			Device,
			System,
			Internal,
			Other,

			CategoryCount
		};
		enum Flags
		{
			WorksOnWindows =    1 << 1,
			WorksOnGnuLinux =	1 << 2,
			WorksOnMac =	    1 << 3,
			Official =			1 << 4
		};

		explicit ActionDefinition(ActionPack *pack) : mPack(pack), mIndex(-1)	{}
		virtual ~ActionDefinition();

		virtual QString name() const = 0;
		virtual QString id() const = 0;
		virtual Flag flags() const														{ return WorksOnWindows | WorksOnGnuLinux | WorksOnMac; }
		virtual QString description() const												{ return QObject::tr("No description"); }
		virtual Tools::Version version() const = 0;
		virtual ActionInstance *newActionInstance() const = 0;
		virtual Status status() const = 0;
		virtual Category category() const = 0;
		virtual QString author() const													{ return (flags() & Official) ? QObject::tr("The Actionaz Team") : QString(); }
		virtual QString website() const													{ return QString(); }
		virtual QString email() const													{ return QString(); }
		virtual QPixmap icon() const													{ return QPixmap(); }
		virtual QStringList tabs() const												{ return QStringList(); }

		void setIndex(int index)														{ mIndex = index; }
		int index() const																{ return mIndex; }

		ActionPack *pack() const														{ return mPack; }
		const QList<ElementDefinition *> &elements() const								{ return mElements; }
		const QList<ActionException *> &exceptions() const								{ return mExceptions; }
		
		ActionInstance *scriptInit(QScriptEngine *scriptEngine) const;
		
		static QString CategoryName[CategoryCount];

	protected:
		void addElement(ElementDefinition *element, int tab = 0);
		void addException(int id, const QString &name)								{ mExceptions.append(new ActionException(id, name)); }

	private:
		ActionPack *mPack;
		QList<ElementDefinition *> mElements;
		QList<ActionException *> mExceptions;
		int mIndex;

		Q_DISABLE_COPY(ActionDefinition)
	};
}

#endif // ACTIONDEFINITION_H