/*
 * mapscene.h
 * Copyright 2008-2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2008, Roderic Morris <roderic@ccs.neu.edu>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
 * Copyright 2010, Jeff Bland <jksb@member.fsf.org>
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

#ifndef MAPSCENE_H
#define MAPSCENE_H

#include <QColor>
#include <QGraphicsScene>
#include <QMap>
#include <QSet>

namespace Tiled {

class ImageLayer;
class Layer;
class MapObject;
class ObjectGroup;
class Tile;
class TileLayer;
class Tileset;

namespace Internal {

class AbstractTool;
class MapDocument;
class MapObjectItem;
class MapScene;
class ObjectGroupItem;
class ObjectSelectionItem;

/**
 * A graphics scene that represents the contents of a map.
 */
class MapScene : public QGraphicsScene
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    MapScene(QObject *parent);

    /**
     * Destructor.
     */
    ~MapScene();

    /**
     * Returns the map document this scene is displaying.
     */
    MapDocument *mapDocument() const { return mMapDocument; }

    /**
     * Sets the map this scene displays.
     */
    void setMapDocument(MapDocument *map);

    /**
     * Returns whether the tile grid is visible.
     */
    bool isGridVisible() const { return mGridVisible; }

    /**
     * Returns the set of selected map object items.
     */
    const QSet<MapObjectItem*> &selectedObjectItems() const
    { return mSelectedObjectItems; }

    /**
     * Sets the set of selected map object items. This translates to a call to
     * MapDocument::setSelectedObjects.
     */
    void setSelectedObjectItems(const QSet<MapObjectItem*> &items);

    /**
     * Returns the MapObjectItem associated with the given \a mapObject.
     */
    MapObjectItem *itemForObject(MapObject *object) const
    { return mObjectItems.value(object); }

    /**
     * Enables the selected tool at this map scene.
     * Therefore it tells that tool, that this is the active map scene.
     */
    void enableSelectedTool();
    void disableSelectedTool();

    /**
     * Sets the currently selected tool.
     */
    void setSelectedTool(AbstractTool *tool);

signals:
    void selectedObjectItemsChanged();

protected:
    /**
     * QGraphicsScene::drawForeground override that draws the tile grid.
     */
    void drawForeground(QPainter *painter, const QRectF &rect) override;

    /**
     * Override for handling enter and leave events.
     */
    bool event(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;

private slots:
    void setGridVisible(bool visible);
    void setObjectLineWidth(qreal lineWidth);
    void setShowTileObjectOutlines(bool enabled);

    /**
     * Sets whether the current layer should be highlighted.
     */
    void setHighlightCurrentLayer(bool highlightCurrentLayer);

    /**
     * Refreshes the map scene.
     */
    void refreshScene();

    /**
     * Repaints the specified region. The region is in tile coordinates.
     */
    void repaintRegion(const QRegion &region, Layer *layer);

    void currentLayerChanged();

    void mapChanged();
    void repaintTileset(Tileset *tileset);
    void tileLayerDrawMarginsChanged(TileLayer *tileLayer);

    void layerAdded(Layer *layer);
    void layerRemoved(Layer *layer);
    void layerChanged(Layer *layer);

    void objectGroupChanged(ObjectGroup *objectGroup);
    void imageLayerChanged(ImageLayer *imageLayer);

    void adaptToTilesetTileSizeChanges(Tileset *tileset);
    void adaptToTileSizeChanges(Tile *tile);

    void tilesetReplaced(int index, Tileset *tileset);

    void objectsInserted(ObjectGroup *objectGroup, int first, int last);
    void objectsRemoved(const QList<MapObject*> &objects);
    void objectsChanged(const QList<MapObject*> &objects);
    void objectsIndexChanged(ObjectGroup *objectGroup, int first, int last);

    void updateSelectedObjectItems();
    void syncAllObjectItems();

private:
    QGraphicsItem *createLayerItem(Layer *layer);

    void updateDefaultBackgroundColor();
    void updateSceneRect();
    void updateCurrentLayerHighlight();

    bool eventFilter(QObject *object, QEvent *event) override;

    MapDocument *mMapDocument;
    AbstractTool *mSelectedTool;
    AbstractTool *mActiveTool;
    bool mGridVisible;
    qreal mObjectLineWidth;
    bool mShowTileObjectOutlines;
    bool mHighlightCurrentLayer;
    bool mUnderMouse;
    Qt::KeyboardModifiers mCurrentModifiers;
    QPointF mLastMousePos;
    QMap<Layer*, QGraphicsItem*> mLayerItems;
    QGraphicsRectItem *mDarkRectangle;
    QColor mDefaultBackgroundColor;
    ObjectSelectionItem *mObjectSelectionItem;

    QMap<MapObject*, MapObjectItem*> mObjectItems;
    QSet<MapObjectItem*> mSelectedObjectItems;
};

} // namespace Internal
} // namespace Tiled

#endif // MAPSCENE_H
