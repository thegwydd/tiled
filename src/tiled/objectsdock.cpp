/*
 * objectsdock.cpp
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

#include "objectsdock.h"

#include "documentmanager.h"
#include "map.h"
#include "mapdocument.h"
#include "mapdocumentactionhandler.h"
#include "mapobject.h"
#include "mapobjectmodel.h"
#include "objectgroup.h"
#include "preferences.h"
#include "reversingproxymodel.h"
#include "utils.h"

#include <QApplication>
#include <QBoxLayout>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

static const char FIRST_SECTION_SIZE_KEY[] = "ObjectsDock/FirstSectionSize";

using namespace Tiled;
using namespace Tiled::Internal;

ObjectsDock::ObjectsDock(QWidget *parent)
    : QDockWidget(parent)
    , mObjectsView(new ObjectsView)
    , mMapDocument(nullptr)
{
    setObjectName(QLatin1String("ObjectsDock"));

    mActionObjectProperties = new QAction(this);
    mActionObjectProperties->setIcon(QIcon(QLatin1String(":/images/16x16/document-properties.png")));

    connect(mActionObjectProperties, SIGNAL(triggered()), SLOT(objectProperties()));

    MapDocumentActionHandler *handler = MapDocumentActionHandler::instance();

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(mObjectsView);

    mActionNewLayer = new QAction(this);
    mActionNewLayer->setIcon(QIcon(QLatin1String(":/images/16x16/document-new.png")));
    connect(mActionNewLayer, SIGNAL(triggered()),
            handler->actionAddObjectGroup(), SIGNAL(triggered()));

    mActionMoveToGroup = new QAction(this);
    mActionMoveToGroup->setIcon(QIcon(QLatin1String(":/images/16x16/layer-object.png")));

    mActionMoveUp = new QAction(this);
    mActionMoveUp->setIcon(QIcon(QLatin1String(":/images/16x16/go-up.png")));
    mActionMoveDown = new QAction(this);
    mActionMoveDown->setIcon(QIcon(QLatin1String(":/images/16x16/go-down.png")));

    Utils::setThemeIcon(mActionObjectProperties, "document-properties");
    Utils::setThemeIcon(mActionMoveUp, "go-up");
    Utils::setThemeIcon(mActionMoveDown, "go-down");

    QToolBar *toolBar = new QToolBar;
    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    toolBar->setIconSize(Utils::smallIconSize());

    toolBar->addAction(mActionNewLayer);
    toolBar->addAction(handler->actionDuplicateObjects());
    toolBar->addAction(handler->actionRemoveObjects());

    toolBar->addAction(mActionMoveUp);
    toolBar->addAction(mActionMoveDown);
    toolBar->addAction(mActionMoveToGroup);
    QToolButton *button;
    button = dynamic_cast<QToolButton*>(toolBar->widgetForAction(mActionMoveToGroup));
    mMoveToMenu = new QMenu(this);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setMenu(mMoveToMenu);
    connect(mMoveToMenu, SIGNAL(aboutToShow()), SLOT(aboutToShowMoveToMenu()));
    connect(mMoveToMenu, SIGNAL(triggered(QAction*)),
            SLOT(triggeredMoveToMenu(QAction*)));

    toolBar->addAction(mActionObjectProperties);

    layout->addWidget(toolBar);
    setWidget(widget);
    retranslateUi();

    connect(DocumentManager::instance(), &DocumentManager::documentAboutToClose,
            this, &ObjectsDock::documentAboutToClose);

    connect(mActionMoveUp, &QAction::triggered, this, &ObjectsDock::moveObjectsUp);
    connect(mActionMoveDown, &QAction::triggered, this, &ObjectsDock::moveObjectsDown);
}

void ObjectsDock::moveObjectsUp()
{
    if (mMapDocument)
        mMapDocument->moveObjectsUp(mMapDocument->selectedObjects());
}

void ObjectsDock::moveObjectsDown()
{
    if (mMapDocument)
        mMapDocument->moveObjectsDown(mMapDocument->selectedObjects());
}

void ObjectsDock::setMapDocument(MapDocument *mapDoc)
{
    if (mMapDocument) {
        saveExpandedGroups();
        mMapDocument->disconnect(this);
    }

    mMapDocument = mapDoc;

    mObjectsView->setMapDocument(mapDoc);

    if (mMapDocument) {
        restoreExpandedGroups();
        connect(mMapDocument, SIGNAL(selectedObjectsChanged()),
                this, SLOT(updateActions()));
    }

    updateActions();
}

void ObjectsDock::changeEvent(QEvent *e)
{
    QDockWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi();
        break;
    default:
        break;
    }
}

void ObjectsDock::retranslateUi()
{
    setWindowTitle(tr("Objects"));

    mActionNewLayer->setToolTip(tr("Add Object Layer"));
    mActionObjectProperties->setToolTip(tr("Object Properties"));
    mActionMoveUp->setToolTip(tr("Move Objects Up"));
    mActionMoveDown->setToolTip(tr("Move Objects Down"));

    updateActions();
}

void ObjectsDock::updateActions()
{
    int selectedObjectsCount = 0;
    int objectGroupCount = 0;

    if (mMapDocument) {
        selectedObjectsCount = mMapDocument->selectedObjects().count();
        objectGroupCount = mMapDocument->map()->objectGroupCount();
    }

    mActionObjectProperties->setEnabled(selectedObjectsCount > 0);
    mActionMoveToGroup->setEnabled(selectedObjectsCount > 0 && objectGroupCount >= 2);
    mActionMoveToGroup->setToolTip(tr("Move %n Object(s) to Layer", "", selectedObjectsCount));
    mActionMoveUp->setEnabled(selectedObjectsCount > 0);
    mActionMoveDown->setEnabled(selectedObjectsCount > 0);
}

void ObjectsDock::aboutToShowMoveToMenu()
{
    mMoveToMenu->clear();

    const auto &objectGroups = mMapDocument->map()->objectGroups();
    for (ObjectGroup *objectGroup : objectGroups) {
        QAction *action = mMoveToMenu->addAction(objectGroup->name());
        action->setData(QVariant::fromValue(objectGroup));
    }
}

void ObjectsDock::triggeredMoveToMenu(QAction *action)
{
    MapDocumentActionHandler *handler = MapDocumentActionHandler::instance();

    ObjectGroup *objectGroup = action->data().value<ObjectGroup*>();
    handler->moveObjectsToGroup(objectGroup);
}

void ObjectsDock::objectProperties()
{
    const QList<MapObject *> &selectedObjects = mMapDocument->selectedObjects();
    MapObject *mapObject = selectedObjects.first();
    mMapDocument->setCurrentObject(mapObject);
    emit mMapDocument->editCurrentObject();
}

void ObjectsDock::saveExpandedGroups()
{
    mExpandedGroups[mMapDocument].clear();

    const auto proxyModel = static_cast<QAbstractProxyModel*>(mObjectsView->model());
    const auto &objectGroups = mMapDocument->map()->objectGroups();

    for (ObjectGroup *og : objectGroups) {
        const QModelIndex sourceIndex = mMapDocument->mapObjectModel()->index(og);
        const QModelIndex index = proxyModel->mapFromSource(sourceIndex);
        if (mObjectsView->isExpanded(index))
            mExpandedGroups[mMapDocument].append(og);
    }
}

void ObjectsDock::restoreExpandedGroups()
{
    const auto objectGroups = mExpandedGroups.take(mMapDocument);
    for (ObjectGroup *og : objectGroups) {
        const QModelIndex sourceIndex = mMapDocument->mapObjectModel()->index(og);
        const QModelIndex index = static_cast<QAbstractProxyModel*>(mObjectsView->model())->mapFromSource(sourceIndex);
        mObjectsView->setExpanded(index, true);
    }
}

void ObjectsDock::documentAboutToClose(Document *document)
{
    if (MapDocument *mapDocument = qobject_cast<MapDocument*>(document))
        mExpandedGroups.remove(mapDocument);
}

///// ///// ///// ///// /////

ObjectsView::ObjectsView(QWidget *parent)
    : QTreeView(parent)
    , mMapDocument(nullptr)
    , mProxyModel(new ReversingProxyModel(this))
    , mSynching(false)
{
    setUniformRowHeights(true);
    setModel(mProxyModel);

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(pressed(QModelIndex)), SLOT(onPressed(QModelIndex)));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(onActivated(QModelIndex)));

    connect(header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onSectionResized(int)));
}

QSize ObjectsView::sizeHint() const
{
    return Utils::dpiScaled(QSize(130, 100));
}

void ObjectsView::setMapDocument(MapDocument *mapDoc)
{
    if (mapDoc == mMapDocument)
        return;

    if (mMapDocument)
        mMapDocument->disconnect(this);

    mMapDocument = mapDoc;

    if (mMapDocument) {
        mProxyModel->setSourceModel(mMapDocument->mapObjectModel());

        const QSettings *settings = Preferences::instance()->settings();
        const int firstSectionSize =
                settings->value(QLatin1String(FIRST_SECTION_SIZE_KEY), 200).toInt();
        header()->resizeSection(0, firstSectionSize);

        connect(mMapDocument, SIGNAL(selectedObjectsChanged()),
                this, SLOT(selectedObjectsChanged()));

        synchronizeSelectedItems();
    } else {
        mProxyModel->setSourceModel(nullptr);
    }
}

MapObjectModel *ObjectsView::mapObjectModel() const
{
    return mMapDocument ? mMapDocument->mapObjectModel() : nullptr;
}

void ObjectsView::onPressed(const QModelIndex &proxyIndex)
{
    const QModelIndex index = mProxyModel->mapToSource(proxyIndex);

    if (MapObject *mapObject = mapObjectModel()->toMapObject(index))
        mMapDocument->setCurrentObject(mapObject);
    else if (ObjectGroup *objectGroup = mapObjectModel()->toObjectGroup(index))
        mMapDocument->setCurrentObject(objectGroup);
}

void ObjectsView::onActivated(const QModelIndex &proxyIndex)
{
    const QModelIndex index = mProxyModel->mapToSource(proxyIndex);

    if (MapObject *mapObject = mapObjectModel()->toMapObject(index)) {
        mMapDocument->setCurrentObject(mapObject);
        emit mMapDocument->editCurrentObject();
    }
}

void ObjectsView::onSectionResized(int logicalIndex)
{
    if (logicalIndex != 0)
        return;

    QSettings *settings = Preferences::instance()->settings();
    settings->setValue(QLatin1String(FIRST_SECTION_SIZE_KEY),
                       header()->sectionSize(0));
}

void ObjectsView::selectionChanged(const QItemSelection &selected,
                                   const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    if (!mMapDocument || mSynching)
        return;

    const QModelIndexList selectedProxyRows = selectionModel()->selectedRows();
    ObjectGroup *singleObjectGroup = nullptr;
    bool multipleObjectGroups = false;

    QList<MapObject*> selectedObjects;
    for (const QModelIndex &proxyIndex : selectedProxyRows) {
        const QModelIndex index = mProxyModel->mapToSource(proxyIndex);

        if (ObjectGroup *og = mapObjectModel()->toLayer(index)) {
            if (!multipleObjectGroups) {
                if (!singleObjectGroup) {
                    singleObjectGroup = og;
                } else if (singleObjectGroup != og) {
                    singleObjectGroup = nullptr;
                    multipleObjectGroups = true;
                }
            }
        }
        if (MapObject *o = mapObjectModel()->toMapObject(index))
            selectedObjects.append(o);
    }

    // Switch the current object layer if only one object layer (and/or its objects)
    // are included in the current selection.
    if (singleObjectGroup)
        mMapDocument->setCurrentLayer(singleObjectGroup);

    if (selectedObjects != mMapDocument->selectedObjects()) {
        mSynching = true;
        if (selectedObjects.count() == 1) {
            const MapObject *o = selectedObjects.first();
            const QPointF center = o->bounds().center();
            DocumentManager::instance()->centerViewOn(center);
        }
        mMapDocument->setSelectedObjects(selectedObjects);
        mSynching = false;
    }
}

void ObjectsView::selectedObjectsChanged()
{
    if (mSynching)
        return;

    synchronizeSelectedItems();

    const QList<MapObject *> &selectedObjects = mMapDocument->selectedObjects();
    if (selectedObjects.count() == 1) {
        MapObject *o = selectedObjects.first();
        scrollTo(mProxyModel->mapFromSource(mapObjectModel()->index(o)));
    }
}

void ObjectsView::synchronizeSelectedItems()
{
    Q_ASSERT(!mSynching);
    Q_ASSERT(mMapDocument);

    QItemSelection itemSelection;

    for (MapObject *o : mMapDocument->selectedObjects()) {
        QModelIndex index = mProxyModel->mapFromSource(mapObjectModel()->index(o));
        itemSelection.select(index, index);
    }

    mSynching = true;
    selectionModel()->select(itemSelection,
                             QItemSelectionModel::Select |
                             QItemSelectionModel::Rows |
                             QItemSelectionModel::Clear);
    mSynching = false;
}
