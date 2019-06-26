/*
    RPG Paper Maker Copyright (C) 2017-2019 Marie Laporte

    RPG Paper Maker engine is under LGPL-3 license.

    Commercial license for commercial use of your games:
        https://creativecommons.org/licenses/by-nc/4.0/.

    See more information here: http://rpg-paper-maker.com/index.php/downloads.
*/

#ifndef SPRITES_H
#define SPRITES_H

#include "sprite.h"

// -------------------------------------------------------
//
//  CLASS SpritesWalls
//
//  A set of sprites walls in a portion of the map.
//
// -------------------------------------------------------

class SpritesWalls : protected QOpenGLFunctions
{
public:
    SpritesWalls();
    virtual ~SpritesWalls();

    void initializeVertices(Position& position, SpriteWallDatas* sprite,
                            int squareSize, int width, int height);
    void initializeGL(QOpenGLShaderProgram* program);
    void updateGL();
    void paintGL();

protected:
    int m_count;

    // OpenGL
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QVector<Vertex> m_vertices;
    QVector<GLuint> m_indexes;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram* m_program;
};

// -------------------------------------------------------
//
//  CLASS Sprites
//
//  A set of sprites in a portion of the map.
//
// -------------------------------------------------------

class Sprites : public Serializable, protected QOpenGLFunctions
{
public:
    Sprites();
    virtual ~Sprites();
    void addOverflow(Position& p);
    void removeOverflow(Position& p);
    bool isEmpty() const;
    void updateEmpty(bool previewSquare);
    bool contains(Position& position) const;
    void changePosition(Position& position, Position& newPosition);
    SpriteDatas* spriteAt(Position& position) const;
    void setSprite(QSet<Portion>& portionsOverflow, Position& p,
                   SpriteDatas* sprite);
    static void getSetPortionsOverflow(
            QSet<Portion>& portionsOverflow, Position& p, SpriteDatas* sprite);
    void addRemoveOverflow(
            QSet<Portion>& portionsOverflow, Position &p, bool add);
    SpriteDatas* removeSprite(QSet<Portion> &portionsOverflow, Position& p);
    bool addSprite(QSet<Portion> &portionsOverflow, Position& p,
                   SpriteDatas *sprite, QJsonObject &previousObj,
                   MapEditorSubSelectionKind &previousType);
    bool deleteSprite(QSet<Portion> &portionsOverflow, Position& p,
                      QJsonObject &previousObj,
                      MapEditorSubSelectionKind &previousType);
    void setSpriteWall(Position& p, SpriteWallDatas* sprite);
    SpriteWallDatas* removeSpriteWall(Position& p);
    bool addSpriteWall(Position& p, SpriteWallDatas *sprite,
                       QJsonObject &previousObj,
                       MapEditorSubSelectionKind &previousType);
    bool deleteSpriteWall(Position& p, QJsonObject &previousObj,
                          MapEditorSubSelectionKind &previousType);
    void updateSpriteWalls(QHash<Position, MapElement*>& preview,
                           QList<Position> &previewDelete);
    SpriteWallDatas* getWallAt(QHash<Position, MapElement*>& preview,
                               QList<Position> &previewDelete,
                               Position& position);
    SpriteWallDatas* getWallAtPosition(Position& position);
    void getWallsWithPreview(QHash<Position, SpriteWallDatas*>&
                             spritesWallWithPreview,
                             QHash<Position, MapElement *>& preview,
                             QList<Position>& previewDelete);
    void removeSpritesOut(MapProperties& properties);
    MapElement *updateRaycasting(int squareSize, float& finalDistance,
                                 Position &finalPosition, QRay3D &ray,
                                 double cameraHAngle, bool layerOn);
    bool updateRaycastingAt(
            Position &position, SpriteDatas* sprite, int squareSize,
            float &finalDistance, Position &finalPosition, QRay3D& ray,
            double cameraHAngle);
    bool updateRaycastingWallAt(
            Position &position, SpriteWallDatas* wall,
            float &finalDistance, Position &finalPosition, QRay3D& ray);
    MapElement* getMapElementAt(Position& position,
                                MapEditorSubSelectionKind subKind);
    int getLastLayerAt(Position& position) const;
    void updateRemoveLayer(
            QSet<Portion> portionsOverflow, Position& position,
            QList<QJsonObject> previous,
            QList<MapEditorSubSelectionKind> previousType,
            QList<Position> positions);

    void initializeVertices(QHash<int, QOpenGLTexture*>& texturesWalls,
                            QHash<Position, MapElement*>& previewSquares,
                            QList<Position>& previewDelete,
                            int squareSize, int width, int height);
    void initializeGL(QOpenGLShaderProgram* programStatic,
                      QOpenGLShaderProgram* programFace);
    void updateGL();
    void paintGL();
    void paintFaceGL();
    void paintSpritesWalls(int textureID);

    virtual void read(const QJsonObject &json);
    virtual void write(QJsonObject &json) const;

protected:
    QHash<Position, SpriteDatas*> m_all;
    QHash<Position, SpriteWallDatas*> m_walls;
    QHash<int, SpritesWalls*> m_wallsGL;
    QSet<Position> m_overflow;
    bool m_isEmpty;

    // OpenGL static
    QOpenGLBuffer m_vertexBufferStatic;
    QOpenGLBuffer m_indexBufferStatic;
    QVector<Vertex> m_verticesStatic;
    QVector<GLuint> m_indexesStatic;
    QOpenGLVertexArrayObject m_vaoStatic;
    QOpenGLShaderProgram* m_programStatic;

    // OpenGL face
    QOpenGLBuffer m_vertexBufferFace;
    QOpenGLBuffer m_indexBufferFace;
    QVector<VertexBillboard> m_verticesFace;
    QVector<GLuint> m_indexesFace;
    QOpenGLVertexArrayObject m_vaoFace;
    QOpenGLShaderProgram* m_programFace;
};

#endif // SPRITES_H