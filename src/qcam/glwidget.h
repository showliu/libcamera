#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * glwidget.h - Rendering YUV frame by OpenGL shader
 */
#include <QOpenGLTexture>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <QOpenGLBuffer>
#include <QImage>

#include <linux/drm_fourcc.h>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	GLWidget(QWidget *parent = 0);
	~GLWidget();

	void        setFrameSize(int width, int height);
	void        updateFrame(unsigned char  *buffer);

	int configure(unsigned int format, unsigned int width,
					unsigned int height);

	void createShader();

protected:
	void        initializeGL() Q_DECL_OVERRIDE;
	void        paintGL() Q_DECL_OVERRIDE;
	void        resizeGL(int w, int h) Q_DECL_OVERRIDE;

private:

	void configureTexture(unsigned int id);
	void removeShader();

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

	unsigned int format_;
	unsigned int width_;
	unsigned int height_;

	unsigned char* yuvDataPtr;

	QOpenGLBuffer glBuffer;
};
#endif // GLWINDOW_H
