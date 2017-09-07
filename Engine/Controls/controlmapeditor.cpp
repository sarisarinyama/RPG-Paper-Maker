﻿/*
    RPG Paper Maker Copyright (C) 2017 Marie Laporte

    This file is part of RPG Paper Maker.

    RPG Paper Maker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPG Paper Maker is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "controlmapeditor.h"
#include "dialogobject.h"
#include "wanok.h"

// -------------------------------------------------------
//
//  CONSTRUCTOR / DESTRUCTOR / GET / SET
//
// -------------------------------------------------------

ControlMapEditor::ControlMapEditor() :
    m_map(nullptr),
    m_grid(nullptr),
    m_wallIndicator(nullptr),
    m_cursor(nullptr),
    m_cursorObject(nullptr),
    m_camera(new Camera),
    m_positionPreviousPreview(-1, 0, 0, -1, 0),
    m_displayGrid(true),
    m_treeMapNode(nullptr)
{

}

ControlMapEditor::~ControlMapEditor(){
    deleteMap(false);
    delete m_camera;
}

Map* ControlMapEditor::map() const { return m_map; }

Grid* ControlMapEditor::grid() const { return m_grid; }

Cursor* ControlMapEditor::cursor() const { return m_cursor; }

Cursor* ControlMapEditor::cursorObject() const { return m_cursorObject; }

Camera* ControlMapEditor::camera() const { return m_camera; }

void ControlMapEditor::setContextMenu(ContextMenuList* m){
    m_contextMenu = m;
}

void ControlMapEditor::setTreeMapNode(QStandardItem* item) {
    m_treeMapNode = item;
}

// -------------------------------------------------------
//
//  INTERMEDIARY FUNCTIONS
//
// -------------------------------------------------------

void ControlMapEditor::moveCursorToMousePosition(QPoint point){
    updateMousePosition(point);
    update();

    if (m_map->isInGrid(m_positionOnPlane)){
        m_cursor->setPositions(m_positionOnPlane);
    }
}

// -------------------------------------------------------

void ControlMapEditor::reLoadTextures(){
    m_map->loadTextures();
}

// -------------------------------------------------------

Map* ControlMapEditor::loadMap(int idMap, QVector3D* position,
                               QVector3D *positionObject, int cameraDistance,
                               int cameraHeight, double cameraHorizontalAngle)
{
    // Map & cursor
    m_cursor = new Cursor(position);
    m_map = new Map(idMap);
    m_cursor->loadTexture(":/textures/Ressources/editor_cursor.png");
    m_cursor->initializeSquareSize(m_map->squareSize());
    m_cursor->initialize();
    m_map->initializeGL();
    m_currentPortion = m_cursor->getPortion();
    m_map->loadPortions(m_currentPortion);

    // Grid
    m_grid = new Grid;
    m_grid->initializeVertices(m_map->mapProperties()->length(),
                               m_map->mapProperties()->width(),
                               m_map->squareSize());
    m_grid->initializeGL();

    // Cursor object
    m_cursorObject = new Cursor(positionObject);
    m_cursorObject->initializeSquareSize(m_map->squareSize());
    Position pos(m_cursorObject->getSquareX(),
                 m_cursorObject->getSquareY(),
                 0,
                 m_cursorObject->getSquareZ(),
                 0);
    setCursorObjectPosition(pos);
    m_cursorObject->setFrameNumber(1);
    m_cursorObject->loadTexture(
                ":/textures/Ressources/object_square_cursor.png");
    m_cursorObject->initialize();

    // Wall indicator


    // Camera
    m_camera->setDistance(cameraDistance * Wanok::coefSquareSize());
    m_camera->setHeight(cameraHeight * Wanok::coefSquareSize());
    m_camera->setHorizontalAngle(cameraHorizontalAngle);
    m_camera->update(m_cursor, m_map->squareSize());

    return m_map;
}

// -------------------------------------------------------

void ControlMapEditor::deleteMap(bool updateCamera){

    // Cursors
    if (m_cursor != nullptr){
        delete m_cursor;
        m_cursor = nullptr;
    }
    if (m_cursorObject != nullptr){
        delete m_cursorObject;
        m_cursorObject = nullptr;
    }
    if (m_wallIndicator != nullptr){
        delete m_wallIndicator;
    }

    // Grid
    if (m_grid != nullptr){
        delete m_grid;
        m_grid = nullptr;
    }

    // Map
    if (m_map != nullptr){
        delete m_map;
        m_map = nullptr;
    }

    // Update camera node
    if (updateCamera && m_treeMapNode != nullptr) {
        m_camera->setDistance(m_camera->distance() / Wanok::coefSquareSize());
        m_camera->setHeight(m_camera->height() / Wanok::coefSquareSize());
        updateCameraTreeNode();
    }
}

// -------------------------------------------------------

void ControlMapEditor::onResize(int width, int height){
    m_width = width;
    m_height = height;
    m_camera->setProjection(width, height);
}

// -------------------------------------------------------

void ControlMapEditor::updateCameraTreeNode(){
    TreeMapTag* tag = (TreeMapTag*) m_treeMapNode->data().value<quintptr>();
    tag->setCameraDistance(m_camera->distance());
    tag->setCameraHeight(m_camera->height());
    tag->setCameraHorizontalAngle(m_camera->horizontalAngle());
}

// -------------------------------------------------------
//
//  UPDATE
//
// -------------------------------------------------------

void ControlMapEditor::update()
{
    updateRaycasting();

    // Update portions
    updatePortions();
    saveTempPortions();
    clearPortionsToUpdate();
    updateMovingPortions();

    // Camera
    m_camera->update(m_cursor, m_map->squareSize());

    // Mouse update
    m_mouseBeforeUpdate = m_mouseMove;
}

// -------------------------------------------------------

void ControlMapEditor::updateMousePosition(QPoint point) {
    m_mouse = point;
}

// -------------------------------------------------------

void ControlMapEditor::updateRaycasting(){

    // Raycasting plane
    QMatrix4x4 projection = m_camera->projection();
    QMatrix4x4 view = m_camera->view();
    QVector3D ray = getRayWorld(m_mouse, projection, view);
    int height = 0;
    m_distancePlane = (height - m_camera->positionY()) / ray.y();
    getCorrectPositionOnRay(m_positionOnPlane, ray, m_distancePlane);
}

// -------------------------------------------------------

void ControlMapEditor::updatePreviewElements(
        MapEditorSelectionKind selection,
        MapEditorSubSelectionKind subSelection,
        QRect& tileset)
{
    Position position = getPositionSelected(selection);
    if (position == m_positionPreviousPreview)
        return;
    m_positionPreviousPreview = position;

    // Remove previous
    removePreviewElements();

    // Add new previous
    MapElement* element = nullptr;
    if (subSelection == MapEditorSubSelectionKind::Floors) {
        MapElement* element = nullptr;
        for (int i = 0; i < tileset.width(); i++){
            if (position.x() + i > m_map->mapProperties()->length())
                break;

            for (int j = 0; j < tileset.height(); j++){
                if (position.z() + j > m_map->mapProperties()->width())
                    break;

                Position shortPosition(position.x() + i, 0, 0, position.z() + j,
                                       position.layer());
                Portion shortPortion = getLocalPortion(shortPosition);
                if (m_map->isInGrid(shortPosition) &&
                    m_map->isInPortion(shortPortion))
                {
                    element = new FloorDatas(new QRect(tileset.x() + i,
                                                       tileset.y() + j, 1, 1));
                    updatePreviewElement(shortPosition, shortPortion, element);
                }
            }
        }
    }
    else {
        Portion portion = getLocalPortion(m_positionPreviousPreview);
        if (m_map->isInGrid(m_positionPreviousPreview) &&
            m_map->isInPortion(portion))
        {
            switch (subSelection) {
            case MapEditorSubSelectionKind::Floors:
                element = new FloorDatas(new QRect(tileset.x(),
                                                   tileset.y(),
                                                   tileset.width(),
                                                   tileset.height()));
                break;
            default:
                break;
            }
            switch (selection) {
            case MapEditorSelectionKind::Sprites:
                element = new SpriteDatas(subSelection, 50, 0,
                                          new QRect(tileset.x(),
                                                    tileset.y(),
                                                    tileset.width(),
                                                    tileset.height()));
                break;
            default:
                break;
            }

            if (element != nullptr) {
                updatePreviewElement(m_positionPreviousPreview, portion,
                                     element);
            }
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::removePreviewElements() {
    for (int i = 0; i < m_positionsPreviousPreview.size(); i++) {
        Position positionPrevious = m_positionsPreviousPreview.at(i);
        Portion portionPrevious = getLocalPortion(positionPrevious);
        if (m_map->isInGrid(positionPrevious) &&
            m_map->isInPortion(portionPrevious))
        {
            m_portionsToUpdate += portionPrevious;
            m_map->mapPortion(portionPrevious)->clearPreview();
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::updatePreviewElement(Position &p, Portion& portion,
                                            MapElement* element)
{
    MapPortion* mapPortion = m_map->mapPortion(portion);
    mapPortion->addPreview(p, element);
    m_portionsToUpdate += portion;
    m_positionsPreviousPreview.append(p);
}

// -------------------------------------------------------

void ControlMapEditor::updatePortions() {
    QSet<Portion>::iterator i;
    for (i = m_portionsToUpdate.begin(); i != m_portionsToUpdate.end(); i++){
        Portion p = *i;
        m_map->updatePortion(p);
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortions() {
    Portion newPortion = m_cursor->getPortion();

    updateMovingPortionsEastWest(newPortion);
    updateMovingPortionsNorthSouth(newPortion);
    updateMovingPortionsUpDown(newPortion);

    m_currentPortion = newPortion;
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsEastWest(Portion& newPortion){
    int r = m_map->portionsRay();
    if (newPortion.x() > m_currentPortion.x()) {
        int k = 0;
        for (int j = -r; j <= r; j++) {
            bool visible = j != -r && j != r;
            int i = -r;
            removePortion(i, k, j);
            setPortion(i, k, j, i + 1, k, j, false);
            i++;
            for (; i < r; i++)
                setPortion(i, k, j, i + 1, k, j, visible);

            loadPortion(newPortion, r, k, j);
        }
    }
    else if (newPortion.x() < m_currentPortion.x()) {
        int k = 0;
        for (int j = -r; j <= r; j++) {
            bool visible = j != -r && j != r;
            int i = r;
            removePortion(i, k, j);
            setPortion(i, k, j, i - 1, k, j, false);
            i--;
            for (; i > -r; i--)
                setPortion(i, k, j, i - 1, k, j, visible);

            loadPortion(newPortion, -r, k, j);
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsNorthSouth(Portion& newPortion){
    int r = m_map->portionsRay();
    if (newPortion.z() > m_currentPortion.z()) {
        int k = 0;
        for (int i = -r; i <= r; i++) {
            bool visible = i != -r && i != r;
            int j = -r;
            removePortion(i, k, j);
            setPortion(i, k, j, i, k, j + 1, false);
            j++;
            for (; j < r; j++)
                setPortion(i, k, j, i, k, j + 1, visible);

            loadPortion(newPortion, i, k, r);
        }
    }
    else if (newPortion.z() < m_currentPortion.z()) {
        int k = 0;
        for (int i = -r; i <= r; i++) {
            bool visible = i != -r && i != r;
            int j = r;
            removePortion(i, k, j);
            setPortion(i, k, j, i, k, j - 1, false);
            j--;
            for (; j > -r; j--)
                setPortion(i, k, j, i, k, j - 1, visible);

            loadPortion(newPortion, i, k, -r);
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::updateMovingPortionsUpDown(Portion&){
    // TODO
}

// -------------------------------------------------------

void ControlMapEditor::removePortion(int i, int j, int k){
    MapPortion* mapPortion = m_map->mapPortion(i, j, k);
    if (mapPortion != nullptr)
        delete mapPortion;
}

// -------------------------------------------------------

void ControlMapEditor::setPortion(int i, int j, int k, int m, int n, int o,
                                  bool visible)
{
    Portion previousPortion(i, j, k);
    Portion newPortion(m, n, o);

    m_map->replacePortion(previousPortion, newPortion, visible);
}

// -------------------------------------------------------

void ControlMapEditor::loadPortion(Portion& currentPortion, int i, int j,
                                   int k)
{
    m_map->loadPortion(currentPortion.x() + i, currentPortion.y() + j,
                       currentPortion.z() + k, i, j, k, false);
}

// -------------------------------------------------------

void ControlMapEditor::saveTempPortions(){
    QSet<Portion>::iterator i;
    for (i = m_portionsToSave.begin(); i != m_portionsToSave.end(); i++)
        saveTempPortion(*i);
}

// -------------------------------------------------------

void ControlMapEditor::saveTempPortion(Portion portion){
    Portion globalPortion = getGlobalFromLocalPortion(portion);
    m_map->savePortionMap(m_map->mapPortion(portion), globalPortion);
}

// -------------------------------------------------------

void ControlMapEditor::clearPortionsToUpdate(){
    m_portionsToUpdate.clear();
    m_portionsToSave.clear();
}

// -------------------------------------------------------

void ControlMapEditor::setToNotSaved(){
    m_map->setSaved(false);
    Wanok::mapsToSave.insert(m_map->mapProperties()->id());
    m_treeMapNode->setText(m_map->mapProperties()->name() + " *");
}

// -------------------------------------------------------

void ControlMapEditor::save(){
    m_treeMapNode->setText(m_map->mapProperties()->name());
}

// -------------------------------------------------------
//
//  MOUSE RAYCASTING
//
// -------------------------------------------------------

QVector3D ControlMapEditor::transformToNormalizedCoords(const QPoint& mouse){
    float x = (2.0f * mouse.x()) / m_width - 1.0f;
    float y = 1.0f - (2.0f * mouse.y()) / m_height;
    float z = 1.0f;

    return QVector3D(x, y, z);
}

// -------------------------------------------------------

QVector4D ControlMapEditor::transformToHomogeneousClip(QVector3D& normalized){
    return QVector4D(normalized.x(), normalized.y(), -1.0, 1.0);
}

// -------------------------------------------------------

QVector4D ControlMapEditor::transformToEyeCoords(QVector4D& rayClip,
                                                 QMatrix4x4& projection){
    QVector4D rayEye = projection.inverted() * rayClip;

    return QVector4D(rayEye.x(), rayEye.y(), -1.0, 0.0);
}

// -------------------------------------------------------

QVector3D ControlMapEditor::transformToWorldCoords(QVector4D& rayEye,
                                                   QMatrix4x4& view){
    QVector4D rayWorld = view.inverted() * rayEye;

    return QVector3D(rayWorld.x(), rayWorld.y(), rayWorld.z()).normalized();
}

// -------------------------------------------------------

QVector3D ControlMapEditor::getRayWorld(const QPoint& mouse,
                                        QMatrix4x4& projection,
                                        QMatrix4x4& view)
{
    QVector3D normalized = transformToNormalizedCoords(mouse);
    QVector4D homogeneous = transformToHomogeneousClip(normalized);
    QVector4D eye = transformToEyeCoords(homogeneous, projection);

    return transformToWorldCoords(eye, view);
}

// -------------------------------------------------------

QVector3D ControlMapEditor::getPositionOnRay(QVector3D &ray, int distance){
    return QVector3D(
                    ray.x() * distance + m_camera->positionX(),
                    ray.y() * distance + m_camera->positionY(),
                    ray.z() * distance + m_camera->positionZ()
                );
}

// -------------------------------------------------------

void ControlMapEditor::getCorrectPositionOnRay(Position &position,
                                               QVector3D &ray, int distance){
    QVector3D point = getPositionOnRay(ray, distance);
    int x = ((int) point.x()) / m_map->squareSize();
    int z = ((int) point.z()) / m_map->squareSize();
    if (point.x() < 0) x--;
    if (point.z() < 0) z--;

    position.setX(x);
    position.setZ(z);
}

// -------------------------------------------------------
//
//  ADD / REMOVE
//
// -------------------------------------------------------

void ControlMapEditor::addRemove(MapEditorSelectionKind selection,
                                 MapEditorSubSelectionKind subSelection,
                                 DrawKind drawKind,
                                 QRect& tileset,
                                 Qt::MouseButton button)
{
    Position p = getPositionSelected(selection);
    if (m_map->isInGrid(p)){
        if (button == Qt::MouseButton::LeftButton)
            add(selection, subSelection, drawKind, tileset, p);
        else if (button == Qt::MouseButton::RightButton)
            remove(selection, drawKind, p);
    }
}

// -------------------------------------------------------

Position ControlMapEditor::getPositionSelected(MapEditorSelectionKind
                                               selection) const
{
    switch (selection){
    case MapEditorSelectionKind::Land:
        return m_positionOnPlane;
    case MapEditorSelectionKind::Sprites:
        return m_positionOnPlane;
    case MapEditorSelectionKind::Objects:
        return m_positionOnPlane;
    default:
        return m_positionOnPlane;
    }
}

// -------------------------------------------------------

void ControlMapEditor::add(MapEditorSelectionKind selection,
                           MapEditorSubSelectionKind subSelection,
                           DrawKind drawKind,
                           QRect& tileset,
                           Position& p)
{
    if (tileset.width() != 0 && tileset.height() != 0) {
        switch (selection){
        case MapEditorSelectionKind::Land:
            addFloor(p, subSelection, drawKind, tileset);
            break;
        case MapEditorSelectionKind::Sprites:
            addSprite(p, subSelection, drawKind, tileset);
            break;
        case MapEditorSelectionKind::Objects:
            setCursorObjectPosition(p); break;
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::remove(MapEditorSelectionKind selection,
                              DrawKind drawKind,
                              Position& p)
{
    switch (selection){
    case MapEditorSelectionKind::Land:
        removeLand(p, drawKind);
        break;
    case MapEditorSelectionKind::Sprites:
        removeSprite(p, drawKind);
        break;
    case MapEditorSelectionKind::Objects:
        setCursorObjectPosition(p);
        showObjectMenuContext();
        break;
    }
}

// -------------------------------------------------------
//
//  Floors
//
// -------------------------------------------------------

void ControlMapEditor::addFloor(Position& p,
                                MapEditorSubSelectionKind kind,
                                DrawKind drawKind,
                                QRect &tileset)
{
    FloorDatas* floor;
    QRect* shortTexture;

    // Pencil
    switch (drawKind) {
    case DrawKind::Pencil:
        if (tileset.width() == 1 && tileset.width() == 1){
            QList<Position> positions;
            traceLine(m_previousMouseCoords, p, positions);
            for (int i = 0; i < positions.size(); i++){
                stockLand(positions[i],
                          new FloorDatas(new QRect(tileset.x(),
                                                   tileset.y(),
                                                   tileset.width(),
                                                   tileset.height())));
            }
        }
        for (int i = 0; i < tileset.width(); i++){
            if (p.x() + i > m_map->mapProperties()->length())
                break;

            for (int j = 0; j < tileset.height(); j++){
                if (p.z() + j > m_map->mapProperties()->width())
                    break;

                Position shortPosition(p.x() + i, 0, 0, p.z() + j, p.layer());
                shortTexture = new QRect(tileset.x() + i, tileset.y() + j,
                                         1, 1);
                floor = new FloorDatas(shortTexture);
                stockLand(shortPosition, floor);
            }
        }
        break;
    case DrawKind::Rectangle:
        break; // TODO
    case DrawKind::Pin:
        paintPinLand(p, kind, tileset);
        break;
    }

    m_previousMouseCoords = p;
}

// -------------------------------------------------------

void ControlMapEditor::paintPinLand(Position& p,
                                    MapEditorSubSelectionKind kindAfter,
                                    QRect& textureAfter)
{
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            LandDatas* landBefore = getLand(portion, p);
            MapEditorSubSelectionKind kindBefore =
                    MapEditorSubSelectionKind::None;
            QRect textureBefore;
            if (landBefore != nullptr){
                kindBefore = landBefore->getSubKind();
                getLandTexture(textureBefore, landBefore);
            }

            // If it's floor, we need to reduce to square * square size texture
            QRect textureAfterReduced;
            if (kindAfter == MapEditorSubSelectionKind::Floors)
                getFloorTextureReduced(textureAfter, textureAfterReduced, 0, 0);

            // If the texture à different, start the algorithm
            if (!areLandsEquals(landBefore, textureAfterReduced, kindAfter)){
                QList<Position> tab;
                tab.push_back(p);
                if (kindAfter == MapEditorSubSelectionKind::None)
                    eraseLand(p);
                else
                    stockLand(p, getLandAfter(kindAfter, textureAfterReduced));

                while(!tab.isEmpty()){
                    QList<Position> adjacent;
                    Position firstPosition = tab.at(0);
                    int x = firstPosition.x();
                    int y = firstPosition.y();
                    int yPlus = firstPosition.yPlus();
                    int z = firstPosition.z();
                    int layer = firstPosition.layer();

                    // Getting all the adjacent squares
                    adjacent << Position(x - 1, y, yPlus, z, layer);
                    adjacent << Position(x + 1, y, yPlus, z, layer);
                    adjacent << Position(x, y, yPlus, z + 1, layer);
                    adjacent << Position(x, y, yPlus, z - 1, layer);

                    tab.removeAt(0);
                    for (int i = 0; i < adjacent.size(); i++){
                        Position adjacentPosition = adjacent.at(i);
                        int localX = adjacentPosition.x() - p.x();
                        int localZ = adjacentPosition.z() - p.z();
                        getFloorTextureReduced(textureAfter,
                                               textureAfterReduced,
                                               localX, localZ);
                        if (m_map->isInGrid(adjacentPosition)){
                            portion = getLocalPortion(adjacentPosition);
                            if (m_map->isInPortion(portion)){
                                LandDatas* landHere = getLand(portion,
                                                              adjacentPosition);
                                if (areLandsEquals(landHere, textureBefore,
                                                   kindBefore))
                                {
                                    if (kindAfter ==
                                            MapEditorSubSelectionKind::None)
                                        eraseLand(adjacentPosition);
                                    else
                                        stockLand(adjacentPosition,
                                                  getLandAfter(
                                                      kindAfter,
                                                      textureAfterReduced));
                                    tab.push_back(adjacentPosition);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// -------------------------------------------------------

LandDatas* ControlMapEditor::getLand(Portion& portion, Position& p){
    MapPortion* mapPortion = m_map->mapPortion(portion);
    if (mapPortion == nullptr)
        return nullptr;
    return mapPortion->getLand(p);
}

// -------------------------------------------------------

void ControlMapEditor::getFloorTextureReduced(QRect& rect, QRect &rectAfter,
                                              int localX, int localZ)
{
    rectAfter.setX(rect.x() + Wanok::mod(localX, rect.width()));
    rectAfter.setY(rect.y() + Wanok::mod(localZ, rect.height()));
    rectAfter.setWidth(1);
    rectAfter.setHeight(1);
}

// -------------------------------------------------------

bool ControlMapEditor::areLandsEquals(LandDatas* landBefore,
                                      QRect& textureAfter,
                                      MapEditorSubSelectionKind kindAfter)
{
    if (landBefore == nullptr)
        return kindAfter == MapEditorSubSelectionKind::None;
    else{
        if (landBefore->getSubKind() == kindAfter){
            switch (kindAfter){
            case MapEditorSubSelectionKind::Floors:
                return (*(((FloorDatas*) landBefore)->textureRect())) ==
                        textureAfter;
            case MapEditorSubSelectionKind::Autotiles:
                return false; // TODO
            case MapEditorSubSelectionKind::Water:
                return false; // TODO
            default:
                return false;
            }
        }
        else
            return false;
    }
}

// -------------------------------------------------------

LandDatas* ControlMapEditor::getLandAfter(MapEditorSubSelectionKind kindAfter,
                                          QRect &textureAfter)
{
    QRect* texture = new QRect(textureAfter.x(),
                               textureAfter.y(),
                               textureAfter.width(),
                               textureAfter.height());
    switch (kindAfter) {
    case MapEditorSubSelectionKind::Floors:
        return new FloorDatas(texture);
    case MapEditorSubSelectionKind::Autotiles:
        return nullptr;
    case MapEditorSubSelectionKind::Water:
        return nullptr;
    default:
        return nullptr;
    }
}

// -------------------------------------------------------

void ControlMapEditor::getLandTexture(QRect& rect, LandDatas* land){
    FloorDatas* floor;

    switch (land->getSubKind()) {
    case MapEditorSubSelectionKind::Floors:
        floor = (FloorDatas*) land;
        rect.setX(floor->textureRect()->x());
        rect.setY(floor->textureRect()->y());
        rect.setWidth(floor->textureRect()->width());
        rect.setHeight(floor->textureRect()->height());
        break;
    case MapEditorSubSelectionKind::Autotiles:
        break;
    case MapEditorSubSelectionKind::Water:
        break;
    default:
        break;
    }
}

// -------------------------------------------------------

void ControlMapEditor::stockLand(Position& p, LandDatas *landDatas){
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            MapPortion* mapPortion = m_map->mapPortion(portion);
            if (mapPortion->addLand(p, landDatas) && m_map->saved())
                setToNotSaved();
            m_portionsToUpdate += portion;
            m_portionsToSave += portion;

            return;
        }
    }

    delete landDatas;
}

// -------------------------------------------------------

void ControlMapEditor::removeLand(Position& p, DrawKind drawKind) {
    QList<Position> positions;

    // Pencil
    switch (drawKind) {
    case DrawKind::Pencil:
        traceLine(m_previousMouseCoords, p, positions);
        for (int i = 0; i < positions.size(); i++)
            eraseLand(positions[i]);
        eraseLand(p);
    case DrawKind::Rectangle:
        break;
    case DrawKind::Pin:
        QRect tileset(0, 0, 1, 1);
        paintPinLand(p, MapEditorSubSelectionKind::None, tileset);
        break;
    }

    m_previousMouseCoords = p;
}

// -------------------------------------------------------

void ControlMapEditor::eraseLand(Position& p){
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            MapPortion* mapPortion = m_map->mapPortion(portion);
            if (mapPortion->deleteLand(p) && m_map->saved())
                setToNotSaved();
            m_portionsToUpdate += portion;
            m_portionsToSave += portion;
        }
    }
}

// -------------------------------------------------------
//
//  Sprites
//
// -------------------------------------------------------

void ControlMapEditor::addSprite(Position& p,
                                 MapEditorSubSelectionKind kind,
                                 DrawKind drawKind,
                                 QRect& tileset)
{
    QList<Position> positions;

    // Pencil
    switch (drawKind) {
    case DrawKind::Pencil:
    case DrawKind::Pin:
        traceLine(m_previousMouseCoords, p, positions);
        for (int i = 0; i < positions.size(); i++)
            stockSprite(positions[i], kind, 50, 0, new QRect(tileset));
        stockSprite(p, kind, 50, 0, new QRect(tileset));
        break;
    case DrawKind::Rectangle:
        break;
    }

    m_previousMouseCoords = p;
}

// -------------------------------------------------------

void ControlMapEditor::stockSprite(Position& p, MapEditorSubSelectionKind kind,
                                   int widthPosition, int angle,
                                   QRect *textureRect)
{
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            MapPortion* mapPortion = m_map->mapPortion(portion);
            if (mapPortion->addSprite(p, kind, widthPosition, angle,
                                      textureRect) &&
                m_map->saved())
            {
                setToNotSaved();
            }

            m_portionsToUpdate += portion;
            m_portionsToSave += portion;

            return;
        }
    }

    delete textureRect;
}

// -------------------------------------------------------

void ControlMapEditor::removeSprite(Position& p, DrawKind drawKind) {
    QList<Position> positions;

    // Pencil
    switch (drawKind) {
    case DrawKind::Pencil:
    case DrawKind::Pin:
        traceLine(m_previousMouseCoords, p, positions);
        for (int i = 0; i < positions.size(); i++)
            eraseSprite(positions[i]);
        eraseSprite(p);
        break;
    case DrawKind::Rectangle:
        break;
    }

    m_previousMouseCoords = p;
}

// -------------------------------------------------------

void ControlMapEditor::eraseSprite(Position& p){
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            MapPortion* mapPortion = m_map->mapPortion(portion);
            if (mapPortion->deleteSprite(p) && m_map->saved())
                setToNotSaved();
            m_portionsToUpdate += portion;
            m_portionsToSave += portion;
        }
    }
}

// -------------------------------------------------------
//
//  Objects
//
// -------------------------------------------------------

void ControlMapEditor::setCursorObjectPosition(Position& p){
    m_cursorObject->setX(p.x());
    m_cursorObject->setZ(p.z());

    Portion portion = getLocalPortion(p);
    if (m_map->isInPortion(portion)){
        m_selectedObject = nullptr;
        MapObjects* mapObjects = m_map->objectsPortion(portion);

        // Generate object informations
        if (mapObjects != nullptr)
            m_selectedObject = mapObjects->getObjectAt(p);
    }
}

// -------------------------------------------------------

void ControlMapEditor::showObjectMenuContext(){
    bool isEmpty = m_selectedObject == nullptr;

    // Edit possible actions
    m_contextMenu->canNew(isEmpty);
    m_contextMenu->canEdit(!isEmpty);
    m_contextMenu->canCopy(false);
    m_contextMenu->canPaste(false);
    m_contextMenu->canDelete(!isEmpty);
    m_contextMenu->canHero(!isEmpty);

    // Show menu context
    m_contextMenu->showContextMenu(m_mouse);
}

// -------------------------------------------------------

void ControlMapEditor::defineAsHero(){
   SystemDatas* datas = Wanok::get()->project()->gameDatas()->systemDatas();
   datas->setIdMapHero(m_map->mapProperties()->id());
   datas->setIdObjectHero(m_selectedObject->id());
   Wanok::get()->project()->writeGameDatas();
}

// -------------------------------------------------------

void ControlMapEditor::addObject(Position& p){
    Portion portion = getLocalPortion(p);
    SystemCommonObject* object = new SystemCommonObject;

    if (m_selectedObject != nullptr)
        object->setCopy(*m_selectedObject);
    else{
        object->setDefault();
        int id = m_map->generateObjectId();
        object->setId(id);
        object->setName(Map::generateObjectName(id));
    }

    // Open the dialog box
    DialogObject dialog(object);
    if (dialog.exec() == QDialog::Accepted){
        MapPortion* mapPortion = m_map->mapPortion(portion);
        if (m_map->addObject(p, mapPortion, object) &&
            m_map->saved())
        {
            setToNotSaved();
        }
        m_selectedObject = object;
        m_map->writeObjects(true);
        saveTempPortion(portion);
    }
    else
        delete object;
}

// -------------------------------------------------------

void ControlMapEditor::removeObject(Position& p){
    if (m_map->isInGrid(p)){
        Portion portion = getLocalPortion(p);
        if (m_map->isInPortion(portion)){
            MapPortion* mapPortion = m_map->mapPortion(portion);
            MapObjects* mapObjects = m_map->objectsPortion(portion);
            SystemCommonObject* object;
            if (mapObjects != nullptr)
                object = mapObjects->getObjectAt(p);

            if (object != nullptr){
                if (m_map->deleteObject(p, mapPortion, object) &&
                    m_map->saved())
                {
                    setToNotSaved();
                }

                m_map->writeObjects(true);
                m_portionsToUpdate += portion;
                m_portionsToSave += portion;
            }
        }
    }
}

// -------------------------------------------------------

void ControlMapEditor::traceLine(Position& previousCoords, Position& coords,
                                 QList<Position>& positions)
{
    int x1 = previousCoords.x(), x2 = coords.x();
    int y = coords.y();
    int yPlus = coords.yPlus();
    int z1 = previousCoords.z(), z2 = coords.z();
    int l = coords.layer();
    int dx = x2 - x1, dz = z2 - z1;
    bool test = true;

    if (dx != 0){
        if (dx > 0){
            if (dz != 0){
                if (dz > 0){
                    if (dx >= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1++;
                            if (x1 == x2) break;
                            e -= dz;
                            if (e < 0){
                                z1++;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1++;
                            if (z1 == z2) break;
                            e -= dx;
                            if (e < 0){
                                x1++;
                                e += dz;
                            }
                        }
                    }
                }
                else{
                    if (dx >= -dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1++;
                            if (x1 == x2) break;
                            e += dz;
                            if (e < 0){
                                z1--;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1--;
                            if (z1 == z2) break;
                            e += dx;
                            if (e > 0){
                                x1++;
                                e += dz;
                            }
                        }
                    }
                }
            }
            else{
                while (x1 != x2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    x1++;
                }
            }
        }
        else{
            dz = z2 - z1;
            if (dz != 0){
                if (dz > 0){
                    if (-dx >= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1--;
                            if (x1 == x2) break;
                            e += dz;
                            if (e >= 0){
                                z1++;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1++;
                            if (z1 == z2) break;
                            e += dx;
                            if (e <= 0)
                            {
                                x1--;
                                e += dz;
                            }
                        }
                    }
                }
                else{
                    if (dx <= dz){
                        int e = dx;
                        dx = 2 * e;
                        dz = dz * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            x1--;
                            if (x1 == x2) break;
                            e -= dz;
                            if (e >= 0){
                                z1--;
                                e += dx;
                            }
                        }
                    }
                    else{
                        int e = dz;
                        dz = 2 * e;
                        dx = dx * 2;

                        while (test){
                            positions.push_back(Position(x1, y, yPlus, z1, l));
                            z1--;
                            if (z1 == z2) break;
                            e -= dx;
                            if (e >= 0){
                                x1--;
                                e += dz;
                            }
                        }
                    }
                }
            }
            else{
                while (x1 != x2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    x1--;
                }
            }
        }
    }
    else{
        dz = z2 - z1;
        if (dz != 0){
            if (dz > 0){
                while(z1 != z2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    z1++;
                }
            }
            else{
                while (z1 != z2){
                    positions.push_back(Position(x1, y, yPlus, z1, l));
                    z1--;
                }
            }
        }
    }
}

// -------------------------------------------------------
//
//  OTHERS
//
// -------------------------------------------------------

Portion ControlMapEditor::getGlobalPortion(Position& position) const{
    return Portion(
                position.x() / Wanok::portionSize,
                position.y() / Wanok::portionSize,
                position.z() / Wanok::portionSize);
}

// -------------------------------------------------------

Portion ControlMapEditor::getLocalPortion(Position& position) const{
    return Portion(
                (position.x() / Wanok::portionSize) -
                (m_cursor->getSquareX() / Wanok::portionSize),
                (position.y() / Wanok::portionSize) -
                (m_cursor->getSquareY() / Wanok::portionSize),
                (position.z() / Wanok::portionSize) -
                (m_cursor->getSquareZ() / Wanok::portionSize));
}

// -------------------------------------------------------

Portion ControlMapEditor::getGlobalFromLocalPortion(Portion& portion) const{
    return Portion(
                portion.x() + (m_cursor->getSquareX() / Wanok::portionSize),
                portion.y() + (m_cursor->getSquareY() / Wanok::portionSize),
                portion.z() + (m_cursor->getSquareZ() / Wanok::portionSize));
}

// -------------------------------------------------------
//
//  GL
//
// -------------------------------------------------------

void ControlMapEditor::paintGL(QMatrix4x4 &modelviewProjection,
                               QVector3D &cameraRightWorldSpace,
                               QVector3D &cameraUpWorldSpace,
                               MapEditorSelectionKind selectionKind)
{
    m_map->paintFloors(modelviewProjection);

    if (selectionKind == MapEditorSelectionKind::Objects)
        m_cursorObject->paintGL(modelviewProjection);

    m_cursor->paintGL(modelviewProjection);

    if (m_displayGrid){
        glDisable(GL_DEPTH_TEST);
        m_grid->paintGL(modelviewProjection);
        glEnable(GL_DEPTH_TEST);
    }

    m_map->paintOthers(modelviewProjection, cameraRightWorldSpace,
                       cameraUpWorldSpace);
}

// -------------------------------------------------------
//
//  EVENTS
//
// -------------------------------------------------------

void ControlMapEditor::onMouseWheelMove(QWheelEvent* event, bool updateTree){
    if (event->delta() > 0)
        m_camera->zoomPlus(0);
    else
        m_camera->zoomLess(0);

    if (updateTree)
        updateCameraTreeNode();
}

// -------------------------------------------------------

void ControlMapEditor::onMouseMove(QPoint point,
                                   Qt::MouseButton button, bool updateTree)
{
    updateMousePosition(point);
    m_mouseMove = point;

    if (button == Qt::MouseButton::MiddleButton){
        m_camera->onMouseWheelPressed(m_mouseMove, m_mouseBeforeUpdate);
        if (updateTree)
            updateCameraTreeNode();
    }
}

// -------------------------------------------------------

void ControlMapEditor::onMousePressed(MapEditorSelectionKind selection,
                                      MapEditorSubSelectionKind subSelection,
                                      DrawKind drawKind,
                                      QRect &tileset,
                                      QPoint point,
                                      Qt::MouseButton button)
{
    updateMousePosition(point);
    m_mouseMove = point;

    // Update
    update();

    // Add/Remove something
    m_previousMouseCoords = getPositionSelected(selection);
    addRemove(selection, subSelection, drawKind, tileset, button);
}

// -------------------------------------------------------

void ControlMapEditor::onKeyPressed(int k, double speed){
    m_cursor->onKeyPressed(k, m_camera->horizontalAngle(),
                           m_map->mapProperties()->length(),
                           m_map->mapProperties()->width(), speed);
}

// -------------------------------------------------------

void ControlMapEditor::onKeyPressedWithoutRepeat(int k){
    if (k == Qt::Key_G){
        m_displayGrid = !m_displayGrid;
    }
}

// -------------------------------------------------------

void ControlMapEditor::onKeyReleased(int ){

}
