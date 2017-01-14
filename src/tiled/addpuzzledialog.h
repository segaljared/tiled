#ifndef ADDPUZZLEDIALOG_H
#define ADDPUZZLEDIALOG_H

#include <QDialog>

namespace Ui {
class AddPuzzleDialog;
}

class AddPuzzleDialog : public QDialog
{
    Q_OBJECT

public:
    enum PuzzleType { FloorTriggeredDoors };
    Q_ENUM(PuzzleType)

    explicit AddPuzzleDialog(QWidget *parent = 0);
    ~AddPuzzleDialog();

    PuzzleType puzzleType() const;

private:
    Ui::AddPuzzleDialog *mUI;
};

#endif // ADDPUZZLEDIALOG_H
