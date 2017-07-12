/*
 * exportasimagedialog.cpp
 * Copyright 2009-2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "exportasimagedialog.h"
#include "ui_exportasimagedialog.h"

#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "mapobjectitem.h"
#include "maprenderer.h"
#include "imagelayer.h"
#include "objectgroup.h"
#include "preferences.h"
#include "tilelayer.h"
#include "utils.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QImageWriter>
#include <QSettings>

static const char * const VISIBLE_ONLY_KEY = "SaveAsImage/VisibleLayersOnly";
static const char * const CURRENT_SCALE_KEY = "SaveAsImage/CurrentScale";
static const char * const DRAW_GRID_KEY = "SaveAsImage/DrawGrid";
static const char * const INCLUDE_BACKGROUND_COLOR = "SaveAsImage/IncludeBackgroundColor";

using namespace Tiled;
using namespace Tiled::Internal;

QString ExportAsImageDialog::mPath;

ExportAsImageDialog::ExportAsImageDialog(MapDocument *mapDocument,
                                         const QString &fileName,
                                         qreal currentScale,
                                         QWidget *parent)
    : QDialog(parent)
    , mUi(new Ui::ExportAsImageDialog)
    , mMapDocument(mapDocument)
    , mCurrentScale(currentScale)
{
    mUi->setupUi(this);
    resize(Utils::dpiScaled(size()));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QPushButton *saveButton = mUi->buttonBox->button(QDialogButtonBox::Save);
    saveButton->setText(tr("Export"));

    // Default to the last chosen location
    QString suggestion = mPath;

    // Suggest a nice name for the image
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        const QString path = fileInfo.path();
        const QString baseName = fileInfo.completeBaseName();

        if (suggestion.isEmpty())
            suggestion = path;

        suggestion += QLatin1Char('/');
        suggestion += baseName;
        suggestion += QLatin1String(".png");
    } else {
        suggestion += QLatin1Char('/');
        suggestion += QLatin1String("map.png");
    }

    mUi->fileNameEdit->setText(suggestion);

    // Restore previously used settings
    QSettings *s = Preferences::instance()->settings();
    const bool visibleLayersOnly =
            s->value(QLatin1String(VISIBLE_ONLY_KEY), true).toBool();
    const bool useCurrentScale =
            s->value(QLatin1String(CURRENT_SCALE_KEY), true).toBool();
    const bool drawTileGrid =
            s->value(QLatin1String(DRAW_GRID_KEY), false).toBool();
    const bool includeBackgroundColor =
            s->value(QLatin1String(INCLUDE_BACKGROUND_COLOR), false).toBool();

    mUi->visibleLayersOnly->setChecked(visibleLayersOnly);
    mUi->currentZoomLevel->setChecked(useCurrentScale);
    mUi->drawTileGrid->setChecked(drawTileGrid);
    mUi->includeBackgroundColor->setChecked(includeBackgroundColor);

    connect(mUi->browseButton, SIGNAL(clicked()), SLOT(browse()));
    connect(mUi->fileNameEdit, SIGNAL(textChanged(QString)),
            this, SLOT(updateAcceptEnabled()));


    Utils::restoreGeometry(this);
}

ExportAsImageDialog::~ExportAsImageDialog()
{
    Utils::saveGeometry(this);
    delete mUi;
}

static bool objectLessThan(const MapObject *a, const MapObject *b)
{
    return a->y() < b->y();
}

static bool smoothTransform(qreal scale)
{
    return scale != qreal(1) && scale < qreal(2);
}

void ExportAsImageDialog::accept()
{
    const QString fileName = mUi->fileNameEdit->text();
    if (fileName.isEmpty())
        return;

    if (QFile::exists(fileName)) {
        const QMessageBox::StandardButton button =
                QMessageBox::warning(this,
                                     tr("Export as Image"),
                                     tr("%1 already exists.\n"
                                        "Do you want to replace it?")
                                     .arg(QFileInfo(fileName).fileName()),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);

        if (button != QMessageBox::Yes)
            return;
    }

    const bool visibleLayersOnly = mUi->visibleLayersOnly->isChecked();
    const bool useCurrentScale = mUi->currentZoomLevel->isChecked();
    const bool drawTileGrid = mUi->drawTileGrid->isChecked();
    const bool includeBackgroundColor = mUi->includeBackgroundColor->isChecked();

    MapRenderer *renderer = mMapDocument->renderer();

    // Remember the current render flags
    const Tiled::RenderFlags renderFlags = renderer->flags();

    renderer->setFlag(ShowTileObjectOutlines, false);

    QSize mapSize = renderer->mapSize();

    QMargins margins = mMapDocument->map()->computeLayerOffsetMargins();
    mapSize.setWidth(mapSize.width() + margins.left() + margins.right());
    mapSize.setHeight(mapSize.height() + margins.top() + margins.bottom());

    if (useCurrentScale)
        mapSize *= mCurrentScale;

    QImage image;

    try {
        image = QImage(mapSize, QImage::Format_ARGB32_Premultiplied);

        if (includeBackgroundColor) {
            if (mMapDocument->map()->backgroundColor().isValid())
                image.fill(mMapDocument->map()->backgroundColor());
            else
                image.fill(Qt::gray);
        } else {
            image.fill(Qt::transparent);
        }
    } catch (const std::bad_alloc &) {
        QMessageBox::critical(this,
                              tr("Out of Memory"),
                              tr("Could not allocate sufficient memory for the image. "
                                 "Try reducing the zoom level or using a 64-bit version of Tiled."));
        return;
    }

    if (image.isNull()) {
        const size_t gigabyte = 1073741824;
        const size_t memory = size_t(mapSize.width()) * size_t(mapSize.height()) * 4;
        const double gigabytes = (double) memory / gigabyte;

        QMessageBox::critical(this,
                              tr("Image too Big"),
                              tr("The resulting image would be %1 x %2 pixels and take %3 GB of memory. "
                                 "Tiled is unable to create such an image. Try reducing the zoom level.")
                              .arg(mapSize.width())
                              .arg(mapSize.height())
                              .arg(gigabytes, 0, 'f', 2));
        return;
    }

    QPainter painter(&image);

    if (useCurrentScale) {
        if (smoothTransform(mCurrentScale))
            painter.setRenderHints(QPainter::SmoothPixmapTransform);

        painter.setTransform(QTransform::fromScale(mCurrentScale,
                                                   mCurrentScale));
        renderer->setPainterScale(mCurrentScale);
    } else {
        renderer->setPainterScale(1);
    }

    painter.translate(margins.left(), margins.top());

    LayerIterator iterator(mMapDocument->map());
    while (Layer *layer = iterator.next()) {
        if (visibleLayersOnly && layer->isHidden())
            continue;

        const auto offset = layer->totalOffset();

        painter.setOpacity(layer->effectiveOpacity());
        painter.translate(offset);

        switch (layer->layerType()) {
        case Layer::TileLayerType: {
            const TileLayer *tileLayer = static_cast<const TileLayer*>(layer);
            renderer->drawTileLayer(&painter, tileLayer);
            break;
        }

        case Layer::ObjectGroupType: {
            const ObjectGroup *objectGroup = static_cast<const ObjectGroup*>(layer);
            QList<MapObject*> objects = objectGroup->objects();

            if (objectGroup->drawOrder() == ObjectGroup::TopDownOrder)
                qStableSort(objects.begin(), objects.end(), objectLessThan);

            foreach (const MapObject *object, objects) {
                if (object->isVisible()) {
                    if (object->rotation() != qreal(0)) {
                        QPointF origin = renderer->pixelToScreenCoords(object->position());
                        painter.save();
                        painter.translate(origin);
                        painter.rotate(object->rotation());
                        painter.translate(-origin);
                    }

                    const QColor color = MapObjectItem::objectColor(object);
                    renderer->drawMapObject(&painter, object, color);

                    if (object->rotation() != qreal(0))
                        painter.restore();
                }
            }
            break;
        }
        case Layer::ImageLayerType: {
            const ImageLayer *imageLayer = static_cast<const ImageLayer*>(layer);
            renderer->drawImageLayer(&painter, imageLayer);
            break;
        }

        case Layer::GroupLayerType:
            // Recursion handled by LayerIterator
            break;
        }

        painter.translate(-offset);
    }

    if (drawTileGrid) {
        Preferences *prefs = Preferences::instance();
        renderer->drawGrid(&painter, QRectF(QPointF(), renderer->mapSize()),
                           prefs->gridColor());
    }

    // Restore the previous render flags
    renderer->setFlags(renderFlags);

    image.save(fileName);
    mPath = QFileInfo(fileName).path();

    // Store settings for next time
    QSettings *s = Preferences::instance()->settings();
    s->setValue(QLatin1String(VISIBLE_ONLY_KEY), visibleLayersOnly);
    s->setValue(QLatin1String(CURRENT_SCALE_KEY), useCurrentScale);
    s->setValue(QLatin1String(DRAW_GRID_KEY), drawTileGrid);
    s->setValue(QLatin1String(INCLUDE_BACKGROUND_COLOR), includeBackgroundColor);

    QDialog::accept();
}

void ExportAsImageDialog::browse()
{
    // Don't confirm overwrite here, since we'll confirm when the user presses
    // the Export button
    const QString filter = Utils::writableImageFormatsFilter();
    QString f = QFileDialog::getSaveFileName(this, tr("Image"),
                                             mUi->fileNameEdit->text(),
                                             filter, nullptr,
                                             QFileDialog::DontConfirmOverwrite);
    if (!f.isEmpty()) {
        mUi->fileNameEdit->setText(f);
        mPath = f;
    }
}

void ExportAsImageDialog::updateAcceptEnabled()
{
    QPushButton *saveButton = mUi->buttonBox->button(QDialogButtonBox::Save);
    saveButton->setEnabled(!mUi->fileNameEdit->text().isEmpty());
}
