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
    CreatePuzzleTool(QObject *parent, PuzzleTypeDock *typeDock);

    void activate(Tiled::Internal::MapScene *scene) override;

    void languageChanged() override;

    void startNewMapObject(const QPointF &pos, ObjectGroup *objectGroup) override;

protected:

    MapObject *createNewMapObject() override;

private:

    PuzzleTypeDock *mTypeDock;
    AddPuzzleDialog::PuzzleType mPuzzleType;
};

}

}
#endif // CREATEPUZZLETOOL_H
