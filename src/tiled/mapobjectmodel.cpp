/*
 * mapobjectmodel.cpp
 * Copyright 2012, Tim Baker <treectrl@hotmail.com>
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

#include "mapobjectmodel.h"

#include "changemapobject.h"
#include "grouplayer.h"
#include "layermodel.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "renamelayer.h"

#include <QCoreApplication>

using namespace Tiled;
using namespace Tiled::Internal;

MapObjectModel::MapObjectModel(QObject *parent):
    QAbstractItemModel(parent),
    mMapDocument(nullptr),
    mMap(nullptr),
    mObjectGroupIcon(QLatin1String(":/images/16x16/layer-object.png"))
{
}

QModelIndex MapObjectModel::index(int row, int column,
                                  const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row < mObjectGroups.count())
            return createIndex(row, column, mGroups[mObjectGroups.at(row)]);
        return QModelIndex();
    }

    ObjectGroup *og = toObjectGroup(parent);

    // happens when deleting the last item in a parent
    if (row >= og->objectCount())
        return QModelIndex();

    // Paranoia: sometimes "fake" objects are in use (see createobjecttool)
    if (!mObjects.contains(og->objects().at(row)))
        return QModelIndex();

    return createIndex(row, column, mObjects[og->objects()[row]]);
}

QModelIndex MapObjectModel::parent(const QModelIndex &index) const
{
    if (MapObject *mapObject = toMapObject(index))
        return this->index(mapObject->objectGroup());

    return QModelIndex();
}

int MapObjectModel::rowCount(const QModelIndex &parent) const
{
    if (!mMapDocument)
        return 0;
    if (!parent.isValid())
        return mObjectGroups.size();
    if (ObjectGroup *og = toObjectGroup(parent))
        return og->objectCount();
    return 0;
}

int MapObjectModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2; // MapObject name|type
}

QVariant MapObjectModel::data(const QModelIndex &index, int role) const
{
    if (MapObject *mapObject = toMapObject(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return index.column() ? mapObject->type() : mapObject->name();
        case Qt::DecorationRole:
            return QVariant(); // no icon -> maybe the color?
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return mapObject->isVisible() ? Qt::Checked : Qt::Unchecked;
        case OpacityRole:
            return qreal(1);
        default:
            return QVariant();
        }
    }
    if (ObjectGroup *objectGroup = toObjectGroup(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return index.column() ? QVariant() : objectGroup->name();
        case Qt::DecorationRole:
            return index.column() ? QVariant() : mObjectGroupIcon;
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return objectGroup->isVisible() ? Qt::Checked : Qt::Unchecked;
        case OpacityRole:
            return objectGroup->opacity();
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool MapObjectModel::setData(const QModelIndex &index, const QVariant &value,
                             int role)
{
    if (MapObject *mapObject = toMapObject(index)) {
        switch (role) {
        case Qt::CheckStateRole: {
            Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
            const bool visible = (c == Qt::Checked);
            if (visible != mapObject->isVisible()) {
                QUndoCommand *command = new SetMapObjectVisible(mMapDocument,
                                                                mapObject,
                                                                visible);
                mMapDocument->undoStack()->push(command);
            }
            return true;
        }
        case Qt::EditRole: {
            const QString s = value.toString();
            if (index.column() == 0 && s != mapObject->name()) {
                QUndoStack *undo = mMapDocument->undoStack();
                undo->beginMacro(tr("Change Object Name"));
                undo->push(new ChangeMapObject(mMapDocument, mapObject,
                                               s, mapObject->type()));
                undo->endMacro();
            }
            if (index.column() == 1 && s != mapObject->type()) {
                QUndoStack *undo = mMapDocument->undoStack();
                undo->beginMacro(tr("Change Object Type"));
                undo->push(new ChangeMapObject(mMapDocument, mapObject,
                                               mapObject->name(), s));
                undo->endMacro();
            }
            return true;
        }
        }
        return false;
    }
    if (ObjectGroup *objectGroup = toObjectGroup(index)) {
        switch (role) {
        case Qt::CheckStateRole: {
            LayerModel *layerModel = mMapDocument->layerModel();
            layerModel->setData(layerModel->index(objectGroup), value, role);
            return true;
        }
        case Qt::EditRole: {
            const QString newName = value.toString();
            if (objectGroup->name() != newName) {
                RenameLayer *rename = new RenameLayer(mMapDocument, objectGroup,
                                                      newName);
                mMapDocument->undoStack()->push(rename);
            }
            return true;
        }
        }
        return false;
    }
    return false;
}

Qt::ItemFlags MapObjectModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractItemModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    else if (index.parent().isValid())
        rc |= Qt::ItemIsEditable; // MapObject type
    return rc;
}

QVariant MapObjectModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Type");
        }
    }
    return QVariant();
}

QModelIndex MapObjectModel::index(ObjectGroup *og) const
{
    const int row = mObjectGroups.indexOf(og);
    Q_ASSERT(mGroups[og]);
    return createIndex(row, 0, mGroups[og]);
}

QModelIndex MapObjectModel::index(MapObject *o, int column) const
{
    const int row = o->objectGroup()->objects().indexOf(o);
    Q_ASSERT(mObjects[o]);
    return createIndex(row, column, mObjects[o]);
}

ObjectGroup *MapObjectModel::toObjectGroup(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    ObjectOrGroup *oog = static_cast<ObjectOrGroup*>(index.internalPointer());
    return oog->mGroup;
}

MapObject *MapObjectModel::toMapObject(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    ObjectOrGroup *oog = static_cast<ObjectOrGroup*>(index.internalPointer());
    return oog->mObject;
}

ObjectGroup *MapObjectModel::toLayer(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    ObjectOrGroup *oog = static_cast<ObjectOrGroup*>(index.internalPointer());
    return oog->mGroup ? oog->mGroup : oog->mObject->objectGroup();
}

void MapObjectModel::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    if (mMapDocument)
        mMapDocument->disconnect(this);

    beginResetModel();
    mMapDocument = mapDocument;
    mMap = nullptr;

    mObjectGroups.clear();
    qDeleteAll(mGroups);
    mGroups.clear();
    qDeleteAll(mObjects);
    mObjects.clear();

    if (mMapDocument) {
        mMap = mMapDocument->map();

        connect(mMapDocument, &MapDocument::layerAdded,
                this, &MapObjectModel::layerAdded);
        connect(mMapDocument, &MapDocument::layerChanged,
                this, &MapObjectModel::layerChanged);
        connect(mMapDocument, &MapDocument::layerAboutToBeRemoved,
                this, &MapObjectModel::layerAboutToBeRemoved);

        for (ObjectGroup *og : mMap->objectGroups()) {
            mObjectGroups.append(og);
            mGroups.insert(og, new ObjectOrGroup(og));
            for (MapObject *o : og->objects())
                mObjects.insert(o, new ObjectOrGroup(o));
        }
    }

    endResetModel();
}

void MapObjectModel::layerAdded(Layer *layer)
{
    int index = mMap->layers().indexOf(layer);
    if (index == -1)
        return; // todo: nested object groups not supported yet

    if (ObjectGroup *og = layer->asObjectGroup()) {
        if (!mGroups.contains(og)) {
            ObjectGroup *prev = nullptr;
            for (index = index - 1; index >= 0; --index)
                if ((prev = mMap->layerAt(index)->asObjectGroup()))
                    break;
            index = prev ? mObjectGroups.indexOf(prev) + 1 : 0;
            mObjectGroups.insert(index, og);
            const int row = mObjectGroups.indexOf(og);
            beginInsertRows(QModelIndex(), row, row);
            mGroups.insert(og, new ObjectOrGroup(og));
            for (MapObject *o : og->objects()) {
                if (!mObjects.contains(o))
                    mObjects.insert(o, new ObjectOrGroup(o));
            }
            endInsertRows();
        }
    }
}

void MapObjectModel::layerChanged(Layer *layer)
{
    if (layer->parentLayer())
        return; // todo: nested object groups not supported yet

    if (ObjectGroup *og = layer->asObjectGroup()) {
        QModelIndex index = this->index(og);
        emit dataChanged(index, index);
    }
}

void MapObjectModel::layerAboutToBeRemoved(GroupLayer *groupLayer, int index)
{
    if (groupLayer)
        return; // todo: nested object groups not supported yet

    Layer *layer = mMap->layerAt(index);
    if (ObjectGroup *og = layer->asObjectGroup()) {
        const int row = mObjectGroups.indexOf(og);
        beginRemoveRows(QModelIndex(), row, row);
        mObjectGroups.removeAt(row);
        delete mGroups.take(og);
        for (MapObject *o : og->objects())
            delete mObjects.take(o);

        endRemoveRows();
    }
}

void MapObjectModel::insertObject(ObjectGroup *og, int index, MapObject *o)
{
    const int row = (index >= 0) ? index : og->objectCount();
    beginInsertRows(this->index(og), row, row);
    og->insertObject(row, o);
    mObjects.insert(o, new ObjectOrGroup(o));
    endInsertRows();
    emit objectsAdded(QList<MapObject*>() << o);
}

int MapObjectModel::removeObject(ObjectGroup *og, MapObject *o)
{
    QList<MapObject*> objects;
    objects << o;

    const int row = og->objects().indexOf(o);
    beginRemoveRows(index(og), row, row);
    og->removeObjectAt(row);
    delete mObjects.take(o);
    endRemoveRows();
    emit objectsRemoved(objects);
    return row;
}

void MapObjectModel::moveObjects(ObjectGroup *og, int from, int to, int count)
{
    const QModelIndex parent = index(og);
    if (!beginMoveRows(parent, from, from + count - 1, parent, to)) {
        Q_ASSERT(false); // The code should never attempt this
        return;
    }

    og->moveObjects(from, to, count);
    endMoveRows();
}

// ObjectGroup color changed
// FIXME: layerChanged should let the scene know that objects need redrawing
void MapObjectModel::emitObjectsChanged(const QList<MapObject *> &objects)
{
    if (objects.isEmpty())
        return;

    emit objectsChanged(objects);
}

void MapObjectModel::setObjectName(MapObject *o, const QString &name)
{
    if (o->name() == name)
        return;

    o->setName(name);
    QModelIndex index = this->index(o);
    emit dataChanged(index, index);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectType(MapObject *o, const QString &type)
{
    if (o->type() == type)
        return;

    o->setType(type);
    QModelIndex index = this->index(o, 1);
    emit dataChanged(index, index);

    QList<MapObject*> objects = QList<MapObject*>() << o;
    emit objectsChanged(objects);
    emit objectsTypeChanged(objects);
}

void MapObjectModel::setObjectPolygon(MapObject *o, const QPolygonF &polygon)
{
    if (o->polygon() == polygon)
        return;

    o->setPolygon(polygon);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectPosition(MapObject *o, const QPointF &pos)
{
    if (o->position() == pos)
        return;

    o->setPosition(pos);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectSize(MapObject *o, const QSizeF &size)
{
    if (o->size() == size)
        return;

    o->setSize(size);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectRotation(MapObject *o, qreal rotation)
{
    if (o->rotation() == rotation)
        return;

    o->setRotation(rotation);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectVisible(MapObject *o, bool visible)
{
    if (o->isVisible() == visible)
        return;

    o->setVisible(visible);
    QModelIndex index = this->index(o);
    emit dataChanged(index, index);
    emit objectsChanged(QList<MapObject*>() << o);
}
