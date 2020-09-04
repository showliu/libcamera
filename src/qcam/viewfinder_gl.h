/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * viewfinder_GL.h - OpenGL Viewfinder for rendering by OpenGL shader
 *
 */
#ifndef __VIEWFINDER_GL_H__
#define __VIEWFINDER_GL_H__

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QSize>

#include <libcamera/buffer.h>
#include <libcamera/formats.h>
#include "viewfinder.h"

class ViewFinderGL : public QOpenGLWidget,
		     public ViewFinder,
		     protected QOpenGLFunctions
{
	Q_OBJECT

public:
	ViewFinderGL(QWidget *parent = 0);
	~ViewFinderGL();

	const QList<libcamera::PixelFormat> &nativeFormats() const override;

	int setFormat(const libcamera::PixelFormat &format, const QSize &size) override;
	void render(libcamera::FrameBuffer *buffer, MappedBuffer *map) override;
	void stop() override;

	QImage getCurrentImage() override;

Q_SIGNALS:
	void renderComplete(libcamera::FrameBuffer *buffer);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
	QSize sizeHint() const override;

private:
	bool isFormatSupport(const libcamera::PixelFormat &format);

	void configureTexture(unsigned int id);
	bool createFragmentShader();
	void createVertexShader();
	void removeShader();
	void doRender();

	/* Captured image size, format and buffer */
	libcamera::FrameBuffer *buffer_;
	libcamera::PixelFormat format_;
	QSize size_;

	/* OpenGL components for render */
	QOpenGLShader *pFShader_;
	QOpenGLShader *pVShader_;
	QOpenGLShaderProgram shaderProgram_;

	/* Vertex buffer */
	QOpenGLBuffer vbuf_;

	/* Fragment and Vertex shader file name */
	QString fsrc_;
	QString vsrc_;

	unsigned char *yuvDataPtr_;

	/* YUV texture planars and parameters */
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
#endif /* __VIEWFINDER_GL_H__ */
