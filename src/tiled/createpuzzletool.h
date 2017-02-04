#ifndef CREATEPUZZLETOOL_H
#define CREATEPUZZLETOOL_H

#include "createtileobjecttool.h"

#include "addpuzzledialog.h"

namespace Tiled {

namespace Internal {

class MapDocument;

}

namespace Custom {

class PuzzleTypeDock;

class CreatePuzzleTool : public Tiled::Internal::CreateTileObjectTool
{
    Q_OBJECT
public:
    enum PuzzleToolMode { CreateNewPuzzle, CreatePuzzlePart, ApplyProperty };

    class TileSelectedObserver
    {
    public:
        virtual void selected(Internal::MapDocument *document, MapObject *mapObject, const QString &puzzleName) = 0;

        virtual PuzzleToolMode getMode() = 0;
    };

    CreatePuzzleTool(QObject *parent, PuzzleTypeDock *typeDock);

    void setNewMode(PuzzleToolMode newMode, TileSelectedObserver *tileSelectedObserver);

    void activate(Tiled::Internal::MapScene *scene) override;

    void languageChanged() override;

    void startNewMapObject(const QPointF &pos, ObjectGroup *objectGroup) override;

    void keyPressed(QKeyEvent *event) override;

    void mouseMoved(const QPointF &pos,
                    Qt::KeyboardModifiers modifiers) override;
    void mousePressed(QGraphicsSceneMouseEvent *event) override;
    void mouseReleased(QGraphicsSceneMouseEvent *event) override;

protected:
    MapObject *createNewMapObject() override;
    void cancelNewMapObject() override;
    void finishNewMapObject() override;

    virtual void applyPropertyMousePressed(QGraphicsSceneMouseEvent *event);

private:
    void changeMode(PuzzleToolMode newMode);
    void moveToNextTool();
    void refreshCursor();

    PuzzleTypeDock *mTypeDock;
    QString mPuzzleType;
    QString mCurrentPuzzleName;

    PuzzleToolMode mMode;
    TileSelectedObserver *mSelectedObserver;
};

}

}
#endif // CREATEPUZZLETOOL_H
