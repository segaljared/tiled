#ifndef PUZZLETYPEDOCK_H
#define PUZZLETYPEDOCK_H

#include <QDockWidget>
#include "createpuzzletool.h"

namespace Tiled {
namespace Custom {

class PuzzleTypeDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit PuzzleTypeDock(QWidget *parent = nullptr);

    CreatePuzzleTool::tileSelected getNextTool();
};

}
}
#endif // PUZZLETYPEDOCK_H
