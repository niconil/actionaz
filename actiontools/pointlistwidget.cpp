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

#include "pointlistwidget.h"
#include "ui_pointlistwidget.h"
#include "pointitemdelegate.h"

#include <QDebug>

namespace ActionTools
{
	PointListWidget::PointListWidget(QWidget *parent)
		: QWidget(parent),
		ui(new Ui::PointListWidget)
	{
		ui->setupUi(this);

		setMinimumHeight(250);

		updateClearStatus();
		on_list_itemSelectionChanged();

		delete ui->list->itemDelegate();
		ui->list->setItemDelegate(new PointItemDelegate(this));

		connect(ui->addPositionPushButton, SIGNAL(positionChosen(QPoint)), this, SLOT(positionChosen(QPoint)));
		connect(ui->capturePathPushButton, SIGNAL(positionChosen(QPoint)), this, SLOT(stopCapture()));
		connect(&mCaptureTimer, SIGNAL(timeout()), this, SLOT(capture()));
	}

	PointListWidget::~PointListWidget()
	{
		delete ui;
	}

	QPolygon PointListWidget::points() const
	{
		QPolygon back;

		for(int index = 0; index < ui->list->rowCount(); ++index)
			back.append(QPoint(ui->list->item(index, 0)->text().toInt(),
							   ui->list->item(index, 1)->text().toInt()));

		return back;
	}

	void PointListWidget::setPoints(const QPolygon &points)
	{
		on_clearPushButton_clicked();

		foreach(const QPoint &point, points)
			addPoint(point);

		updateClearStatus();
	}

	void PointListWidget::addPoint(const QPoint &point)
	{
		int row = ui->list->rowCount();
		ui->list->setRowCount(row + 1);

		QTableWidgetItem *item = new QTableWidgetItem(QString::number(point.x()));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->list->setItem(row, 0, item);

		item = new QTableWidgetItem(QString::number(point.y()));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->list->setItem(row, 1, item);

		updateClearStatus();
	}

	void PointListWidget::clear()
	{
		on_clearPushButton_clicked();
	}

	void PointListWidget::on_addPushButton_clicked()
	{
		addPoint(QPoint());

		ui->list->scrollToBottom();

		updateClearStatus();
	}

	void PointListWidget::on_addPositionPushButton_clicked()
	{
		updateClearStatus();
	}

	void PointListWidget::on_removePushButton_clicked()
	{
		int row = ui->list->row(ui->list->currentItem());
		ui->list->removeRow(row);

		updateClearStatus();
	}

	void PointListWidget::on_clearPushButton_clicked()
	{
		ui->list->setRowCount(0);

		updateClearStatus();
	}

	void PointListWidget::positionChosen(QPoint position)
	{
		addPoint(position);
	}

	void PointListWidget::on_list_itemSelectionChanged()
	{
		ui->removePushButton->setEnabled(!ui->list->selectedItems().isEmpty());
	}

	void PointListWidget::on_capturePathPushButton_chooseStarted()
	{
		mCaptureTimer.start(25);
	}

	void PointListWidget::capture()
	{
		addPoint(QCursor::pos());
	}

	void PointListWidget::stopCapture()
	{
		mCaptureTimer.stop();
	}

	void PointListWidget::updateClearStatus()
	{
		ui->clearPushButton->setEnabled(ui->list->rowCount() > 0);
	}
}
