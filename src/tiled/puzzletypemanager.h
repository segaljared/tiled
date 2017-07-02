#ifndef PUZZLETYPEMANAGER_H
#define PUZZLETYPEMANAGER_H

#include "createpuzzletool.h"
#include "mappuzzlemodel.h"

#include <QObject>
#include <QList>

namespace Tiled {

namespace Internal {

class MapDocument;

}

namespace Custom {

class PuzzleTypeManager : public QObject
{
    Q_OBJECT
public:
    static PuzzleTypeManager *instance();

    static void deleteInstance();

    void initialize(const QString &path);

    const QList<QString> &puzzleTypes();

    void puzzleChanged(MapPuzzleModel::PartOrPuzzle *puzzle, Internal::MapDocument *mapDocument);

    QList<QString> puzzlePartTypes(const QString &puzzleType);

    QList<CreatePuzzleTool::PuzzleToolMode> puzzleToolModes(const QString &puzzleType);

    CreatePuzzleTool::PuzzleToolMode puzzleToolMode(const QString &puzzleType, const QString &puzzlePartType);

private:
    class PuzzleInformation
    {
    public:
        PuzzleInformation(const QString &puzzleName);

        void addPuzzlePartType(const QString &puzzlePartType);

        void addPuzzlePartEntry(const QString &puzzlePartType, const QString &entryName, const QString& entryValue);

        void addMinimum(const QString &puzzlePartType, int minimum);

        void addMaximum(const QString &puzzlePartType, int maximum);

        void setPartTypeMode(const QString &puzzlePartType, CreatePuzzleTool::PuzzleToolMode mode);

        QString getName() const { return mName; }

        const QList<QString> &getPuzzlePartTypes() const;

        const QMap<QString, QString> &getPuzzlePartEntries(const QString &puzzlePartType) const;

        bool isCountAllowed(const QString& puzzlePartType, int count) const;

        const QList<CreatePuzzleTool::PuzzleToolMode> &getPuzzlePartTypeModes() const;

        void finalize();

        static PuzzleInformation *fromJson(const QJsonObject& puzzleInfoData);

    private:
        QString mName;
        mutable QMap<QString, QMap<QString, QString>> mPuzzleParts;
        QMap<QString, int> mMinimums;
        QMap<QString, int> mMaximums;
        QMap<QString, CreatePuzzleTool::PuzzleToolMode> mModes;
        QList<QString> mPuzzlePartTypes;
        QList<CreatePuzzleTool::PuzzleToolMode> mModesList;
        QMap<QString, QString> mDefaultEntries;
    };

    PuzzleTypeManager();

    static PuzzleTypeManager *mInstance;

    void addIdentifier(const QString &puzzleName, const QString &entry, QMap<QString, QString> &identifiers);

    QString applyEntryNonIdentifiers(const QString &entry, int index, QMap<QString, QList<MapPuzzleModel::PartOrPuzzle*>> &parts);

    QString applyIdentifiers(const QString &entry, QMap<QString, QString> &identifiers);

    QList<QString> mPuzzleTypeNames;
    QMap<QString, const PuzzleInformation*> mPuzzles;
    QRegularExpression mIdentifierRegex;
    QRegularExpression mIdentifierListRegex;
    QRegularExpression mCountRegex;
    QRegularExpression mIndexRegex;
};

}

}

#endif // PUZZLETYPEMANAGER_H
