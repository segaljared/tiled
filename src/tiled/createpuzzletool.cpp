#include "createpuzzletool.h"

#include "mapdocument.h"
#include "mappuzzlemodel.h"
#include "mapscene.h"
#include "puzzletypedock.h"
#include "utils.h"

using namespace Tiled;
using namespace Tiled::Custom;

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
