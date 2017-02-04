#ifndef PUZZLETYPEDOCK_H
#define PUZZLETYPEDOCK_H

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDockWidget>
#include "createpuzzletool.h"

namespace Tiled {

namespace Internal {

class MapDocument;

}

namespace Custom {

class PuzzleTypeDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit PuzzleTypeDock(QWidget *parent = nullptr);

    CreatePuzzleTool::TileSelectedObserver *setPuzzleType(const QString &puzzleType);

    CreatePuzzleTool::TileSelectedObserver *getNextTool();

    CreatePuzzleTool::TileSelectedObserver *createdNewPuzzle();

    void setPuzzleTool(CreatePuzzleTool *puzzleTool) { mPuzzleTool = puzzleTool; }

private slots:
    void buttonClicked(QAbstractButton *button);

private:
    void applyProperty(Internal::MapDocument *document, MapObject * mapObject, const QString &puzzleName, bool addImmediately);

    void setUpButtonsForType(const QString &puzzleType);

    class TypeDockTileSelectedObserver : public CreatePuzzleTool::TileSelectedObserver
    {
    public:
        TypeDockTileSelectedObserver(PuzzleTypeDock *dock, CreatePuzzleTool::PuzzleToolMode mode);

        void selected(Internal::MapDocument *document, MapObject *mapObject, const QString &puzzleName) override;

        CreatePuzzleTool::PuzzleToolMode getMode() { return mMode; }
    private:
        PuzzleTypeDock *mDock;
        CreatePuzzleTool::PuzzleToolMode mMode;
    };

    QString mPuzzleType;
    QString mCurrentPuzzlePartType;
    QButtonGroup *mButtonGroup;
    CreatePuzzleTool *mPuzzleTool;
};

}
}
#endif // PUZZLETYPEDOCK_H
