#ifndef CREATEPUZZLETOOL_H
#define CREATEPUZZLETOOL_H

#include "createtileobjecttool.h"

#include "addpuzzledialog.h"

namespace Tiled {

namespace Custom {

class PuzzleTypeDock;

class CreatePuzzleTool : public Tiled::Internal::CreateTileObjectTool
{
    Q_OBJECT
public:
    typedef void (*tileSelected)(MapObject *);

    enum PuzzleToolMode { Create, ApplyProperty };

    CreatePuzzleTool(QObject *parent, PuzzleTypeDock *typeDock);

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
    virtual void applyPropertyMousePressed(QGraphicsSceneMouseEvent *event);

private:
    void changeMode(PuzzleToolMode newMode);
    void refreshCursor();

    PuzzleTypeDock *mTypeDock;
    AddPuzzleDialog::PuzzleType mPuzzleType;

    PuzzleToolMode mMode;
    tileSelected mSelected;
};

}

}
#endif // CREATEPUZZLETOOL_H
