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
    PuzzleTypeManager();

    static PuzzleTypeManager *mInstance;

    QList<QString> mPuzzleTypeNames;
};

}

}

#endif // PUZZLETYPEMANAGER_H
