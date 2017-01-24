/*
 * mapdocument.h
 * Copyright 2008-2014, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Jeff Bland <jeff@teamphobic.com>
 * Copyright 2011, Stefan Beller <stefanbeller@googlemail.com
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

#ifndef MAPDOCUMENT_H
#define MAPDOCUMENT_H

#include "document.h"
#include "layer.h"
#include "tiled.h"
#include "tileset.h"

#include <QList>
#include <QPointer>
#include <QRegion>

class QModelIndex;
class QPoint;
class QRect;
class QSize;
class QUndoStack;

namespace Tiled {

class Map;
class MapObject;
class MapRenderer;
class MapFormat;
class Terrain;
class Tile;

namespace Internal {

class LayerModel;
class MapObjectModel;
class TerrainModel;
class TileSelectionModel;

/**
 * Represents an editable map. The purpose of this class is to make sure that
 * any editing operations will cause the appropriate signals to be emitted, in
 * order to allow the GUI to update accordingly.
 *
 * The map document provides the layer model, keeps track of the currently
 * selected layer and provides an API for adding and removing map objects. It
 * also owns the QUndoStack.
 */
class MapDocument : public Document
{
    Q_OBJECT

public:
    /**
     * Constructs a map document around the given map. The map document takes
     * ownership of the map.
     */
    MapDocument(Map *map, const QString &fileName = QString());

    ~MapDocument();

    bool save(const QString &fileName, QString *error = nullptr) override;

    /**
     * Loads a map and returns a MapDocument instance on success. Returns null
     * on error and sets the \a error message.
     */
    static MapDocument *load(const QString &fileName,
                             MapFormat *format,
                             QString *error = nullptr);

    QString lastExportFileName() const;
    void setLastExportFileName(const QString &fileName);

    MapFormat *readerFormat() const;
    void setReaderFormat(MapFormat *format);

    FileFormat *writerFormat() const override;
    void setWriterFormat(MapFormat *format);

    MapFormat *exportFormat() const;
    void setExportFormat(MapFormat *format);

    QString displayName() const override;

    /**
     * Returns the map instance. Be aware that directly modifying the map will
     * not allow the GUI to update itself appropriately.
     */
    Map *map() const { return mMap; }

    int layerIndex(const Layer *layer) const;

    /**
     * Returns the currently selected layer, or 0 if no layer is currently
     * selected.
     */
    Layer *currentLayer() const { return mCurrentLayer; }
    void setCurrentLayer(Layer *layer);

    /**
     * Resize this map to the given \a size, while at the same time shifting
     * the contents by \a offset. If \a removeObjects is true then all objects
     * which are outside the map will be removed.
     */
    void resizeMap(const QSize &size, const QPoint &offset, bool removeObjects);

    /**
     * Offsets the layers at \a layerIndexes by \a offset, within \a bounds,
     * and optionally wraps on the X or Y axis.
     */
    void offsetMap(const QList<Layer *> &layerIndexes,
                   const QPoint &offset,
                   const QRect &bounds,
                   bool wrapX, bool wrapY);

    void flipSelectedObjects(FlipDirection direction);
    void rotateSelectedObjects(RotateDirection direction);

    Layer *addLayer(Layer::TypeFlag layerType);
    void duplicateLayer();
    void mergeLayerDown();
    void moveLayerUp(Layer *layer);
    void moveLayerDown(Layer *layer);
    void removeLayer(Layer *layer);
    void toggleOtherLayers(Layer *layer);

    void insertTileset(int index, const SharedTileset &tileset);
    void removeTilesetAt(int index);
    SharedTileset replaceTileset(int index, const SharedTileset &tileset);

    void duplicateObjects(const QList<MapObject*> &objects);
    void removeObjects(const QList<MapObject*> &objects);
    void moveObjectsToGroup(const QList<MapObject*> &objects,
                            ObjectGroup *objectGroup);
    void moveObjectsUp(const QList<MapObject*> &objects);
    void moveObjectsDown(const QList<MapObject*> &objects);

    /**
     * Returns the layer model. Can be used to modify the layer stack of the
     * map, and to display the layer stack in a view.
     */
    LayerModel *layerModel() const { return mLayerModel; }

    MapObjectModel *mapObjectModel() const { return mMapObjectModel; }

    TerrainModel *terrainModel() const { return mTerrainModel; }

    /**
     * Returns the map renderer.
     */
    MapRenderer *renderer() const { return mRenderer; }

    /**
     * Creates the map renderer. Should be called after changing the map
     * orientation.
     */
    void createRenderer();

    /**
     * Returns the selected area of tiles.
     */
    const QRegion &selectedArea() const { return mSelectedArea; }

    /**
     * Sets the selected area of tiles.
     */
    void setSelectedArea(const QRegion &selection);

    /**
     * Returns the list of selected objects.
     */
    const QList<MapObject*> &selectedObjects() const
    { return mSelectedObjects; }

    /**
     * Sets the list of selected objects, emitting the selectedObjectsChanged
     * signal.
     */
    void setSelectedObjects(const QList<MapObject*> &selectedObjects);

    QList<Object*> currentObjects() const override;

    void unifyTilesets(Map *map);
    void unifyTilesets(Map *map, QVector<SharedTileset> &missingTilesets);

    void emitMapChanged();

    void emitRegionChanged(const QRegion &region, Layer *layer);
    void emitRegionEdited(const QRegion &region, Layer *layer);

    void emitTileLayerDrawMarginsChanged(TileLayer *layer);

    void emitObjectGroupChanged(ObjectGroup *objectGroup);
    void emitImageLayerChanged(ImageLayer *imageLayer);

    void emitEditLayerNameRequested();

signals:
    /**
     * Emitted when the selected tile region changes. Sends the currently
     * selected region and the previously selected region.
     */
    void selectedAreaChanged(const QRegion &newSelection,
                             const QRegion &oldSelection);

    /**
     * Emitted when the list of selected objects changes.
     */
    void selectedObjectsChanged();

    /**
     * Emitted when the map size or its tile size changes.
     */
    void mapChanged();

    void layerAdded(Layer *layer);
    void layerAboutToBeRemoved(GroupLayer *parentLayer, int index);
    void layerRemoved(Layer *layer);
    void layerChanged(Layer *layer);

    /**
     * Emitted after a new layer was added and the name should be edited.
     * Applies to the current layer.
     */
    void editLayerNameRequested();

    /**
     * Emitted when the current layer changes.
     */
    void currentLayerChanged(Layer *layer);

    /**
     * Emitted when a certain region of the map changes. The region is given in
     * tile coordinates.
     */
    void regionChanged(const QRegion &region, Layer *layer);

    /**
     * Emitted when a certain region of the map was edited by user input.
     * The region is given in tile coordinates.
     * If multiple layers have been edited, multiple signals will be emitted.
     */
    void regionEdited(const QRegion &region, Layer *layer);

    void tileLayerDrawMarginsChanged(TileLayer *layer);

    void objectGroupChanged(ObjectGroup *objectGroup);

    void imageLayerChanged(ImageLayer *imageLayer);

    void tilesetAboutToBeAdded(int index);
    void tilesetAdded(int index, Tileset *tileset);
    void tilesetAboutToBeRemoved(int index);
    void tilesetRemoved(Tileset *tileset);
    void tilesetReplaced(int index, Tileset *tileset, Tileset *oldTileset);

    void objectsAdded(const QList<MapObject*> &objects);
    void objectsInserted(ObjectGroup *objectGroup, int first, int last);
    void objectsRemoved(const QList<MapObject*> &objects);
    void objectsChanged(const QList<MapObject*> &objects);
    void objectsTypeChanged(const QList<MapObject*> &objects);
    void objectsIndexChanged(ObjectGroup *objectGroup, int first, int last);

    // emitted from the TilesetDocument
    void tilesetNameChanged(Tileset *tileset);
    void tilesetTerrainAboutToBeAdded(Tileset *tileset, int terrainId);
    void tilesetTerrainAdded(Tileset *tileset, int terrainId);
    void tilesetTerrainAboutToBeRemoved(Tileset *tileset, Terrain *terrain);
    void tilesetTerrainRemoved(Tileset *tileset, Terrain *terrain);

private slots:
    void onObjectsRemoved(const QList<MapObject*> &objects);

    void onMapObjectModelRowsInserted(const QModelIndex &parent, int first, int last);
    void onMapObjectModelRowsInsertedOrRemoved(const QModelIndex &parent, int first, int last);
    void onObjectsMoved(const QModelIndex &parent, int start, int end,
                        const QModelIndex &destination, int row);

    void onLayerAdded(Layer *layer);
    void onLayerAboutToBeRemoved(GroupLayer *groupLayer, int index);
    void onLayerRemoved(Layer *layer);

private:
    void deselectObjects(const QList<MapObject*> &objects);
    void moveObjectIndex(const MapObject *object, int count);

    QString mLastExportFileName;

    /*
     * QPointer is used since the formats referenced here may be dynamically
     * added by a plugin, and can also be removed again.
     */
    QPointer<MapFormat> mReaderFormat;
    QPointer<MapFormat> mWriterFormat;
    QPointer<MapFormat> mExportFormat;
    Map *mMap;
    LayerModel *mLayerModel;
    QRegion mSelectedArea;
    QList<MapObject*> mSelectedObjects;
    Object *mCurrentObject;             /**< Current properties object. */
    MapRenderer *mRenderer;
    Layer* mCurrentLayer;
    MapObjectModel *mMapObjectModel;
    TerrainModel *mTerrainModel;
};


inline QString MapDocument::lastExportFileName() const
{
    return mLastExportFileName;
}

inline void MapDocument::setLastExportFileName(const QString &fileName)
{
    mLastExportFileName = fileName;
}

/**
 * Emits the map changed signal. This signal should be emitted after changing
 * the map size or its tile size.
 */
inline void MapDocument::emitMapChanged()
{
    emit mapChanged();
}

/**
 * Emits the region changed signal for the specified region. The region
 * should be in tile coordinates. This method is used by the TilePainter.
 */
inline void MapDocument::emitRegionChanged(const QRegion &region, Layer *layer)
{
    emit regionChanged(region, layer);
}

/**
 * Emits the region edited signal for the specified region and tile layer.
 * The region should be in tile coordinates. This should be called from
 * all map document changing classes which are triggered by user input.
 */
inline void MapDocument::emitRegionEdited(const QRegion &region, Layer *layer)
{
    emit regionEdited(region, layer);
}

inline void MapDocument::emitTileLayerDrawMarginsChanged(TileLayer *layer)
{
    emit tileLayerDrawMarginsChanged(layer);
}

/**
 * Emits the objectGroupChanged signal, should be called when changing the
 * color or drawing order of an object group.
 */
inline void MapDocument::emitObjectGroupChanged(ObjectGroup *objectGroup)
{
    emit objectGroupChanged(objectGroup);
}

/**
 * Emits the imageLayerChanged signal, should be called when changing the
 * image or the transparent color of an image layer.
 */
inline void MapDocument::emitImageLayerChanged(ImageLayer *imageLayer)
{
    emit imageLayerChanged(imageLayer);
}

/**
 * Emits the editLayerNameRequested signal, to get renamed.
 */
inline void MapDocument::emitEditLayerNameRequested()
{
    emit editLayerNameRequested();
}

} // namespace Internal
} // namespace Tiled

#endif // MAPDOCUMENT_H
