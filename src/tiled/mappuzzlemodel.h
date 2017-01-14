#ifndef MAPPUZZLEMODEL_H
#define MAPPUZZLEMODEL_H

#include <QAbstractItemModel>
#include "mapobject.h"

namespace Tiled {

namespace Internal {

class MapDocument;

}

namespace Custom {

class MapPuzzleModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    static QString const PUZZLE_PART;
    static QString const PUZZLE_PART_TYPE;
    static QString const PUZZLE_ROOT;

    struct PartOrPuzzle
    {
        PartOrPuzzle(const QString& name)
            : mName(name),
              mPuzzlePartType(tr(""))
        {
            mParts = new QList<PartOrPuzzle*>();
        }

        PartOrPuzzle(MapObject *o, const QString& name)
            : mName(name),
              mPuzzlePartType(tr("")),
              mPart(o)
        {
            if (o->hasProperty(PUZZLE_PART_TYPE))
            {
                mPuzzlePartType = o->propertyAsString(PUZZLE_PART_TYPE);
            }
        }

        ~PartOrPuzzle()
        {
            delete mParts;
        }

        QString mName;
        QList<PartOrPuzzle*>* mParts;
        MapObject *mPart;
        QString mPuzzlePartType;
    };

    MapPuzzleModel(QObject *parent = nullptr);
    ~MapPuzzleModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QModelIndex index(const QString &name) const;
    QModelIndex index(MapObject *o, int column = 0) const;

    PartOrPuzzle *toPartOrPuzzle(const QModelIndex &index) const;
    PartOrPuzzle *toPuzzle(const QModelIndex &index) const;
    MapObject *toPuzzlePart(const QModelIndex &index) const;

    QString puzzleName(const QModelIndex &index) const;
    QString puzzlePartType(const QModelIndex &index) const;

    bool changePuzzleName(PartOrPuzzle* partOrPuzzle, const QString &newName);
    bool createPuzzlePart(MapObject *o, const QString &puzzleName, bool asPuzzleRoot = false);
    bool createPuzzlePart(MapObject *o, const QString &puzzleName, const QString &puzzlePartType, bool asPuzzleRoot = false);
    bool addPuzzlePart(MapObject *o, const QString &puzzleName);
    void removePuzzlePart(MapObject *o);
    bool puzzlePartExists(MapObject *o);

    bool isPuzzleRoot(const QModelIndex &index) const;

    QString getNextPuzzleName() const;

    void setMapDocument(Internal::MapDocument *mapDocument);
    Internal::MapDocument *mapDocument() const { return mMapDocument; }

    const QList<QString> &puzzleNames() const { return mPuzzleNames; }

private slots:
    void objectsAdded(const QList<MapObject*> &objects);
    void objectsRemoved(const QList<MapObject*> &objects);
    void propertyAdded(Object *object, const QString &name);
    void propertyRemoved(Object *object, const QString &name);
    void propertyChanged(Object *object, const QString &name);
    void propertiesChanged(Object *object);

private:
    void updateModelIfNecessary(MapObject *mo);
    void removePuzzlePart(PartOrPuzzle *puzzlePart);
    void internalAddPuzzlePart(MapObject *o, const QString &puzzleName);
    void onPropertyChanged(Object *object, const QString &name);

    Internal::MapDocument *mMapDocument;

    QList<QString> mPuzzleNames;
    QList<PartOrPuzzle*> mFullPuzzles;
    QMap<QString, PartOrPuzzle*> mPuzzles;
    QMap<MapObject*, PartOrPuzzle*> mPuzzleParts;

    QIcon mPuzzleRootIcon;
};

}

}

#endif // MAPPUZZLEMODEL_H
