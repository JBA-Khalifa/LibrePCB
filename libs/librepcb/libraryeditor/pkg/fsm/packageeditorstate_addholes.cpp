/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "packageeditorstate_addholes.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include "../packageeditorwidget.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageEditorState_AddHoles::PackageEditorState_AddHoles(Context& context) noexcept :
    PackageEditorState(context), mCurrentHole(nullptr), mCurrentGraphicsItem(nullptr),
    mLastDiameter(1000000)
{
}

PackageEditorState_AddHoles::~PackageEditorState_AddHoles() noexcept
{
    Q_ASSERT(mEditCmd.isNull());
    Q_ASSERT(mCurrentHole == nullptr);
    Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool PackageEditorState_AddHoles::entry() noexcept
{
    mContext.graphicsScene.setSelectionArea(QPainterPath()); // clear selection
    mContext.graphicsView.setCursor(Qt::CrossCursor);

    // populate command toolbar
    mContext.commandToolBar.addLabel(tr("Diameter:"), 10);

    std::unique_ptr<QDoubleSpinBox> diameterSpinBox(new QDoubleSpinBox());
    diameterSpinBox->setMinimum(0.0001);
    diameterSpinBox->setMaximum(100);
    diameterSpinBox->setSingleStep(0.2);
    diameterSpinBox->setDecimals(6);
    diameterSpinBox->setValue(mLastDiameter.toMm());
    connect(diameterSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &PackageEditorState_AddHoles::diameterSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(diameterSpinBox));

    Point pos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
    return startAddHole(pos);
}

bool PackageEditorState_AddHoles::exit() noexcept
{
    if (mCurrentHole && !abortAddHole()) {
        return false;
    }

    // cleanup command toolbar
    mContext.commandToolBar.clear();

    mContext.graphicsView.setCursor(Qt::ArrowCursor);
    return true;
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool PackageEditorState_AddHoles::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (mCurrentHole) {
        Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
        mEditCmd->setPosition(currentPos, true);
        return true;
    } else {
        return false;
    }
}

bool PackageEditorState_AddHoles::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
    if (mCurrentHole) {
        finishAddHole(currentPos);
    }
    return startAddHole(currentPos);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool PackageEditorState_AddHoles::startAddHole(const Point& pos) noexcept
{
    try {
        mStartPos = pos;
        mContext.undoStack.beginCmdGroup(tr("Add hole"));
        mCurrentHole = new Hole(Uuid::createRandom(), pos, mLastDiameter);
        mContext.undoStack.appendToCmdGroup(new CmdHoleInsert(
            mContext.currentFootprint->getHoles(), std::shared_ptr<Hole>(mCurrentHole)));
        mEditCmd.reset(new CmdHoleEdit(*mCurrentHole));
        mCurrentGraphicsItem = mContext.currentGraphicsItem->getHoleGraphicsItem(*mCurrentHole);
        Q_ASSERT(mCurrentGraphicsItem);
        mCurrentGraphicsItem->setSelected(true);
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mCurrentGraphicsItem = nullptr;
        mCurrentHole = nullptr;
        mEditCmd.reset();
        return false;
    }
}

bool PackageEditorState_AddHoles::finishAddHole(const Point& pos) noexcept
{
    if (pos == mStartPos) {
        return abortAddHole();
    }

    try {
        mEditCmd->setPosition(pos, true);
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentHole = nullptr;
        mContext.undoStack.appendToCmdGroup(mEditCmd.take());
        mContext.undoStack.commitCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

bool PackageEditorState_AddHoles::abortAddHole() noexcept
{
    try {
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentHole = nullptr;
        mEditCmd.reset();
        mContext.undoStack.abortCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

void PackageEditorState_AddHoles::diameterSpinBoxValueChanged(double value) noexcept
{
    mLastDiameter = Length::fromMm(value);
    if (mEditCmd) {
        mEditCmd->setDiameter(mLastDiameter, true);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb