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

#include "scriptmodelundocommands.h"
#include "scriptmodel.h"
#include "actioninstance.h"
#include "actiondefinition.h"
#include "script.h"

//ChangeEnabledCommand
ChangeEnabledCommand::ChangeEnabledCommand(const QList<int> &rows, bool enabled, ScriptModel *model)
	: QUndoCommand(),
	mModel(model),
	mRows(rows),
	mNew(enabled)
{
	foreach(int row, mRows)
	{
		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		mOld << actionInstance->isEnabled();
	}

	setText(QObject::tr("Change the enabled status"));
}

void ChangeEnabledCommand::redo()
{
	foreach(int row, mRows)
	{
		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		actionInstance->setEnabled(mNew);

		for(int column = 0; column < mModel->columnCount(); ++column)
			mModel->emitDataChanged(mModel->index(row, column));
	}
}

void ChangeEnabledCommand::undo()
{
	for(int rowIndex = 0; rowIndex < mRows.count(); ++rowIndex)
	{
		int row = mRows.at(rowIndex);
		bool enabled = mOld.at(rowIndex);

		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		actionInstance->setEnabled(enabled);

		for(int column = 0; column < mModel->columnCount(); ++column)
			mModel->emitDataChanged(mModel->index(row, column));
	}
}

//ChangeColorCommand
ChangeColorCommand::ChangeColorCommand(const QList<int> &rows, const QColor &color, ScriptModel *model)
	: QUndoCommand(),
	mModel(model),
	mRows(rows),
	mNew(color)
{
	foreach(int row, mRows)
	{
		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		mOld << actionInstance->color();
	}

	setText(QObject::tr("Change the color"));
}

void ChangeColorCommand::redo()
{
	foreach(int row, mRows)
	{
		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		actionInstance->setColor(mNew);

		for(int column = 0; column < mModel->columnCount(); ++column)
			mModel->emitDataChanged(mModel->index(row, column));
	}
}

void ChangeColorCommand::undo()
{
	for(int rowIndex = 0; rowIndex < mRows.count(); ++rowIndex)
	{
		int row = mRows.at(rowIndex);
		const QColor &color = mOld.at(rowIndex);

		ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(row);
		if(!actionInstance)
			continue;

		actionInstance->setColor(color);

		for(int column = 0; column < mModel->columnCount(); ++column)
			mModel->emitDataChanged(mModel->index(row, column));
	}
}

//ChangeCommentCommand
ChangeCommentCommand::ChangeCommentCommand(const QString &value, int row, ScriptModel *model)
	: QUndoCommand(),
	mModel(model),
	mNew(value),
	mRow(row)
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	mOld = actionInstance->comment();

	setText(QObject::tr("Change the comment from %1 to %2").arg(mOld).arg(mNew));
}

void ChangeCommentCommand::redo()
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setComment(mNew);
	mModel->emitDataChanged(mModel->index(mRow, 2));
}

void ChangeCommentCommand::undo()
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setComment(mOld);
	mModel->emitDataChanged(mModel->index(mRow, 2));
}

//ChangeLabelCommand
ChangeLabelCommand::ChangeLabelCommand(const QString &value, int row, ScriptModel *model)
	: QUndoCommand(),
	mModel(model),
	mNew(value),
	mRow(row)
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	mOld = actionInstance->label();

	setText(QObject::tr("Change the label from %1 to %2").arg(mOld).arg(mNew));
}

void ChangeLabelCommand::redo()
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setLabel(mNew);
	mModel->emitDataChanged(mModel->index(mRow, 0));
}

void ChangeLabelCommand::undo()
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setLabel(mOld);
	mModel->emitDataChanged(mModel->index(mRow, 0));
}

//ChangeDataCommand
ChangeDataCommand::ChangeDataCommand(const QModelIndex &index, const ActionTools::ParametersData &value, ScriptModel *model)
	: QUndoCommand(),
	mModel(model),
	mNew(value)
{
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	mOld = actionInstance->parametersData();
	mRow = index.row();
	mCol = index.column();

	setText(QObject::tr("Change the action parameters on line %1").arg(mRow));
}

void ChangeDataCommand::redo()
{
	QModelIndex index = mModel->index(mRow, mCol);
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setParametersData(mNew);
	mModel->emitDataChanged(index);
}

void ChangeDataCommand::undo()
{
	QModelIndex index = mModel->index(mRow, mCol);
	ActionTools::ActionInstance *actionInstance = mModel->mScript->actionAt(mRow);
	if(!actionInstance)
		return;

	actionInstance->setParametersData(mOld);
	mModel->emitDataChanged(index);
}

//CopyActionCommand
CopyActionCommand::CopyActionCommand(int row, const QList<ActionTools::ActionInstanceBuffer> &actionInstanceBuffers, ScriptModel *model)
	: QUndoCommand(),
	mRow(row),
	mActionInstanceBuffers(actionInstanceBuffers),
	mModel(model)
{
	setText(QObject::tr("Copy some actions"));
}

void CopyActionCommand::redo()
{
	foreach(const ActionTools::ActionInstanceBuffer &actionInstanceBuffer, mActionInstanceBuffers)
	{
		mModel->insertRow(mRow);
		mModel->setData(mModel->index(mRow, 0), actionInstanceBuffer.actionInstanceId(), ScriptModel::ActionIdRole);
		mModel->setData(mModel->index(mRow, 0), actionInstanceBuffer.actionAsVariant(), ScriptModel::ActionDataRole);
	}
}

void CopyActionCommand::undo()
{
	for(int i = 0; i < mActionInstanceBuffers.count(); ++i)
		mModel->removeRow(mRow);
}

//InsertActionCommand
InsertNewActionCommand::InsertNewActionCommand(int row, const ActionTools::ActionInstanceBuffer &actionInstanceBuffer, ScriptModel *model)
	: QUndoCommand(),
	mRow(row),
	mActionInstanceBuffer(actionInstanceBuffer),
	mModel(model)
{
	setText(QObject::tr("Add an action"));
}

void InsertNewActionCommand::redo()
{
	mModel->insertRow(mRow);
	mModel->setData(mModel->index(mRow, 0), mActionInstanceBuffer.actionInstanceId(), ScriptModel::ActionIdRole);
	mModel->setData(mModel->index(mRow, 0), mActionInstanceBuffer.actionAsVariant(), ScriptModel::ActionDataRole);
}

void InsertNewActionCommand::undo()
{
	mModel->removeRow(mRow);
}

//RemoveActionCommand
RemoveActionCommand::RemoveActionCommand(const QList<int> &rows, ScriptModel *model)
	: QUndoCommand(),
	mRows(rows),
	mModel(model)
{
	foreach(int row, rows)
	{
		mActionInstanceBuffers << ActionTools::ActionInstanceBuffer(model->mScript->actionAt(row)->definition()->id(),
											  *model->mScript->actionAt(row));
	}

	setText(QObject::tr("Remove %n action(s)", "", rows.count()));
}

void RemoveActionCommand::redo()
{
	foreach(int row, mRows)
	{
		mModel->removeRow(row);
	}
}

void RemoveActionCommand::undo()
{
	int rowCount = mRows.count();
	for(int rowIndex = rowCount - 1; rowIndex >= 0; --rowIndex)
	{
		int row = mRows.at(rowIndex);

		mModel->insertRow(row);
		mModel->setData(mModel->index(row, 0), mActionInstanceBuffers.at(rowIndex).actionInstanceId(), ScriptModel::ActionIdRole);
		mModel->setData(mModel->index(row, 0), mActionInstanceBuffers.at(rowIndex).actionAsVariant(), ScriptModel::ActionDataRole);
	}
}

//MoveActionCommand
MoveActionCommand::MoveActionCommand(int row, const QList<int> &previousRows, ScriptModel *model)
	: QUndoCommand(),
	mRow(row),
	mPreviousRows(previousRows),
	mModel(model),
        mChangeDest(0),
        mChangePrevious(0)
{
	if(mRow == -1)
		mRow = mModel->rowCount() - 1;

	setText(QObject::tr("Copy some actions"));
}

void MoveActionCommand::redo()
{
	qSort(mPreviousRows.begin(), mPreviousRows.end(), qGreater<int>());

	mChangePrevious = 0;
	mChangeDest = 0;

	for(QList<int>::iterator i = mPreviousRows.begin(); i != mPreviousRows.end();)
	{
		int previousRow = *i + mChangePrevious;
		int destRow = mRow + mChangeDest;

		if(previousRow < destRow)
			--destRow;

		if(!mModel->moveRow(previousRow, destRow))
		{
			i = mPreviousRows.erase(i);
			continue;
		}
		else
			++i;

		if(destRow > previousRow)
			--mChangeDest;
		else
			++mChangePrevious;
	}
}

void MoveActionCommand::undo()
{
	qSort(mPreviousRows.begin(), mPreviousRows.end(), qLess<int>());

	for(QList<int>::iterator i = mPreviousRows.begin(); i != mPreviousRows.end();)
	{
		int previousRow = mRow + mChangeDest;
		int destRow = *i + mChangePrevious;

		if(previousRow < destRow)
			--destRow;

		if(!mModel->moveRow(previousRow, destRow))
		{
			i = mPreviousRows.erase(i);
			continue;
		}
		else
			++i;

		if(destRow < previousRow)
			++mChangeDest;
		else
			--mChangePrevious;
	}
}

//MoveActionOneRowCommand
MoveActionOneRowCommand::MoveActionOneRowCommand(const QList<int> &rows, bool moveUp, ScriptModel *model)
	: QUndoCommand(),
	mRows(rows),
	mMoveUp(moveUp),
	mModel(model)
{
	setText(QObject::tr("Move %n action(s)", "", rows.count()));
}

void MoveActionOneRowCommand::redo()
{
	if(mMoveUp)
		qSort(mRows.begin(), mRows.end(), qLess<int>());
	else
		qSort(mRows.begin(), mRows.end(), qGreater<int>());

	moveAllActions(mMoveUp ? -1 : 1);
}

void MoveActionOneRowCommand::undo()
{
	if(!mMoveUp)
		qSort(mRows.begin(), mRows.end(), qLess<int>());
	else
		qSort(mRows.begin(), mRows.end(), qGreater<int>());

	moveAllActions(mMoveUp ? 1 : -1);
}

void MoveActionOneRowCommand::moveAllActions(int direction)
{
	QList<int>::iterator i = mRows.begin();
	while(i != mRows.end())
	{
		if(move(*i, direction))
			++i;
		else
			i = mRows.erase(i);
	}
}

bool MoveActionOneRowCommand::move(int &row, int direction)
{
	if(row + direction < 0 || row + direction >= mModel->rowCount())
		return false;

	mModel->mSelectionModel->select(mModel->index(row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);

	if(mModel->moveRow(row, row + direction))
	{
		mModel->mSelectionModel->select(mModel->index(row + direction, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
		mModel->mSelectionModel->setCurrentIndex(mModel->index(row + direction, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);

		row += direction;

		return true;
	}

	return false;
}
