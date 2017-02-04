#include "puzzletypemanager.h"

#include "changeproperties.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mappuzzlemodel.h"

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

PuzzleTypeManager *PuzzleTypeManager::mInstance;

PuzzleTypeManager::PuzzleTypeManager()
{
}

PuzzleTypeManager* PuzzleTypeManager::instance()
{
    if (!mInstance)
    {
        mInstance = new PuzzleTypeManager;
    }
    return mInstance;
}

void PuzzleTypeManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

void PuzzleTypeManager::initialize(const QString &path)
{
    mPuzzleTypeNames.append(QLatin1String("doorPuzzle"));
}

const QList<QString> &PuzzleTypeManager::puzzleTypes()
{
    return mPuzzleTypeNames;
}

void PuzzleTypeManager::puzzleChanged(MapPuzzleModel::PartOrPuzzle *puzzle, MapDocument *mapDocument)
{
    if (puzzle->mPuzzleType == QLatin1String("doorPuzzle"))
    {
        QList<MapPuzzleModel::PartOrPuzzle *> doors;
        MapPuzzleModel::PartOrPuzzle *trigger = nullptr;
        MapPuzzleModel::PartOrPuzzle *keyGen = nullptr;
        for (MapPuzzleModel::PartOrPuzzle *part : *(puzzle->mParts))
        {
            if (part->mPuzzlePartType == QLatin1String("trigger"))
            {
                trigger = part;
            }
            else if (part->mPuzzlePartType == QLatin1String("keyGen"))
            {
                keyGen = part;
            }
            else if (part->mPuzzlePartType == QLatin1String("door"))
            {
                doors.append(part);
            }
        }

        if (!trigger || !keyGen || doors.count() == 0)
        {
            return;
        }

        QUndoStack *undo = mapDocument->undoStack();
        undo->beginMacro(tr("Update puzzle"));
        undo->push(new SetProperty(mapDocument,
                                   QList<Object*>() << trigger->mPart,
                                   QLatin1String("floorTrigger"),
                                   puzzle->mName + QLatin1String("_trigger")));
        undo->push(new SetProperty(mapDocument,
                                   QList<Object*>() << keyGen->mPart,
                                   QLatin1String("isTriggeredBy"),
                                   puzzle->mName + QLatin1String("_trigger")));
        undo->push(new SetProperty(mapDocument,
                                   QList<Object*>() << keyGen->mPart,
                                   QLatin1String("objectIdentifier"),
                                   puzzle->mName + QLatin1String("_keyGen")));
        undo->push(new SetProperty(mapDocument,
                                   QList<Object*>() << keyGen->mPart,
                                   QLatin1String("randomKeyGen"),
                                   doors.length()));
        int i = 0;
        for (MapPuzzleModel::PartOrPuzzle *door : doors)
        {
            undo->push(new SetProperty(mapDocument,
                                       QList<Object*>() << door->mPart,
                                       QLatin1String("unlockedBy"),
                                       QLatin1String("(") + QString::number(i) + QLatin1String(")") + puzzle->mName + QLatin1String("_keyGen")));
            i++;
        }
        undo->endMacro();
    }
}

QList<QString> PuzzleTypeManager::puzzlePartTypes(const QString &puzzleType)
{
    if (puzzleType == QLatin1String("doorPuzzle"))
    {
        QList<QString> puzzlePartTypesList;
        puzzlePartTypesList.append(QLatin1String("trigger"));
        puzzlePartTypesList.append(QLatin1String("keyGen"));
        puzzlePartTypesList.append(QLatin1String("door"));
        return puzzlePartTypesList;
    }
    return QList<QString>();
}

QList<CreatePuzzleTool::PuzzleToolMode> PuzzleTypeManager::puzzleToolModes(const QString &puzzleType)
{
    if (puzzleType == QLatin1String("doorPuzzle"))
    {
        QList<CreatePuzzleTool::PuzzleToolMode> puzzleToolModesList;
        puzzleToolModesList.append(CreatePuzzleTool::PuzzleToolMode::CreateNewPuzzle);
        puzzleToolModesList.append(CreatePuzzleTool::PuzzleToolMode::CreatePuzzlePart);
        puzzleToolModesList.append(CreatePuzzleTool::PuzzleToolMode::ApplyProperty);
        return puzzleToolModesList;
    }
    return QList<CreatePuzzleTool::PuzzleToolMode>();
}

CreatePuzzleTool::PuzzleToolMode PuzzleTypeManager::puzzleToolMode(const QString &puzzleType, const QString &puzzlePartType)
{
    QList<CreatePuzzleTool::PuzzleToolMode> modes = puzzleToolModes(puzzleType);
    int index = puzzlePartTypes(puzzleType).indexOf(puzzlePartType);
    if (index >= 0 && index < modes.count())
    {
        return modes.at(index);
    }
    return CreatePuzzleTool::PuzzleToolMode::CreateNewPuzzle;
}
