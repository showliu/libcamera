/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * viewfinder.h - qcam - Viewfinder
 */
#ifndef __QCAM_VIEWFINDER_H__
#define __QCAM_VIEWFINDER_H__

#include <QMutex>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

#include "format_converter.h"

class QImage;

class ViewFinder : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
	ViewFinder(QWidget *parent);
	~ViewFinder();

	int setFormat(unsigned int format, unsigned int width,
		      unsigned int height);
	void display(const unsigned char *rgb, size_t size);

	QImage getCurrentImage();

	void setFrameSize(int width, int height);
	void updateFrame(const unsigned char  *buffer);
	void setGPUConvertFlag();

protected:
	void paintEvent(QPaintEvent *) override;
	QSize sizeHint() const override;

	void        initializeGL() Q_DECL_OVERRIDE;
	void        paintGL() Q_DECL_OVERRIDE;
	void        resizeGL(int w, int h) Q_DECL_OVERRIDE;

private:
	unsigned int format_;
	unsigned int width_;
	unsigned int height_;

	FormatConverter converter_;

	QImage *image_;
	QMutex mutex_; /* Prevent concurrent access to image_ */

	QOpenGLShader *pVShader;
	QOpenGLShader *pFShader;
	QOpenGLShaderProgram shaderProgram;

	GLuint textureUniformY;
	GLuint textureUniformU;
	GLuint textureUniformV;
	GLuint id_y;
	GLuint id_u;
	GLuint id_v;
	QOpenGLTexture textureY;
	QOpenGLTexture textureU;
	QOpenGLTexture textureV;

	int frameW;
	int frameH;
	unsigned char* yuvDataPtr;

	QOpenGLBuffer glBuffer;

	bool isGPUConvert_;
};

#endif /* __QCAM_VIEWFINDER__ */
