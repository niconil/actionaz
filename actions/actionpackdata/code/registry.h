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

#ifndef REGISTRY_H
#define REGISTRY_H

#include "code/codeclass.h"

#include <QObject>
#include <QScriptValue>
#include <QScriptEngine>
#include <QStringList>

#ifdef Q_WS_WIN
#include <Windows.h>
#endif

namespace Code
{
	class Registry : public CodeClass
	{
		Q_OBJECT
		Q_ENUMS(Key)
	
	public:
		enum Key
		{
			ClassesRoot,
			CurrentConfig,
			CurrentUser,
			Users,
			LocalMachine
		};
	
		static QScriptValue constructor(QScriptContext *context, QScriptEngine *engine);
	
		Registry();
		~Registry();
	
	public slots:
		QString toString() const					{ return "Registry"; }
        virtual bool equals(const QScriptValue &other) const;
		QScriptValue openKey(Key key, const QString &subKey);
		QScriptValue createKey(Key key, const QString &subKey);
		QScriptValue setValue(const QString &value, const QVariant &data) const;
		QVariant value(const QString &value = QString()) const;
		QStringList valueNames() const;
		QStringList keys() const;
		QScriptValue deleteValue(const QString &value = QString()) const;
		QScriptValue deleteKey(Key key, const QString &subKey) const;
		QScriptValue deleteKey() const;
		QScriptValue closeKey() const;
	
	private:
	#ifdef Q_WS_WIN
		HKEY enumToKey(Key key) const;
	
		HKEY mHKey;
		Key mRootKey;
		QString mSubKey;
	#endif
	};
}

#endif // REGISTRY_H
