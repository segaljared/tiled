#include "puzzletypedock.h"

#include "changeproperties.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mappuzzlemodel.h"
#include "puzzletypemanager.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QUndoStack>

#include <qstring.h>

using namespace Tiled;
using namespace Tiled::Custom;
using namespace Tiled::Internal;

PuzzleTypeDock::PuzzleTypeDock(QWidget *parent)
    : QDockWidget(parent),
      mButtonGroup(nullptr)
{
    setObjectName(QLatin1String("layerDock"));

    setUpButtonsForType(QLatin1String(""));
}

CreatePuzzleTool::TileSelectedObserver *PuzzleTypeDock::setPuzzleType(const QString &puzzleType)
{
    mPuzzleType = puzzleType;
    mCurrentPuzzlePartType = PuzzleTypeManager::instance()->puzzlePartTypes(puzzleType).first();
    setUpButtonsForType(mPuzzleType);
    CreatePuzzleTool::PuzzleToolMode mode = PuzzleTypeManager::instance()->puzzleToolModes(puzzleType).first();
    return new TypeDockTileSelectedObserver(this, mode);
}

CreatePuzzleTool::TileSelectedObserver *PuzzleTypeDock::getNextTool()
{
    QList<QString> partTypes = PuzzleTypeManager::instance()->puzzlePartTypes(mPuzzleType);
    int index = partTypes.indexOf(mCurrentPuzzlePartType);
    if (partTypes.count() > (index + 1))
    {
        mCurrentPuzzlePartType = partTypes.at(index + 1);
        mButtonGroup->buttons().at(index + 1)->setChecked(true);
        CreatePuzzleTool::PuzzleToolMode mode = PuzzleTypeManager::instance()->puzzleToolModes(mPuzzleType).at(index + 1);
        return new TypeDockTileSelectedObserver(this, mode);
    }
    return nullptr;
}

CreatePuzzleTool::TileSelectedObserver *PuzzleTypeDock::createdNewPuzzle()
{
    QList<QString> partTypes = PuzzleTypeManager::instance()->puzzlePartTypes(mPuzzleType);
    if (partTypes.count() > 1)
    {
        mCurrentPuzzlePartType = partTypes.at(1);
        mButtonGroup->buttons().at(1)->setChecked(true);
        CreatePuzzleTool::PuzzleToolMode mode = PuzzleTypeManager::instance()->puzzleToolModes(mPuzzleType).at(1);
        return new TypeDockTileSelectedObserver(this, mode);
    }
    return nullptr;
}

void PuzzleTypeDock::applyProperty(MapDocument *document, MapObject *mapObject, const QString &puzzleName, bool addImmediately)
{
    QUndoStack *undo = document->undoStack();
    if (addImmediately)
    {
        document->mapPuzzleModel()->startChange();
    }
    undo->beginMacro(tr("Add new puzzle part"));
    undo->push(new SetProperty(document,
                               QList<Object*>() << mapObject,
                               MapPuzzleModel::PUZZLE_PART,
                               puzzleName));
    undo->push(new SetProperty(document,
                               QList<Object*>() << mapObject,
                               MapPuzzleModel::PUZZLE_PART_TYPE,
                               mCurrentPuzzlePartType));
    if (addImmediately)
    {
        document->mapPuzzleModel()->addPuzzlePart(mapObject, puzzleName);
        document->mapPuzzleModel()->endChange();
    }
    undo->endMacro();
}

void PuzzleTypeDock::setUpButtonsForType(const QString &puzzleType)
{
    // clear the widgets
    if (mButtonGroup)
    {
        delete mButtonGroup;
    }
    QWidget *old = widget();
    if (old)
    {
        old->setParent(0);
        delete old;
    }

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setMargin(10);

    QList<QString> types = PuzzleTypeManager::instance()->puzzlePartTypes(puzzleType);
    if (types.length() == 0)
    {
        QLabel *label = new QLabel;
        layout->addWidget(label);
        label->setText(tr("No puzzle types"));
    }
    else
    {
        mButtonGroup = new QButtonGroup();
        mButtonGroup->setExclusive(true);
        bool first = true;
        for(QString partType : types)
        {
            QPushButton *button = new QPushButton(partType);
            layout->addWidget(button);
            button->setCheckable(true);
            mButtonGroup->addButton(button);
            if (first)
            {
                button->setChecked(true);
                first = false;
            }
        }
        connect(mButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
    }

    setWidget(widget);
    setWindowTitle(tr("Puzzles"));
}

void PuzzleTypeDock::buttonClicked(QAbstractButton *button)
{
    int index = mButtonGroup->buttons().indexOf(button);
    QString partType = button->text();
    mCurrentPuzzlePartType = partType;
    if (index == 0 || index == -1)
    {
        CreatePuzzleTool::PuzzleToolMode mode = PuzzleTypeManager::instance()->puzzleToolModes(mPuzzleType).first();
        mPuzzleTool->setNewMode(CreatePuzzleTool::PuzzleToolMode::CreateNewPuzzle, new TypeDockTileSelectedObserver(this, mode));
    }
    else
    {

        CreatePuzzleTool::PuzzleToolMode mode = PuzzleTypeManager::instance()->puzzleToolMode(mPuzzleType, mCurrentPuzzlePartType);
        mPuzzleTool->setNewMode(CreatePuzzleTool::PuzzleToolMode::ApplyProperty, new TypeDockTileSelectedObserver(this, mode));
    }
}

PuzzleTypeDock::TypeDockTileSelectedObserver::TypeDockTileSelectedObserver(PuzzleTypeDock *dock, CreatePuzzleTool::PuzzleToolMode mode)
    : mDock(dock),
      mMode(mode)

{
}

void PuzzleTypeDock::TypeDockTileSelectedObserver::selected(MapDocument *document, MapObject *mapObject, const QString &puzzleName)
{
    mDock->applyProperty(document, mapObject, puzzleName, mMode == CreatePuzzleTool::PuzzleToolMode::ApplyProperty);
}
