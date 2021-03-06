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

#include "screenshooter.h"
#include "windowhandle.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QImage>
#include <QPainter>

#include <limits>

namespace ActionTools
{
    QList< QPair<QPixmap, QRect> > ScreenShooter::captureScreens()
    {
        QDesktopWidget *desktop = QApplication::desktop();
        QList< QPair<QPixmap, QRect> > result;

        for(int screenIndex = 0; screenIndex < desktop->screenCount(); ++screenIndex)
        {
            const QRect &screenGeometry = desktop->screenGeometry(screenIndex);

            result.append(qMakePair(QPixmap::grabWindow(desktop->winId(), screenGeometry.x(), screenGeometry.y(), screenGeometry.width(), screenGeometry.height()), screenGeometry));
        }

        return result;
    }

    QList<QPair<QPixmap, QRect> > ScreenShooter::captureWindows(const QList<WindowHandle> &windows)
    {
        QDesktopWidget *desktop = QApplication::desktop();
        QList< QPair<QPixmap, QRect> > result;

        foreach(const WindowHandle &window, windows)
        {
            if(!window.isValid())
                continue;

            const QRect &windowGeometry = window.rect();

            result.append(qMakePair(QPixmap::grabWindow(desktop->winId(), windowGeometry.x(), windowGeometry.y(), windowGeometry.width(), windowGeometry.height()), windowGeometry));
        }

        return result;
    }

    QPixmap ScreenShooter::captureScreen()
    {
        const QList< QPair<QPixmap, QRect> > &screens = captureScreens();
        QRect resultRect;
        QPoint minimalTopLeft(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());

        typedef QPair<QPixmap, QRect> PixmapRectPair;
        foreach(const PixmapRectPair &screen, screens)
        {
            const QRect &screenRect = screen.second;

            resultRect = resultRect.united(screenRect);

            if(minimalTopLeft.x() > screenRect.x())
                minimalTopLeft.setX(screenRect.x());
            if(minimalTopLeft.y() > screenRect.y())
                minimalTopLeft.setY(screenRect.y());
        }

        QImage result(resultRect.width(), resultRect.height(), QImage::Format_RGB32);
        {
            QPainter painter(&result);

            foreach(const PixmapRectPair &screen, screens)
            {
                const QRect &screenRect = screen.second;

                painter.drawPixmap(screenRect.x() - minimalTopLeft.x(), screenRect.y() - minimalTopLeft.y(), screen.first);
            }
        }

        return QPixmap::fromImage(result);
    }
}
