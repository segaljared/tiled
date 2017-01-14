#include "addpuzzledialog.h"
#include "ui_addpuzzledialog.h"

#include <QMetaEnum>

AddPuzzleDialog::AddPuzzleDialog(QWidget *parent) :
    QDialog(parent),
    mUI(new Ui::AddPuzzleDialog)
{
    mUI->setupUi(this);

    QMetaEnum metaEnum = QMetaEnum::fromType<AddPuzzleDialog::PuzzleType>();
    for(int i = 0; i < metaEnum.keyCount(); i++)
    {
        mUI->puzzleType->addItem(tr(metaEnum.key(i)), metaEnum.value(i));
    }
}

AddPuzzleDialog::~AddPuzzleDialog()
{
    delete mUI;
}

AddPuzzleDialog::PuzzleType AddPuzzleDialog::puzzleType() const
{
    return static_cast<PuzzleType>(mUI->puzzleType->currentData().toInt());
}
