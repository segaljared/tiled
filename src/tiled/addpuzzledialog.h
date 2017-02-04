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
    explicit AddPuzzleDialog(QWidget *parent = 0);
    ~AddPuzzleDialog();

    QString puzzleType() const;

private:
    Ui::AddPuzzleDialog *mUI;
};

#endif // ADDPUZZLEDIALOG_H
