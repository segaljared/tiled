#include "puzzletypedock.h"

#include <QBoxLayout>
#include <QLabel>

using namespace Tiled;
using namespace Tiled::Custom;

PuzzleTypeDock::PuzzleTypeDock(QWidget *parent)
    : QDockWidget(parent)
{
    setObjectName(QLatin1String("layerDock"));

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setMargin(0);

    QLabel *label = new QLabel;
    layout->addWidget(label);

    setWidget(widget);
    setWindowTitle(tr("Puzzles"));
    label->setText(tr("Test"));
}
