#include "mappuzzlemodel.h"

#include "changemapobject.h"
#include "changeproperties.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mapobjectmodel.h"
#include "objectgroup.h"
#include "puzzletypemanager.h"

#include <qstring.h>

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

const QString MapPuzzleModel::PUZZLE_PART = QStringLiteral("puzzlePart");
const QString MapPuzzleModel::PUZZLE_PART_TYPE = QStringLiteral("puzzlePartType");
const QString MapPuzzleModel::PUZZLE_ROOT = QStringLiteral("puzzleRoot");

MapPuzzleModel::MapPuzzleModel(QObject *parent):
    QAbstractItemModel(parent),
    mMapDocument(nullptr),
    mPuzzleRootIcon(QLatin1String(":images/24x24/puzzle-piece.png"))
{
}

MapPuzzleModel::~MapPuzzleModel()
{
    qDeleteAll(mPuzzles);
    qDeleteAll(mPuzzleParts);
}

QModelIndex MapPuzzleModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row < mFullPuzzles.count())
            return createIndex(row, column, mFullPuzzles.at(row));
        return QModelIndex();
    }

    PartOrPuzzle *puzzle = toPuzzle(parent);

    // happens when deleting the last item in a parent
    if (row >= puzzle->mParts->count())
        return QModelIndex();

    return createIndex(row, column, puzzle->mParts->at(row));
}

QModelIndex MapPuzzleModel::parent(const QModelIndex &child) const
{
    if (toPuzzlePart(child))
    {
        QString name = puzzleName(child);
        if (mPuzzles.contains(name))
        {
            return this->index(name);
        }
    }
    return QModelIndex();
}

int MapPuzzleModel::rowCount(const QModelIndex &parent) const
{
    if (!mMapDocument)
        return 0;
    if (!parent.isValid())
        return mFullPuzzles.size();
    if (PartOrPuzzle *puzzle = toPuzzle(parent))
        return puzzle->mParts->size();
    return 0;
}

int MapPuzzleModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant MapPuzzleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0: return tr("Puzzle Name");
        case 1: return tr("Puzzle Part Type");
        }
    }
    return QVariant();
}

bool MapPuzzleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (MapObject *mapObject = toPuzzlePart(index))
    {
        switch(role)
        {
        case Qt::CheckStateRole:
        {
            Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
            const bool visible = (c == Qt::Checked);
            if (visible != mapObject->isVisible())
            {
                QUndoCommand *command = new SetMapObjectVisible(mMapDocument,
                                                                mapObject,
                                                                visible);
                mMapDocument->undoStack()->push(command);
            }
            return true;
        }
        case Qt::EditRole:
        {
            const QString s = value.toString();
            if (index.column() == 1 && puzzlePartType(index) != s)
            {
                QUndoStack *undo = mMapDocument->undoStack();
                undo->beginMacro(tr("Change Puzzle Part Type"));
                undo->push(new SetProperty(mMapDocument,
                                           QList<Object*>() << mapObject,
                                           PUZZLE_PART_TYPE,
                                           s));
                onPuzzleChanged(index);
                undo->endMacro();

                return true;
            }
        }
        }
        return false;
    }
    if (PartOrPuzzle *puzzle = toPuzzle(index))
    {
        switch(role)
        {
        case Qt::CheckStateRole:
        {
            Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
            const bool visible = (c == Qt::Checked);
            //if (visible != puzzle->isVisible())
            //{
            //    QUndoCommand *command = new SetMapObjectVisible(mMapDocument,
            //                                                    mapObject,
            //                                                    visible);
            //    mMapDocument->undoStack()->push(command);
            //}
            return true;
        }
        case Qt::EditRole:
        {
            QString newName = value.toString();
            if (puzzle->mName != newName)
            {
                changePuzzleName(puzzle, newName);
            }
            return true;
        }
        }
        return false;
    }
    return false;
}

QVariant MapPuzzleModel::data(const QModelIndex &index, int role) const
{
    if (MapObject *mapObject = toPuzzlePart(index))
    {
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return index.column() ? puzzleName(index) : puzzlePartType(index);
        case Qt::DecorationRole:
            return isPuzzleRoot(index) ? mPuzzleRootIcon : QVariant();
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return mapObject->isVisible() ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    }
    if (PartOrPuzzle *puzzle = toPuzzle(index))
    {
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return index.column() ? QVariant() : puzzle->mName;
        case Qt::DecorationRole:
            return QVariant();
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return QVariant();
        default:
            return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags MapPuzzleModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractItemModel::flags(index);
    if (index.column() == 0)
    {
        rc |= Qt::ItemIsEditable;
        if (!toPuzzle(index))
            rc |= Qt::ItemIsUserCheckable;
    }
    else if (index.parent().isValid())
        rc |= Qt::ItemIsEditable;
    return rc;
}

QModelIndex MapPuzzleModel::index(const QString &name) const
{
    PartOrPuzzle *puzzle = mPuzzles[name];
    if (!puzzle)
    {
        return QModelIndex();
    }
    const int row = mFullPuzzles.indexOf(puzzle);
    return createIndex(row, 0, puzzle);
}

QModelIndex MapPuzzleModel::index(MapObject *o, int column) const
{
    PartOrPuzzle *puzzlePart = mPuzzleParts[o];
    if (!puzzlePart)
    {
        return QModelIndex();
    }
    PartOrPuzzle *puzzle = mPuzzles[puzzlePart->mName];
    if (!puzzle)
    {
        return QModelIndex();
    }
    const int row = puzzle->mParts->indexOf(puzzlePart);
    return createIndex(row, column, puzzlePart);
}

MapPuzzleModel::PartOrPuzzle *MapPuzzleModel::toPartOrPuzzle(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    PartOrPuzzle *puzzle = static_cast<PartOrPuzzle*>(index.internalPointer());
    return puzzle;
}

MapPuzzleModel::PartOrPuzzle *MapPuzzleModel::toPuzzle(const QModelIndex &index) const
{
    PartOrPuzzle *puzzle = toPartOrPuzzle(index);
    if (puzzle && puzzle->mParts)
        return puzzle;
    return nullptr;
}

MapObject *MapPuzzleModel::toPuzzlePart(const QModelIndex &index) const
{
    PartOrPuzzle *puzzle = toPartOrPuzzle(index);
    if (puzzle)
        return puzzle->mPart;
    return nullptr;
}

QString MapPuzzleModel::puzzleName(const QModelIndex &index) const
{
    PartOrPuzzle *partOrPuzzle = toPartOrPuzzle(index);
    return partOrPuzzle ? partOrPuzzle->mName : tr("");
}

QString MapPuzzleModel::puzzlePartType(const QModelIndex &index) const
{
    PartOrPuzzle *partOrPuzzle = toPartOrPuzzle(index);
    return partOrPuzzle ? partOrPuzzle->mPuzzlePartType : tr("");
}

bool MapPuzzleModel::isPuzzleRoot(const QModelIndex &index) const
{
    PartOrPuzzle *puzzle = toPartOrPuzzle(index);
    if (puzzle && puzzle->mPart)
        return puzzle->mPart->hasProperty(PUZZLE_ROOT);
    return false;
}

QString MapPuzzleModel::getNextPuzzleName() const
{
    QString nextPuzzleName = QStringLiteral("puzzle_1");
    int next = 2;
    while (mPuzzleNames.contains(nextPuzzleName))
    {
        nextPuzzleName = QStringLiteral("puzzle_") + QString::number(next);
        next++;
    }
    return nextPuzzleName;
}

void MapPuzzleModel::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    if (mMapDocument)
    {
        mMapDocument->disconnect(this);
        mMapDocument->mapObjectModel()->disconnect(this);
    }

    beginResetModel();
    mMapDocument = mapDocument;

    mPuzzles.clear();
    mPuzzleParts.clear();
    qDeleteAll(mFullPuzzles);
    mFullPuzzles.clear();
    mPuzzleNames.clear();

    if (mMapDocument)
    {
        MapObjectModel *mapObjectModel = mMapDocument->mapObjectModel();
        Map *map = mMapDocument->map();

        connect(mapObjectModel, &MapObjectModel::objectsAdded,
                this, &MapPuzzleModel::objectsAdded);
        connect(mapObjectModel, &MapObjectModel::objectsRemoved,
                this, &MapPuzzleModel::objectsRemoved);
        connect(mMapDocument, &MapDocument::propertyAdded,
                this, &MapPuzzleModel::propertyAdded);
        connect(mMapDocument, &MapDocument::propertyRemoved,
                this, &MapPuzzleModel::propertyRemoved);
        connect(mMapDocument, &MapDocument::propertyChanged,
                this, &MapPuzzleModel::propertyChanged);
        connect(mMapDocument, &MapDocument::propertiesChanged,
                this, &MapPuzzleModel::propertiesChanged);

        for (ObjectGroup *og : map->objectGroups())
        {
            for (MapObject *mo : og->objects())
            {
                if (mo->hasProperty(PUZZLE_PART))
                {
                    QString puzzleName = mo->propertyAsString(PUZZLE_PART);
                    PartOrPuzzle *puzzle = mPuzzles[puzzleName];
                    if (!puzzle)
                    {
                        puzzle = new PartOrPuzzle(puzzleName);
                        mFullPuzzles.append(puzzle);
                        mPuzzles.insert(puzzleName, puzzle);
                        mPuzzleNames.append(puzzleName);
                    }
                    PartOrPuzzle *puzzlePart = new PartOrPuzzle(mo, puzzleName);
                    puzzle->mParts->append(puzzlePart);
                    mPuzzleParts.insert(mo, puzzlePart);
                    if (mo->hasProperty(PUZZLE_ROOT))
                    {
                        if (mo->hasProperty(PUZZLE_ROOT))
                        {
                            puzzle->mPuzzleType = mo->propertyAsString(PUZZLE_ROOT);
                        }
                    }
                }
            }
        }
    }

    endResetModel();
}

bool MapPuzzleModel::changePuzzleName(PartOrPuzzle* partOrPuzzle, const QString &newName)
{
    if (!partOrPuzzle || mPuzzleNames.contains(newName) || newName == partOrPuzzle->mName)
    {
        return false;
    }
    if (partOrPuzzle->mParts)
    {
        Q_ASSERT(mPuzzles[partOrPuzzle->mName] == partOrPuzzle);
        const int nameIndex = mPuzzleNames.indexOf(partOrPuzzle->mName);
        QUndoStack *undo = mMapDocument->undoStack();
        undo->beginMacro(tr("Change name of puzzle."));
        mPuzzles.remove(partOrPuzzle->mName);
        mPuzzles.insert(newName, partOrPuzzle);
        partOrPuzzle->mName = newName;
        for (PartOrPuzzle *part : *(partOrPuzzle->mParts))
        {
            part->mName = newName;
            undo->push(new SetProperty(mMapDocument,
                                       QList<Object*>() << part->mPart,
                                       PUZZLE_PART,
                                       newName));
        }
        mPuzzleNames.replace(nameIndex, newName);
        QModelIndex index = this->index(newName);
        emit dataChanged(index, index);
        onPuzzleChanged(index);
        undo->endMacro();
        return true;
    }
    PartOrPuzzle *puzzle = mPuzzles[partOrPuzzle->mName];
    return changePuzzleName(puzzle, newName);
}

bool MapPuzzleModel::addPuzzlePart(MapObject *o, const QString &puzzleName)
{
    if (puzzlePartExists(o))
        return false;
    internalAddPuzzlePart(o, puzzleName);
    return true;
}

bool MapPuzzleModel::createPuzzlePart(MapObject *o, const QString &puzzleName, bool asPuzzleRoot)
{
    if (puzzlePartExists(o))
        return false;
    QUndoStack *undo = mMapDocument->undoStack();
    undo->beginMacro(tr("Creating a puzzle part"));
    undo->push(new SetProperty(mMapDocument,
                               QList<Object*>() << o,
                               PUZZLE_PART,
                               puzzleName));
    if (asPuzzleRoot)
    {
        undo->push(new SetProperty(mMapDocument,
                                   QList<Object*>() << o,
                                   PUZZLE_ROOT,
                                   tr("")));
    }
    addPuzzlePart(o, puzzleName);
    undo->endMacro();
    return true;
}

bool MapPuzzleModel::createPuzzlePart(MapObject *o, const QString &puzzleName, const QString &puzzlePartType, bool asPuzzleRoot)
{
    if (puzzlePartExists(o))
        return false;
    QUndoStack *undo = mMapDocument->undoStack();
    undo->beginMacro(tr("Creating a puzzle part"));
    undo->push(new SetProperty(mMapDocument,
                               QList<Object*>() << o,
                               PUZZLE_PART,
                               puzzleName));

    undo->push(new SetProperty(mMapDocument,
                               QList<Object*>() << o,
                               PUZZLE_PART_TYPE,
                               puzzlePartType));
    if (asPuzzleRoot)
    {
        undo->push(new SetProperty(mMapDocument,
                                   QList<Object*>() << o,
                                   PUZZLE_ROOT,
                                   tr("")));
    }
    internalAddPuzzlePart(o, puzzleName);
    undo->endMacro();
    return true;
}

bool MapPuzzleModel::puzzlePartExists(MapObject *o)
{
    return mPuzzleParts[o];
}

void MapPuzzleModel::objectsAdded(const QList<MapObject*> &objects)
{
    for (MapObject *mo : objects)
    {
        updateModelIfNecessary(mo);
    }
}

void MapPuzzleModel::objectsRemoved(const QList<MapObject*> &objects)
{
    for (MapObject *mo : objects)
    {
        updateModelIfNecessary(mo);
    }
}

void MapPuzzleModel::propertyAdded(Object *object, const QString &name)
{
    onPropertyChanged(object, name);
}

void MapPuzzleModel::propertyRemoved(Object *object, const QString &name)
{
    onPropertyChanged(object, name);
}

void MapPuzzleModel::propertyChanged(Object *object, const QString &name)
{
    onPropertyChanged(object, name);
}

void MapPuzzleModel::propertiesChanged(Object *object)
{
    MapObject *mo = static_cast<MapObject*>(object);
    updateModelIfNecessary(mo);
}

void MapPuzzleModel::onPropertyChanged(Object *object, const QString &name)
{
    if (mInChange)
    {
        return;
    }
    MapObject *mo = static_cast<MapObject*>(object);
    if (name == PUZZLE_PART)
    {
        updateModelIfNecessary(mo);
    }
    else if(name == PUZZLE_PART_TYPE || name == PUZZLE_ROOT)
    {
        QModelIndex index = this->index(mo);
        if (!index.isValid())
        {
            return;
        }
        QModelIndex indexEnd = this->index(mo, 1);
        emit dataChanged(index, indexEnd);
        onPuzzleChanged(index);
    }
}

void MapPuzzleModel::updateModelIfNecessary(MapObject *mo)
{
    PartOrPuzzle *oldPuzzlePart = mPuzzleParts[mo];
    if (mo->hasProperty(PUZZLE_PART))
    {
        QString currentName = mo->propertyAsString(PUZZLE_PART);
        if (oldPuzzlePart)
        {
            if (oldPuzzlePart->mName == currentName)
                return;
            removePuzzlePart(oldPuzzlePart);
        }
        internalAddPuzzlePart(mo, currentName);
    }
    else if(oldPuzzlePart)
    {
        removePuzzlePart(oldPuzzlePart);
    }
}

void MapPuzzleModel::removePuzzlePart(PartOrPuzzle *puzzlePart)
{
    const QString name = puzzlePart->mName;
    PartOrPuzzle *oldPuzzle = mPuzzles[name];
    Q_ASSERT(oldPuzzle);
    const int row = oldPuzzle->mParts->indexOf(puzzlePart);
    beginRemoveRows(index(name), row, row);
    oldPuzzle->mParts->removeAt(row);
    delete mPuzzleParts.take(puzzlePart->mPart);
    endRemoveRows();
    if (oldPuzzle->mParts->count() == 0)
    {
        const int puzzleRow = mFullPuzzles.indexOf(oldPuzzle);
        beginRemoveRows(QModelIndex(), puzzleRow, puzzleRow);
        mFullPuzzles.removeAt(puzzleRow);
        delete mPuzzles.take(name);
        mPuzzleNames.removeOne(name);
        endRemoveRows();
    }
    onPuzzleChanged(this->index(name));
}

void MapPuzzleModel::internalAddPuzzlePart(MapObject *o, const QString &puzzleName)
{
    PartOrPuzzle *puzzle = mPuzzles[puzzleName];
    if (!puzzle)
    {
        const int puzzleRow = mFullPuzzles.count();
        beginInsertRows(QModelIndex(), puzzleRow, puzzleRow);
        puzzle = new PartOrPuzzle(puzzleName);
        if (o->hasProperty(PUZZLE_ROOT))
        {
            puzzle->mPuzzleType = o->propertyAsString(PUZZLE_ROOT);
        }
        mFullPuzzles.append(puzzle);
        mPuzzleNames.append(puzzleName);
        mPuzzles.insert(puzzleName, puzzle);
        endInsertRows();
    }
    const int row = puzzle->mParts->count();
    beginInsertRows(index(puzzleName), row, row);
    PartOrPuzzle *puzzlePart = new PartOrPuzzle(o, puzzleName);
    puzzle->mParts->append(puzzlePart);
    endInsertRows();
    onPuzzleChanged(index(puzzleName));
}

void MapPuzzleModel::onPuzzleChanged(const QModelIndex &index)
{
    if (index.parent().isValid())
    {
        onPuzzleChanged(index.parent());
    }
    else if (index.isValid())
    {
        PartOrPuzzle *puzzle = this->toPuzzle(index);
        PuzzleTypeManager::instance()->puzzleChanged(puzzle, mMapDocument);
    }
}
