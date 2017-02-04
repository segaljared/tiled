#include "addpuzzledialog.h"
#include "ui_addpuzzledialog.h"

#include "puzzletypemanager.h"

#include <QMetaEnum>

AddPuzzleDialog::AddPuzzleDialog(QWidget *parent) :
    QDialog(parent),
    mUI(new Ui::AddPuzzleDialog)
{
    mUI->setupUi(this);


    for(QString item : Tiled::Custom::PuzzleTypeManager::instance()->puzzleTypes())
    {
        mUI->puzzleType->addItem(item, item);
    }
}

AddPuzzleDialog::~AddPuzzleDialog()
{
    delete mUI;
}

QString AddPuzzleDialog::puzzleType() const
{
    return mUI->puzzleType->currentData().toString();
}
