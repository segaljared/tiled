#include "puzzletypemanager.h"

#include "changeproperties.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mappuzzlemodel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpressionMatchIterator>
#include <QTextStream>

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

PuzzleTypeManager *PuzzleTypeManager::mInstance;

PuzzleTypeManager::PuzzleTypeManager()
    : mIdentifierRegex(QLatin1Literal("\\{id:(?<id>[^\\s\\{\\}]+)\\}")),
      mCountRegex(QLatin1Literal("\\{count:(?<partType>[^\\s\\{\\}]+)\\}")),
      mIndexRegex(QLatin1Literal("\\{index\\}"))
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
    mPuzzles.clear();
    QFile puzzleFile(path);
    if (!puzzleFile.open(QIODevice::ReadOnly))
    {
        PuzzleInformation *doorInfo = new PuzzleInformation(QLatin1String("doorPuzzle"));
        doorInfo->addPuzzlePartType(QLatin1Literal("trigger"));
        doorInfo->addPuzzlePartType(QLatin1Literal("keyGen"));
        doorInfo->addPuzzlePartType(QLatin1Literal("door"));

        doorInfo->addMinimum(QLatin1Literal("trigger"), 1);
        doorInfo->addMinimum(QLatin1Literal("keyGen"), 1);
        doorInfo->addMinimum(QLatin1Literal("door"), 1);
        doorInfo->addMaximum(QLatin1Literal("trigger"), 1);
        doorInfo->addMaximum(QLatin1Literal("keyGen"), 1);

        doorInfo->setPartTypeMode(QLatin1Literal("trigger"), CreatePuzzleTool::PuzzleToolMode::CreateNewPuzzle);
        doorInfo->setPartTypeMode(QLatin1Literal("keyGen"), CreatePuzzleTool::PuzzleToolMode::CreatePuzzlePart);
        doorInfo->setPartTypeMode(QLatin1Literal("door"), CreatePuzzleTool::PuzzleToolMode::ApplyProperty);

        doorInfo->addPuzzlePartEntry(QLatin1Literal("trigger"), QLatin1Literal("floorTrigger"), QLatin1Literal("{id:trigger}"));
        doorInfo->addPuzzlePartEntry(QLatin1Literal("keyGen"), QLatin1Literal("isTriggeredBy"), QLatin1Literal("{id:trigger}"));
        doorInfo->addPuzzlePartEntry(QLatin1Literal("keyGen"), QLatin1Literal("objectIdentifier"), QLatin1Literal("{id:keyGen}"));
        doorInfo->addPuzzlePartEntry(QLatin1Literal("keyGen"), QLatin1Literal("randomKeyGen"), QLatin1Literal("{count:door}"));
        doorInfo->addPuzzlePartEntry(QLatin1Literal("door"), QLatin1Literal("unlockedBy"), QLatin1Literal("({index}){id:keyGen}"));

        doorInfo->finalize();
        mPuzzles[doorInfo->getName()] = doorInfo;
    }
    else
    {
        QByteArray puzzleData = puzzleFile.readAll();

        QJsonDocument puzzleDocument(QJsonDocument::fromJson(puzzleData));
        for(const QJsonValue &value : puzzleDocument.array())
        {
            QJsonObject puzzleInfoData = value.toObject();
            PuzzleInformation *puzzleInfo = PuzzleInformation::fromJson(puzzleInfoData);


        }
    }

    //READ STUFF

    mPuzzleTypeNames = mPuzzles.keys();
}

const QList<QString> &PuzzleTypeManager::puzzleTypes()
{
    return mPuzzleTypeNames;
}

void PuzzleTypeManager::puzzleChanged(MapPuzzleModel::PartOrPuzzle *puzzle, MapDocument *mapDocument)
{
    const PuzzleInformation *puzzleInfo = mPuzzles[puzzle->mPuzzleType];
    if (puzzleInfo)
    {
        QMap<QString, QList<MapPuzzleModel::PartOrPuzzle*>> parts;
        for (MapPuzzleModel::PartOrPuzzle *part : *(puzzle->mParts))
        {
            parts[part->mPuzzlePartType].append(part);
        }
        // verify minimums have been met
        for (QString partType : puzzleInfo->getPuzzlePartTypes())
        {
            if (!puzzleInfo->isCountAllowed(partType, parts[partType].count()))
            {
                return;
            }
        }

        QMap<MapPuzzleModel::PartOrPuzzle*, QMap<QString, QString>> appliedProperties;
        QMap<QString, QString> identifiers;
        // identifier pass
        QMapIterator<QString, QList<MapPuzzleModel::PartOrPuzzle*>> iter(parts);
        while (iter.hasNext())
        {
            iter.next();
            QMap<QString, QString> entries = puzzleInfo->getPuzzlePartEntries(iter.key());
            int index = 0;
            for (MapPuzzleModel::PartOrPuzzle* part : iter.value())
            {
                QMapIterator<QString, QString> entryIter(entries);
                while (entryIter.hasNext())
                {
                    entryIter.next();
                    QString entry = applyEntryNonIdentifiers(entryIter.value(), index, parts);
                    addIdentifier(puzzle->mName, entry, identifiers);
                    (appliedProperties[part])[entryIter.key()] = entry;
                }
                index++;
            }
        }

        QUndoStack *undo = mapDocument->undoStack();
        undo->beginMacro(tr("Update puzzle"));

        QMapIterator<MapPuzzleModel::PartOrPuzzle*, QMap<QString, QString>> partIter(appliedProperties);
        while (partIter.hasNext())
        {
            partIter.next();
            QMapIterator<QString, QString> propIter(partIter.value());
            while (propIter.hasNext())
            {
                propIter.next();
                undo->push(new SetProperty(mapDocument,
                                           QList<Object*>() << partIter.key()->mPart,
                                           propIter.key(),
                                           applyIdentifiers(propIter.value(), identifiers)));
            }
        }

        //QMapIterator<QString, QList<MapPuzzleModel::PartOrPuzzle*>> partIter(parts);
        //while (partIter.hasNext())
        //{
        //    partIter.next();
        //    QMap<QString, QString> entries = puzzleInfo->getPuzzlePartEntries(partIter.key());
        //    int index = 0;
        //    for (MapPuzzleModel::PartOrPuzzle * part : partIter.value())
        //    {
        //        QMapIterator<QString, QString> entryIter(entries);
        //        while (entryIter.hasNext())
        //        {
        //            entryIter.next();
        //            undo->push(new SetProperty(mapDocument,
        //                                       QList<Object*>() << part->mPart,
        //                                       entryIter.key(),
        //                                       applyEntry(entryIter.value(), index, parts, identifiers)));
        //        }
        //        index++;
        //    }
        //}

        undo->endMacro();
    }
}

QList<QString> PuzzleTypeManager::puzzlePartTypes(const QString &puzzleType)
{
    const PuzzleInformation *puzzleInfo = mPuzzles[puzzleType];
    if (puzzleInfo)
    {
        return puzzleInfo->getPuzzlePartTypes();
    }
    return QList<QString>();
}

QList<CreatePuzzleTool::PuzzleToolMode> PuzzleTypeManager::puzzleToolModes(const QString &puzzleType)
{
    const PuzzleInformation *puzzleInfo = mPuzzles[puzzleType];
    if (puzzleInfo)
    {
        return puzzleInfo->getPuzzlePartTypeModes();
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

void PuzzleTypeManager::addIdentifier(const QString &puzzleName, const QString &entry, QMap<QString, QString> &identifiers)
{
    QRegularExpressionMatchIterator matchIter = mIdentifierRegex.globalMatch(entry);
    while (matchIter.hasNext())
    {
        QRegularExpressionMatch match = matchIter.next();
        QString id = match.captured(QLatin1Literal("id"));
        if (!identifiers.contains(id))
        {
            QString potentialIdentifier(puzzleName + QLatin1String("_") + id);
            int count = 1;
            while (identifiers.values().contains(potentialIdentifier))
            {
                potentialIdentifier = puzzleName + QLatin1String("_") + id + QString::number(count);
                count++;
            }
            identifiers[id] = potentialIdentifier;
        }
    }
}

QString PuzzleTypeManager::applyEntryNonIdentifiers(const QString &entry, int index, QMap<QString, QList<MapPuzzleModel::PartOrPuzzle *> > &parts)
{
    QString firstPass;
    QTextStream firstPassStream(&firstPass);
    QRegularExpressionMatchIterator indexIter = mIndexRegex.globalMatch(entry);
    int start = 0;
    while (indexIter.hasNext())
    {
        QRegularExpressionMatch match = indexIter.next();
        int length = match.capturedStart() - start;
        firstPassStream << entry.mid(start, length);
        firstPassStream << index;
        start = match.capturedEnd();
    }
    firstPassStream << entry.mid(start);
    QString finalPass;
    QTextStream finalPassStream(&finalPass);
    QRegularExpressionMatchIterator countIter = mCountRegex.globalMatch(firstPass);
    start = 0;
    while (countIter.hasNext())
    {
        QRegularExpressionMatch match = countIter.next();
        int length = match.capturedStart() - start;
        finalPassStream << entry.mid(start, length);
        int count = parts[match.captured(QLatin1Literal("partType"))].length();
        finalPassStream << count;
        start = match.capturedEnd();
    }
    finalPassStream << firstPass.mid(start);
    return finalPass;
}

QString PuzzleTypeManager::applyIdentifiers(const QString &entry, QMap<QString, QString> &identifiers)
{
    QString identified;
    QTextStream identifiedStream(&identified);
    QRegularExpressionMatchIterator matchIter = mIdentifierRegex.globalMatch(entry);
    int start = 0;
    while (matchIter.hasNext())
    {
        QRegularExpressionMatch match = matchIter.next();
        QString id = match.captured(QLatin1Literal("id"));
        int length = match.capturedStart() - start;
        identifiedStream << entry.mid(start, length);
        identifiedStream << identifiers[id];
        start = match.capturedEnd();
    }
    identifiedStream << entry.mid(start);
    return identified;
}

PuzzleTypeManager::PuzzleInformation::PuzzleInformation(const QString &puzzleName)
    : mName(puzzleName)
{
}

void PuzzleTypeManager::PuzzleInformation::addPuzzlePartType(const QString &puzzlePartType)
{
    mPuzzlePartTypes.append(puzzlePartType);
}

void PuzzleTypeManager::PuzzleInformation::addPuzzlePartEntry(const QString &puzzlePartType, const QString &entryName, const QString &entryValue)
{
    (mPuzzleParts[puzzlePartType])[entryName] = entryValue;
}

void PuzzleTypeManager::PuzzleInformation::addMinimum(const QString &puzzlePartType, int minimum)
{
    mMinimums[puzzlePartType] = minimum;
}

void PuzzleTypeManager::PuzzleInformation::addMaximum(const QString &puzzlePartType, int maximum)
{
    mMaximums[puzzlePartType] = maximum;
}

void PuzzleTypeManager::PuzzleInformation::setPartTypeMode(const QString &puzzlePartType, CreatePuzzleTool::PuzzleToolMode mode)
{
    mModes[puzzlePartType] = mode;
}

const QList<QString> &PuzzleTypeManager::PuzzleInformation::getPuzzlePartTypes() const
{
    return mPuzzlePartTypes;
}

const QMap<QString, QString> &PuzzleTypeManager::PuzzleInformation::getPuzzlePartEntries(const QString &puzzlePartType) const
{
    if (mPuzzleParts.contains(puzzlePartType))
    {
        return mPuzzleParts[puzzlePartType];
    }
    return mDefaultEntries;
}

bool PuzzleTypeManager::PuzzleInformation::isCountAllowed(const QString &puzzlePartType, int count) const
{
    if (mMinimums.contains(puzzlePartType) && count < mMinimums[puzzlePartType])
    {
        return false;
    }
    if (mMaximums.contains(puzzlePartType) && count > mMaximums[puzzlePartType])
    {
        return false;
    }
    return true;
}

const QList<CreatePuzzleTool::PuzzleToolMode> &PuzzleTypeManager::PuzzleInformation::getPuzzlePartTypeModes() const
{
    return mModesList;
}

void PuzzleTypeManager::PuzzleInformation::finalize()
{
    for (QString partType : mPuzzlePartTypes)
    {
        mModesList.append(mModes[partType]);
    }
}

PuzzleTypeManager::PuzzleInformation *PuzzleTypeManager::PuzzleInformation::fromJson(const QJsonObject &puzzleInfoData)
{
    PuzzleInformation *puzzleInfo = new PuzzleInformation(puzzleInfoData[QLatin1Literal("puzzleName")].toString());

    for (const QJsonValue &value : puzzleInfoData[QLatin1Literal("puzzlePartTypes")].toArray())
    {
        QJsonObject partTypeData = value.toObject();
        QString partName = partTypeData[QLatin1Literal("partName")].toString();
        puzzleInfo->addPuzzlePartType(partName);
        QString partTypeString = partTypeData[QLatin1Literal("partType")].toString();
        if (partTypeString == QLatin1Literal("newPuzzle"))
        {
            puzzleInfo->setPartTypeMode(partName, CreatePuzzleTool::PuzzleToolMode::CreateNewPuzzle);
        }
        else if (partTypeString == QLatin1Literal("newPart"))
        {
            puzzleInfo->setPartTypeMode(partName, CreatePuzzleTool::PuzzleToolMode::CreatePuzzlePart);
        }
        else if (partTypeString == QLatin1Literal("applyProperty"))
        {
            puzzleInfo->setPartTypeMode(partName, CreatePuzzleTool::PuzzleToolMode::ApplyProperty);
        }
        if (partTypeData.contains(QLatin1Literal("maximum")))
        {
            puzzleInfo->addMaximum(partName, partTypeData[QLatin1Literal("maximum")].toInt());
        }
        if (partTypeData.contains(QLatin1Literal("minimum")))
        {
            puzzleInfo->addMinimum(partName, partTypeData[QLatin1Literal("minimum")].toInt());
        }
        QJsonObject partEntries = partTypeData[QLatin1Literal("partEntries")].toObject();
        QJsonObject::const_iterator it;
        for (it = partEntries.constBegin(); it != partEntries.end(); it++)
        {
            QString entryName = it.key();
            QString entryValue = it.value().toString();
            puzzleInfo->addPuzzlePartEntry(partName, entryName, entryValue);
        }
    }

    puzzleInfo->finalize();
    return puzzleInfo;
}
