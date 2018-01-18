#include "puzzletypemanager.h"

#include "changeproperties.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mappuzzlemodel.h"

#include <QDir>
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
      mIdentifierListRegex(QLatin1Literal("\\{ids:(?<id>[^\\s\\{\\}]+)\\}")),
      mCountRegex(QLatin1Literal("\\{count:(?<partType>[^\\s\\{\\}]+)\\}")),
      mIndexRegex(QLatin1Literal("\\{index\\}")),
      mResourceRegex(QLatin1Literal("\\{res\\((?<id>[^\\s\\{\\}\\[\\]]+)\\)(\\[(?<index>[0-9]+)\\]:(?<keyOrValue>key|value)|(?<key>[^\\s\\{\\}]+))\\}")),
      mRepeatRegex(QLatin1Literal("\\{repeat:(?<id>[0-9a-zA-Z]+):(?<count>[0-9]+)(:(?<separator>[^\\}]+))?\\}(?<content>.+)\\{endrepeat:\\g{id}\\}")),
      mRepeatIndexRegex(QLatin1Literal("\\{index:(?<id>[0-9a-zA-Z]+)\\}"))
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
    QDir directory(path);
    QString absolutePath = directory.absolutePath();
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
        QString puzzleDataString = QString::fromUtf8(puzzleData);
        QJsonDocument puzzleDocument(QJsonDocument::fromJson(puzzleData));
        for(const QJsonValue &value : puzzleDocument.array())
        {
            QJsonObject puzzleInfoData = value.toObject();
            PuzzleInformation *puzzleInfo = PuzzleInformation::fromJson(puzzleInfoData);
            mPuzzles[puzzleInfo->getName()] = puzzleInfo;
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
        QMap<QString, QList<QString>> identifiers;
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
                QString identifierAdded;
                while (entryIter.hasNext())
                {
                    entryIter.next();
                    QString entry = applyEntryNonIdentifiers(entryIter.value(), index, parts);
                    entry = addIdentifier(puzzle->mName, entry, part->mPuzzlePartType, identifiers, identifierAdded);
                    entry = applyLoops(entry);
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
                QString entry = applyIdentifiers(propIter.value(), identifiers);
                undo->push(new SetProperty(mapDocument,
                                           QList<Object*>() << partIter.key()->mPart,
                                           propIter.key(),
                                           applyResources(puzzleInfo, entry)));
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

QString PuzzleTypeManager::addIdentifier(const QString &puzzleName, const QString &entry, const QString &partType, QMap<QString, QList<QString> > &identifiers, QString &identifierAdded)
{
    QRegularExpressionMatchIterator matchIter = mIdentifierRegex.globalMatch(entry);
    QString newEntry;
    QTextStream newEntryStream(&newEntry);
    int start = 0;
    while (matchIter.hasNext())
    {
        QRegularExpressionMatch match = matchIter.next();
        QString id = match.captured(QLatin1Literal("id"));
        int length = match.capturedStart() - start;
        newEntryStream << entry.mid(start, length);
        if (id.compare(QLatin1Literal("this")) == 0)
        {
            if (identifierAdded.length() == 0)
            {
                QString potentialIdentifier(puzzleName + QLatin1String("_") + partType);
                int count = 1;
                while (identifiers[partType].contains(potentialIdentifier))
                {
                    potentialIdentifier = puzzleName + QLatin1String("_") + partType + QString::number(count);
                    count++;
                }
                identifiers[partType].append(potentialIdentifier);
                identifierAdded = potentialIdentifier;
            }
            newEntryStream << identifierAdded;
            start = match.capturedEnd();
        }
        else
        {
            start = match.capturedStart();
        }
    }
    newEntryStream << entry.mid(start);
    return newEntry;
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

QString PuzzleTypeManager::applyIdentifiers(const QString &entry, QMap<QString, QList<QString> > &identifiers)
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
        Q_ASSERT(identifiers[id].length() == 1);
        identifiedStream << identifiers[id].first();
        start = match.capturedEnd();
    }
    identifiedStream << entry.mid(start);
    QString listPass;
    QTextStream listPassStream(&listPass);
    QRegularExpressionMatchIterator listIter = mIdentifierListRegex.globalMatch(identified);
    start = 0;
    while (listIter.hasNext())
    {
        QRegularExpressionMatch match = listIter.next();
        QString ids = match.captured(QLatin1Literal("ids"));
        int length = match.capturedStart() - start;
        listPassStream << entry.mid(start, length);
        for (int i = 0; i < identifiers[ids].length(); i++)
        {
            listPassStream << identifiers[ids].at(i);
            if (i < identifiers[ids].length() - 1)
            {
                listPassStream << QLatin1Char(',');
            }
        }
        start = match.capturedEnd();
    }
    listPassStream << identified.mid(start);
    return listPass;
}

QString PuzzleTypeManager::applyLoops(const QString &entry)
{
    QRegularExpressionMatch repeatMatch = mRepeatRegex.match(entry);
    QString lastRepeat = entry;
    while (repeatMatch.hasMatch())
    {
        QString repeated;
        QTextStream repeatedStream(&repeated);
        repeatedStream << lastRepeat.mid(0, repeatMatch.capturedStart());
        QString repeatId = repeatMatch.captured(QLatin1Literal("id"));
        int count = repeatMatch.captured(QLatin1Literal("count")).toInt();
        QString content = repeatMatch.captured(QLatin1Literal("content"));
        QString separator = repeatMatch.captured(QLatin1Literal("separator"));
        if (separator.isEmpty())
        {
            separator = QLatin1Literal("");
        }
        for (int index = 0; index < count; index++)
        {
            QRegularExpressionMatchIterator indexIter = mRepeatIndexRegex.globalMatch(content);
            QString iteration;
            QTextStream iterationStream(&iteration);
            int start = 0;
            while (indexIter.hasNext())
            {
                QRegularExpressionMatch indexMatch = indexIter.next();
                int length = indexMatch.capturedStart() - start;
                iterationStream << content.mid(start, length);
                QString indexId = indexMatch.captured(QLatin1Literal("id"));
                if (indexId.compare(repeatId) == 0)
                {
                    iterationStream << index;
                    start = indexMatch.capturedEnd();
                }
                else
                {
                    start = indexMatch.capturedStart();
                }
            }
            iterationStream << content.mid(start);
            repeatedStream << iteration;
            if (index < count - 1)
            {
                repeatedStream << separator;
            }
        }
        repeatedStream << lastRepeat.mid(repeatMatch.capturedEnd());
        repeatMatch = mRepeatRegex.match(repeated);
        lastRepeat = repeated;
    }
    return lastRepeat;
}

QString PuzzleTypeManager::applyResources(const PuzzleInformation *puzzleInfo, const QString &entry)
{
    QString resourced;
    QTextStream resourcedStream(&resourced);
    QRegularExpressionMatchIterator matchIter = mResourceRegex.globalMatch(entry);
    int start = 0;
    while (matchIter.hasNext())
    {
        QRegularExpressionMatch match = matchIter.next();
        int length = match.capturedStart() - start;
        resourcedStream << entry.mid(start, length);

        QString id = match.captured(QLatin1Literal("id"));
        QString indexString = match.captured(QLatin1Literal("index"));
        if (!indexString.isEmpty())
        {
            int index = indexString.toInt();
            QString keyOrValue = match.captured(QLatin1Literal("keyOrValue"));
            if (keyOrValue.compare(QLatin1Literal("key")))
            {
                resourcedStream << puzzleInfo->getResource(id).keys().at(index);
            }
            else
            {
                resourcedStream << puzzleInfo->getResource(id).values().at(index);
            }
        }
        else
        {
            QString key = match.captured(QLatin1Literal("key"));
            resourcedStream << (puzzleInfo->getResource(id))[key];
        }
        start = match.capturedEnd();
    }
    resourcedStream << entry.mid(start);
    return resourced;
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

const QMap<QString, QString> &PuzzleTypeManager::PuzzleInformation::getResource(const QString &resourceId) const
{
    return mResources[resourceId];
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

    for (QJsonObject::const_iterator iter = puzzleInfoData.constBegin(); iter != puzzleInfoData.end(); iter++)
    {
        QString resourceName = iter.key();
        if (resourceName.compare(QLatin1Literal("puzzleName")) == 0 || resourceName.compare(QLatin1Literal("puzzlePartTypes")) == 0)
        {
            continue;
        }
        QJsonObject resources = iter.value().toObject();
        QJsonObject::const_iterator resourceIter;
        for (resourceIter = resources.constBegin(); resourceIter != resources.end(); resourceIter++)
        {
            QString entryName = resourceIter.key();
            QString entryValue = resourceIter.value().toString();
            (puzzleInfo->mResources[resourceName])[entryName] = entryValue;
        }
    }

    puzzleInfo->finalize();
    return puzzleInfo;
}
