include(../common_actions.pri)
include(actions/actions.pri)
TARGET = ActionPackInternal
QT += script
HEADERS += actionpackinternal.h
RESOURCES += actionpackinternal.qrc
TRANSLATIONS = ../../locale/actionpackinternal_fr_FR.ts
INCLUDEPATH += ../../executer
LIBS += -L../.. -lexecuter

unix {
	locales.path = $${PREFIX}/share/actionaz/locale
	locales.files = ../../locale/actionpackinternal_fr_FR.qm
	locales.CONFIG = no_check_exist

	INSTALLS += locales
}
