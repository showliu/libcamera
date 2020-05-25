/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * viewfinderGL.h - Render YUV format frame by OpenGL shader
 */
#ifndef __VIEWFINDERGL_H__
#define __VIEWFINDERGL_H__

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <QImage>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QSize>
#include <QOpenGLTexture>
#include <QOpenGLWidget>

#include <libcamera/buffer.h>
#include <libcamera/pixel_format.h>
#include "viewfinder.h"

class ViewFinderGL : public QOpenGLWidget,
					 public ViewFinderHandler,
					 protected QOpenGLFunctions
{
	Q_OBJECT

public:
	ViewFinderGL(QWidget *parent = 0);
	~ViewFinderGL();

	int setFormat(const libcamera::PixelFormat &format, const QSize &size);
	void render(libcamera::FrameBuffer *buffer, MappedBuffer *map);
	void stop();

	QImage getCurrentImage();

	void setFrameSize(int width, int height);
	void updateFrame(unsigned char  *buffer);

	char *selectFragmentShader(unsigned int format);
	void createFragmentShader();
	void render();

Q_SIGNALS:
	void renderComplete(libcamera::FrameBuffer *buffer);

protected:
	void initializeGL() Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;
	void resizeGL(int w, int h) Q_DECL_OVERRIDE;
	QSize sizeHint() const override;

private:

	void configureTexture(unsigned int id);
	void removeShader();

	/* Captured image size, format and buffer */
	libcamera::FrameBuffer *buffer_;
	libcamera::PixelFormat format_;
	QOpenGLBuffer glBuffer;
	QSize size_;

	GLuint id_u;
	GLuint id_v;
	GLuint id_y;

	QMutex mutex_; /* Prevent concurrent access to image_ */

	QOpenGLShader *pFShader;
	QOpenGLShader *pVShader;
	QOpenGLShaderProgram shaderProgram;

	GLuint textureUniformU;
	GLuint textureUniformV;
	GLuint textureUniformY;

	QOpenGLTexture textureU;
	QOpenGLTexture textureV;
	QOpenGLTexture textureY;

	unsigned int width_;
	unsigned int height_;

	unsigned char* yuvDataPtr;

	/* NV parameters */
	unsigned int horzSubSample_ ;
	unsigned int vertSubSample_;
};
#endif // __VIEWFINDERGL_H__
