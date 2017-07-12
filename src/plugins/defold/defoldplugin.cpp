/*
 * Defold Tiled Plugin
 * Copyright 2016, Nikita Razdobreev <exzo0mex@gmail.com>
 * Copyright 2016, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#include "defoldplugin.h"

#include "tokendefines.h"

#include "layer.h"
#include "map.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "savefile.h"
#include "tile.h"
#include "tilelayer.h"

#include <QTextStream>

#include <cmath>

namespace Defold {

static QString replaceTags(QString context, const QVariantHash &map)
{
    QHashIterator<QString,QVariant> it{map};
    while (it.hasNext()) {
        it.next();
        context.replace(QLatin1String("{{") + it.key() + QLatin1String("}}"),
                        it.value().toString());
    }
    return context;
}

QStringList DefoldPlugin::outputFiles(const Tiled::Map *, const QString &fileName) const
{
    return QStringList() << fileName;
}

QString DefoldPlugin::nameFilter() const
{
    return tr("Defold files (*.tilemap)");
}

QString DefoldPlugin::shortName() const
{
    return QLatin1String("defold");
}

QString DefoldPlugin::errorString() const
{
    return mError;
}

DefoldPlugin::DefoldPlugin()
{
}

bool DefoldPlugin::write(const Tiled::Map *map, const QString &fileName)
{
    QVariantHash map_h;

    QList<QList<QString>> types;

    int layerWidth = 0;
    int layerHeight = 0;

    QString layers = "";
    foreach (Tiled::TileLayer *tileLayer, map->tileLayers()) {
        QVariantHash layer_h;
        layer_h["id"] = tileLayer->name();
        layer_h["z"] = 0;
        layer_h["is_visible"] = tileLayer->isVisible() ? 1 : 0;
        QString cells = "";

        layerWidth = std::max(tileLayer->width(), layerWidth);
        layerHeight = std::max(tileLayer->height(), layerHeight);

        for (int x = 0; x < tileLayer->width(); ++x) {
            QList<QString> t;
            if (types.size() < tileLayer->width())
                types.append(t);
            for (int y = 0; y < tileLayer->height(); ++y) {
                const Tiled::Cell &cell = tileLayer->cellAt(x, y);
                if (cell.isEmpty())
                    continue;
                QVariantHash cell_h;
                cell_h["x"] = x;
                cell_h["y"] = tileLayer->height() - y - 1;
                cell_h["tile"] = cell.tileId();
                cell_h["h_flip"] = cell.flippedHorizontally() ? 1 : 0;
                cell_h["v_flip"] = cell.flippedVertically() ? 1 : 0;
                cells.append(replaceTags(QLatin1String(cell_t), cell_h));
                if (const Tiled::Tile *tile = cell.tile()) {
                    if (types[x].size() < tileLayer->height())
                        types[x].append(tile->property("Type").toString());
                    else if (!tile->property("Type").toString().isEmpty())
                        types[x][tileLayer->height() - y - 1] = tile->property("Type").toString();
                }
            }
        }
        layer_h["cells"] = cells;
        layers.append(replaceTags(QLatin1String(layer_t), layer_h));
    }
    map_h["layers"] = layers;
    map_h["material"] = "/builtins/materials/tile_map.material";
    map_h["blend_mode"] = "BLEND_MODE_ALPHA";
    map_h["tile_set"] = "";

    QString result = replaceTags(QLatin1String(map_t), map_h);
    Tiled::SaveFile mapFile(fileName);
    if (!mapFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = tr("Could not open file for writing.");
        return false;
    }
    QTextStream stream(mapFile.device());
    stream << result;

    if (mapFile.error() != QFileDevice::NoError) {
        mError = mapFile.errorString();
        return false;
    }

    if (!mapFile.commit()) {
        mError = mapFile.errorString();
        return false;
    }

    return true;
}

} // namespace Defold
