#include "createpuzzletool.h"

#include "mapdocument.h"
#include "mapobjectitem.h"
#include "mappuzzlemodel.h"
#include "mapscene.h"
#include "puzzletypedock.h"
#include "utils.h"

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

CreatePuzzleTool::CreatePuzzleTool(QObject *parent, PuzzleTypeDock *typeDock)
    : CreateTileObjectTool(parent)
    , mTypeDock(typeDock)
    , mPuzzleType(AddPuzzleDialog::PuzzleType::FloorTriggeredDoors)
{
    setIcon(QIcon(QLatin1String(":images/24x24/puzzle-piece.png")));
    languageChanged();
}

void CreatePuzzleTool::activate(Tiled::Internal::MapScene *scene)
{
    mTypeDock->raise();
    CreateObjectTool::activate(scene);
}

void CreatePuzzleTool::languageChanged()
{
    setName(tr("Insert Puzzle"));
    setShortcut(QKeySequence(tr("P")));
}

void CreatePuzzleTool::startNewMapObject(const QPointF &pos, ObjectGroup *objectGroup)
{
    AddPuzzleDialog dialog(mTypeDock);
    if (dialog.exec() == AddPuzzleDialog::Accepted)
    {
        mPuzzleType = dialog.puzzleType();
        CreateObjectTool::startNewMapObject(pos, objectGroup);
    }
}

MapObject *CreatePuzzleTool::createNewMapObject()
{
    if (!mTile)
        return nullptr;

    MapObject *newMapObject = new MapObject;
    newMapObject->setShape(MapObject::Rectangle);
    newMapObject->setCell(Cell(mTile));
    newMapObject->setSize(mTile->size());
    QString newName = mapScene()->mapDocument()->mapPuzzleModel()->getNextPuzzleName();
    newMapObject->setProperty(MapPuzzleModel::PUZZLE_PART, newName);
    mapScene()->mapDocument()->mapPuzzleModel()->addPuzzlePart(newMapObject, newName);
    return newMapObject;
}

void CreatePuzzleTool::keyPressed(QKeyEvent *event)
{
    if (mMode == PuzzleToolMode::ApplyProperty)
    {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (!(mSelected = mTypeDock->getNextTool()))
            {
                changeMode(PuzzleToolMode::Create);
            }
            break;
        case Qt::Key_Escape:
            changeMode(PuzzleToolMode::Create);
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
    case PuzzleToolMode::Create:
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
    case PuzzleToolMode::Create:
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
    if (mMode == PuzzleToolMode::Create)
        CreateTileObjectTool::mouseReleased(event);
}

void CreatePuzzleTool::applyPropertyMousePressed(QGraphicsSceneMouseEvent *event)
{
    QPointF scenePosition = event->scenePos();
    MapObjectItem *clickedItem = topMostObjectItemAt(scenePosition);
    if (clickedItem)
    {
        MapObject *clickedObject = clickedItem->mapObject();
        if (clickedObject && mSelected)
        {
            mSelected(clickedObject);
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
