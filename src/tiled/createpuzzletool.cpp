#include "createpuzzletool.h"

#include "mapdocument.h"
#include "mapobjectitem.h"
#include "mappuzzlemodel.h"
#include "mapscene.h"
#include "puzzletypedock.h"
#include "utils.h"

#include <QKeyEvent>

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

CreatePuzzleTool::CreatePuzzleTool(QObject *parent, PuzzleTypeDock *typeDock)
    : CreateTileObjectTool(parent)
    , mTypeDock(typeDock)
    , mPuzzleType(QLatin1String("doorPuzzle"))
    , mSelectedObserver(nullptr)
    , mMode(PuzzleToolMode::CreateNewPuzzle)
    , mCurrentPuzzleName()
{
    setIcon(QIcon(QLatin1String(":images/24x24/puzzle-piece.png")));
    languageChanged();
}

void CreatePuzzleTool::setNewMode(PuzzleToolMode newMode, TileSelectedObserver *tileSelectedObserver)
{
    mSelectedObserver = tileSelectedObserver;
    changeMode(newMode);
}

void CreatePuzzleTool::activate(Tiled::Internal::MapScene *scene)
{
    CreateObjectTool::activate(scene);
}

void CreatePuzzleTool::languageChanged()
{
    setName(tr("Insert Puzzle"));
    setShortcut(QKeySequence(tr("P")));
}

bool CreatePuzzleTool::startNewMapObject(const QPointF &pos, ObjectGroup *objectGroup)
{
    switch(mMode)
    {
    case PuzzleToolMode::CreateNewPuzzle:
    {
        AddPuzzleDialog dialog(mTypeDock);
        if (dialog.exec() == AddPuzzleDialog::Accepted)
        {
            mPuzzleType = dialog.puzzleType();
            mSelectedObserver = mTypeDock->setPuzzleType(mPuzzleType);
            return CreateObjectTool::startNewMapObject(pos, objectGroup);
        }
        break;
    }
    case PuzzleToolMode::CreatePuzzlePart:
        return CreateObjectTool::startNewMapObject(pos, objectGroup);
    default:
        break;
    }
    return false;
}

MapObject *CreatePuzzleTool::createNewMapObject()
{
    if (!mTile)
        return nullptr;

    mapScene()->mapDocument()->mapPuzzleModel()->startChange();
    MapObject *newMapObject = new MapObject;
    newMapObject->setShape(MapObject::Rectangle);
    newMapObject->setCell(Cell(mTile));
    newMapObject->setSize(mTile->size());
    switch (mMode)
    {
    case PuzzleToolMode::CreateNewPuzzle:
        mCurrentPuzzleName = mapScene()->mapDocument()->mapPuzzleModel()->getNextPuzzleName();
        newMapObject->setProperty(MapPuzzleModel::PUZZLE_ROOT, mPuzzleType);
        break;
    default:
        break;
    }

    newMapObject->setProperty(MapPuzzleModel::PUZZLE_PART, mCurrentPuzzleName);
    mSelectedObserver->selected(mapScene()->mapDocument(), newMapObject, mCurrentPuzzleName);
    return newMapObject;
}

void CreatePuzzleTool::keyPressed(QKeyEvent *event)
{
    if (mMode == PuzzleToolMode::ApplyProperty)
    {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            moveToNextTool();
            break;
        case Qt::Key_Escape:
            changeMode(PuzzleToolMode::CreateNewPuzzle);
            break;
        }
    }
    else
    {
        CreateTileObjectTool::keyPressed(event);
    }
}

void CreatePuzzleTool::mouseMoved(const QPointF &pos, Qt::KeyboardModifiers modifiers)
{
    switch (mMode)
    {
    case PuzzleToolMode::CreateNewPuzzle:
    case PuzzleToolMode::CreatePuzzlePart:
        CreateTileObjectTool::mouseMoved(pos, modifiers);
        break;
    case PuzzleToolMode::ApplyProperty:
        AbstractObjectTool::mouseMoved(pos, modifiers);
        break;
    }

    refreshCursor();
}

void CreatePuzzleTool::mousePressed(QGraphicsSceneMouseEvent *event)
{
    switch (mMode)
    {
    case PuzzleToolMode::CreateNewPuzzle:
    case PuzzleToolMode::CreatePuzzlePart:
        CreateTileObjectTool::mousePressed(event);
        break;
    case PuzzleToolMode::ApplyProperty:
        if (event->button() != Qt::LeftButton) {
            AbstractObjectTool::mousePressed(event);
            return;
        }
        applyPropertyMousePressed(event);
        break;
    }
}

void CreatePuzzleTool::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    if (mMode != PuzzleToolMode::ApplyProperty)
        CreateTileObjectTool::mouseReleased(event);
}

void CreatePuzzleTool::cancelNewMapObject()
{
    CreateTileObjectTool::cancelNewMapObject();
    mCurrentPuzzleName = QString();
}

void CreatePuzzleTool::finishNewMapObject()
{
    bool created = false;
    if (currentObjectGroup() && !mCurrentPuzzleName.isNull())
    {
        MapObject *newMapObject = mNewMapObjectItem->mapObject();
        mapScene()->mapDocument()->mapPuzzleModel()->addPuzzlePart(newMapObject, mCurrentPuzzleName);
        created = true;
    }
    CreateTileObjectTool::finishNewMapObject();
    mapScene()->mapDocument()->mapPuzzleModel()->endChange();
    if (created)
    {
        moveToNextTool();
        mTypeDock->raise();
    }
}

void CreatePuzzleTool::applyPropertyMousePressed(QGraphicsSceneMouseEvent *event)
{
    QPointF scenePosition = event->scenePos();
    MapObjectItem *clickedItem = topMostObjectItemAt(scenePosition);
    if (clickedItem)
    {
        MapObject *clickedObject = clickedItem->mapObject();
        if (clickedObject && mSelectedObserver && !mCurrentPuzzleName.isNull())
        {
            mSelectedObserver->selected(mapScene()->mapDocument(), clickedObject, mCurrentPuzzleName);
        }
    }
}

void CreatePuzzleTool::changeMode(PuzzleToolMode newMode)
{
    mMode = newMode;
    refreshCursor();
}

void CreatePuzzleTool::refreshCursor()
{
    Qt::CursorShape cursorShape = Qt::ArrowCursor;
    switch(mMode)
    {
    case PuzzleToolMode::ApplyProperty:
        cursorShape = Qt::PointingHandCursor;
    }
    if (cursor().shape() != cursorShape)
        setCursor(cursorShape);
}

void CreatePuzzleTool::moveToNextTool()
{
    if (!(mSelectedObserver = mTypeDock->getNextTool()))
    {
        changeMode(PuzzleToolMode::CreateNewPuzzle);
    }
    else
    {
        changeMode(mSelectedObserver->getMode());
    }
}
