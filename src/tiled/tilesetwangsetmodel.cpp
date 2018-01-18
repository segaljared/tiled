/*
 * tilesetwangsetmodel.cpp
 * Copyright 2017, Benjamin Trotter <bdtrotte@ucsc.edu>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tilesetwangsetmodel.h"

#include "tilesetdocument.h"
#include "renamewangset.h"
#include "wangset.h"
#include "tileset.h"
#include "tile.h"

using namespace Tiled;
using namespace Internal;

TilesetWangSetModel::TilesetWangSetModel(TilesetDocument *mapDocument,
                                         QObject *parent):
    QAbstractListModel(parent),
    mTilesetDocument(mapDocument)
{
}

TilesetWangSetModel::~TilesetWangSetModel()
{
}

QModelIndex TilesetWangSetModel::index(WangSet *wangSet)
{
    Tileset *tileset = wangSet->tileset();
    int row = tileset->wangSets().indexOf(wangSet);

    return index(row, 0);
}

int TilesetWangSetModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return mTilesetDocument->tileset()->wangSetCount();

    return 0;
}

int TilesetWangSetModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}


QVariant TilesetWangSetModel::data(const QModelIndex &index, int role) const
{
    if (WangSet *wangSet = wangSetAt(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return wangSet->name();
        case Qt::DecorationRole:
            if (Tile *imageTile = wangSet->imageTile())
                return imageTile->image();
            break;
        case WangSetRole:
            return QVariant::fromValue(wangSet);
        }
    }

    return QVariant();
}

bool TilesetWangSetModel::setData(const QModelIndex &index,
                                  const QVariant &value,
                                  int role)
{
    if (role == Qt::EditRole) {
        const QString newName = value.toString();
        WangSet *wangSet = wangSetAt(index);
        if (wangSet->name() != newName) {
            RenameWangSet *rename = new RenameWangSet(mTilesetDocument,
                                                      mTilesetDocument->tileset()->wangSets().indexOf(wangSet),
                                                      newName);
            mTilesetDocument->undoStack()->push(rename);
        }
        return true;
    }

    return false;
}

Qt::ItemFlags TilesetWangSetModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractItemModel::flags(index);
    if (index.isValid())  // can edit wangSet names
        rc |= Qt::ItemIsEditable;
    return rc;
}

WangSet *TilesetWangSetModel::wangSetAt(const QModelIndex &index) const
{
    if (index.isValid())
        return mTilesetDocument->tileset()->wangSet(index.row());

    return nullptr;
}

void TilesetWangSetModel::insertWangSet(int index, WangSet *wangSet)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    int row = tileset->wangSetCount();

    emit wangSetAboutToBeAdded(tileset);

    beginInsertRows(QModelIndex(), row, row);
    tileset->insertWangSet(index, wangSet);
    endInsertRows();

    emit wangSetAdded(tileset);
}

WangSet *TilesetWangSetModel::takeWangSetAt(int index)
{
    Tileset *tileset = mTilesetDocument->tileset().data();

    emit wangSetAboutToBeRemoved(tileset->wangSet(index));

    beginRemoveRows(QModelIndex(), index, index);
    WangSet *wangSet = tileset->takeWangSetAt(index);
    endRemoveRows();

    emit wangSetRemoved(wangSet);

    return wangSet;
}

void TilesetWangSetModel::setWangSetName(int index, const QString &name)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);
    wangSet->setName(name);
    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::setWangSetEdges(int index, int value)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);
    wangSet->setEdgeColorCount(value);
    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::setWangSetCorners(int index, int value)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);
    wangSet->setCornerColorCount(value);
    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::setWangSetImage(int index, int tileId)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);
    wangSet->setImageTileId(tileId);
    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::insertWangColor(int index, QSharedPointer<WangColor> wangColor)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);
    wangSet->insertWangColor(wangColor);
    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::removeWangColorAt(int index, int color, bool isEdge)
{
    Tileset *tileset = mTilesetDocument->tileset().data();
    WangSet *wangSet = tileset->wangSet(index);

    if (isEdge && wangSet->edgeColorCount() == 2)
        wangSet->setEdgeColorCount(1);
    else if (!isEdge && wangSet->cornerColorCount() == 2)
        wangSet->setCornerColorCount(1);
    else
        wangSet->removeWangColorAt(color, isEdge);

    emitWangSetChange(wangSet);
}

void TilesetWangSetModel::emitWangSetChange(WangSet *wangSet)
{
    const QModelIndex index = TilesetWangSetModel::index(wangSet);
    emit dataChanged(index, index);
    emit wangSetChanged(wangSet->tileset(), index.row());
}
