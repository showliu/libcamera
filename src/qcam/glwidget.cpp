/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * glwidget.cpp - Rendering YUV frame by OpenGL shader
 */

#include "glwidget.h"
#include <QImage>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

GLWidget::GLWidget(QWidget *parent)
	: QOpenGLWidget(parent),
	textureY(QOpenGLTexture::Target2D),
	textureU(QOpenGLTexture::Target2D),
	textureV(QOpenGLTexture::Target2D),
	yuvDataPtr(NULL),
	glBuffer(QOpenGLBuffer::VertexBuffer)
{
}

GLWidget::~GLWidget()
{
	removeShader();

	textureY.destroy();
	textureU.destroy();
	textureV.destroy();

	glBuffer.destroy();
}

void GLWidget::updateFrame(unsigned char  *buffer)
{
	if(buffer) {
		yuvDataPtr = buffer;
		update();
	}
}

void GLWidget::setFrameSize(int width, int height)
{
	if(width > 0 && height > 0) {
		width_ = width;
		height_ = height;
	}
}

int GLWidget::configure(unsigned int format, unsigned int width,
						unsigned int height)
{
	switch (format) {
	case DRM_FORMAT_NV12:
	break;
	case DRM_FORMAT_NV21:
	break;
	default:
		return -EINVAL;
	};
	format_ = format;
	width_ = width;
	height_ = height;

	return 0;
}

void GLWidget::createShader()
{
	bool ret = false;

	QString vsrc =  "attribute vec4 vertexIn; \n"
					"attribute vec2 textureIn; \n"
					"varying vec2 textureOut; \n"
					"void main(void) \n"
					"{ \n"
						"gl_Position = vertexIn; \n"
						"textureOut = textureIn; \n"
					"} \n";

	ret = shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
	if(!ret) {
		qDebug() << __func__ << ":" << shaderProgram.log();
	}

	QString fsrc =  "#ifdef GL_ES \n"
					"precision highp float; \n"
					"#endif \n"
					"varying vec2 textureOut; \n"
					"uniform sampler2D tex_y; \n"
					"uniform sampler2D tex_uv; \n"
					"void main(void) \n"
					"{ \n"
					"vec3 yuv; \n"
					"vec3 rgb; \n"
					"yuv.x = texture2D(tex_y, textureOut).r - 0.0625; \n"
					"yuv.y = texture2D(tex_uv, textureOut).r - 0.5; \n"
					"yuv.z = texture2D(tex_uv, textureOut).g - 0.5; \n"
					"rgb = mat3( 1.0,       1.0,         1.0, \n"
					 "           0.0,       -0.39465,  2.03211, \n"
					 "           1.13983, -0.58060,  0.0) * yuv; \n"
					"gl_FragColor = vec4(rgb, 1.0); \n"
					"}\n";
	ret = shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
	if(!ret) {
		qDebug() << __func__ << ":" << shaderProgram.log();
	}

	// Link shader pipeline
	if (!shaderProgram.link()) {
		qDebug() << __func__ << ": shader program link failed.\n" << shaderProgram.log();
		close();
	}

	// Bind shader pipeline for use
	if (!shaderProgram.bind()) {
		qDebug() << __func__ << ": shader program binding failed.\n" << shaderProgram.log();
		close();
	}

}

void GLWidget::configureTexture(unsigned int id)
{
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void GLWidget::removeShader()
{
	if (shaderProgram.isLinked()) {
		shaderProgram.release();
		shaderProgram.removeAllShaders();
	}
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();


	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	static const GLfloat vertices[] {
		-1.0f,-1.0f,
		-1.0f,+1.0f,
		+1.0f,+1.0f,
		+1.0f,-1.0f,
		0.0f,1.0f,
		0.0f,0.0f,
		1.0f,0.0f,
		1.0f,1.0f,
	};

	glBuffer.create();
	glBuffer.bind();
	glBuffer.allocate(vertices,sizeof(vertices));

	createShader();

	shaderProgram.enableAttributeArray(ATTRIB_VERTEX);
	shaderProgram.enableAttributeArray(ATTRIB_TEXTURE);

	shaderProgram.setAttributeBuffer(ATTRIB_VERTEX,GL_FLOAT,0,2,2*sizeof(GLfloat));
	shaderProgram.setAttributeBuffer(ATTRIB_TEXTURE,GL_FLOAT,8*sizeof(GLfloat),2,2*sizeof(GLfloat));

	textureUniformY = shaderProgram.uniformLocation("tex_y");
	textureUniformU = shaderProgram.uniformLocation("tex_uv");

	textureY.create();
	textureU.create();

	id_y = textureY.textureId();
	id_u = textureU.textureId();

	configureTexture(id_y);
	configureTexture(id_u);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

void GLWidget::paintGL()
{
	if(yuvDataPtr)
	{
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// activate texture Y
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, id_y);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width_, height_, 0, GL_RED, GL_UNSIGNED_BYTE, yuvDataPtr);
		glUniform1i(textureUniformY, 0);

		// activate texture UV
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, id_u);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width_/2, height_/2, 0, GL_RG, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+width_*height_);
		glUniform1i(textureUniformU, 1);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void GLWidget::resizeGL(int w, int h)
{
	glViewport(0,0,w,h);
}
