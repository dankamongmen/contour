/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <terminal/Color.h>

#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShader>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QVector4D>
#include <QtGui/QOpenGLFunctions_3_2_Core>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLVertexArrayObject>

namespace terminal::view {

/// OpenGL Object for rendering character cell's background.
class CellBackground : public QOpenGLFunctions_3_2_Core {
  public:
    CellBackground(QSize _size, QMatrix4x4 _projectionMatrix);
    ~CellBackground();

    void setProjection(QMatrix4x4 const& _projectionMatrix);
    void resize(QSize _size);
    void render(QPoint _pos, QVector4D const& _color);

  private:
    QMatrix4x4 projectionMatrix_;
    QOpenGLShaderProgram shader_;
    GLint transformLocation_;
    GLint colorLocation_;
    QOpenGLBuffer vbo_;
    QOpenGLVertexArrayObject vao_;
};

} // namespace terminal::view
