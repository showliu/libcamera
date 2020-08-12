/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * renderer.h - Render YUV format frame by OpenGL shader
 */
#ifndef __QCAM_RENDERER_H__
#define __QCAM_RENDERER_H__

#include <QImage>
#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QSize>
#include <QSurfaceFormat>

#include <libcamera/formats.h>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

class Renderer : public QOffscreenSurface, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	Renderer();
	~Renderer();

	void initializeGL();
	bool configure(const libcamera::PixelFormat &format, const QSize &size);
	void render(unsigned char *buffer);
	QImage toImage();

private:
	bool createShader();
	void configureTexture(unsigned int id);
	void removeShader();
	bool selectShader(const libcamera::PixelFormat &format);

	/* OpenGL renderer components */
	QOpenGLContext ctx_;
	QOpenGLFramebufferObject *fbo_;
	QOpenGLBuffer vbo_;
	QOpenGLShader *pFShader_;
	QOpenGLShader *pVShader_;
	QOpenGLShaderProgram shaderProgram_;
	QSurfaceFormat surfaceFormat_;

	/* Fragment and Vertex shader file */
	QString fsrc_;
	QString vsrc_;

	/* YUV frame size and format */
	libcamera::PixelFormat format_;
	QSize size_;

	/* YUV texture planars and parameters*/
	GLuint id_u_;
	GLuint id_v_;
	GLuint id_y_;
	GLuint textureUniformU_;
	GLuint textureUniformV_;
	GLuint textureUniformY_;
	QOpenGLTexture textureU_;
	QOpenGLTexture textureV_;
	QOpenGLTexture textureY_;
	unsigned int horzSubSample_;
	unsigned int vertSubSample_;

	QImage image_;
	QMutex mutex_; /* Prevent concurrent access to image_ */
};

#endif /* __QCAM_RENDERER_H__ */
