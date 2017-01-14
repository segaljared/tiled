#ifndef PUZZLETYPEDOCK_H
#define PUZZLETYPEDOCK_H

#include <QDockWidget>

namespace Tiled {
namespace Custom {

class PuzzleTypeDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit PuzzleTypeDock(QWidget *parent = nullptr);
};

}
}
#endif // PUZZLETYPEDOCK_H
