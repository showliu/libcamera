/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * viewfinder.cpp - qcam - Viewfinder
 */

#include <QImage>
#include <QImageWriter>
#include <QMutexLocker>
#include <QPainter>

#include <linux/drm_fourcc.h>
#include "format_converter.h"
#include "viewfinder.h"

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

ViewFinder::ViewFinder(QWidget *parent)
	: QOpenGLWidget(parent), format_(0), width_(0), height_(0), image_(nullptr),
	textureY(QOpenGLTexture::Target2D),
    textureU(QOpenGLTexture::Target2D),
    textureV(QOpenGLTexture::Target2D),
    yuvDataPtr(NULL),
    glBuffer(QOpenGLBuffer::VertexBuffer),
    isGPUConvert_(true)
{
}

ViewFinder::~ViewFinder()
{
	delete image_;

	delete pVShader;
	delete pFShader;

	textureY.destroy();
	textureU.destroy();
	textureV.destroy();

	glBuffer.destroy();
}

void ViewFinder::display(const unsigned char *raw, size_t size)
{
	QMutexLocker locker(&mutex_);

	/*
	 * \todo We're not supposed to block the pipeline handler thread
	 * for long, implement a better way to save images without
	 * impacting performances.
	 */
	if(isGPUConvert_){
		updateFrame(raw);
	} else {
		converter_.convert(raw, size, image_);
		update();
	}
}

QImage ViewFinder::getCurrentImage()
{
	QMutexLocker locker(&mutex_);

	return image_->copy();
}

int ViewFinder::setFormat(unsigned int format, unsigned int width,
			  unsigned int height)
{
	int ret;

	ret = converter_.configure(format, width, height);
	if (ret < 0)
		return ret;

	format_ = format;
	width_ = width;
	height_ = height;

	delete image_;
	image_ = new QImage(width, height, QImage::Format_RGB32);

	if(isGPUConvert_){
		setFrameSize( width_, height_);
	}

	updateGeometry();
	return 0;
}

void ViewFinder::paintEvent(QPaintEvent *)
{
	if(isGPUConvert_) {
		paintGL();
	} else {
		QPainter painter(this);
		painter.drawImage(rect(), *image_, image_->rect());
	}
}

QSize ViewFinder::sizeHint() const
{
	return image_ ? image_->size() : QSize(640, 480);
}

void ViewFinder::initializeGL()
{
	bool bCompile;
	initializeOpenGLFunctions();

	glEnable(GL_DEPTH_TEST);

	static const GLfloat vertices[]{
		//Vertex
		-1.0f,-1.0f,
		-1.0f,+1.0f,
		+1.0f,+1.0f,
		+1.0f,-1.0f,
		//Texture
		0.0f,1.0f,
		0.0f,0.0f,
		1.0f,0.0f,
		1.0f,1.0f,
	};

	glBuffer.create();
	glBuffer.bind();
	glBuffer.allocate(vertices,sizeof(vertices));

	pVShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
	const char *vsrc =  "attribute vec4 vertexIn; \
                        attribute vec2 textureIn; \
                        varying vec2 textureOut;  \
                        void main(void)           \
                        {                         \
                            gl_Position = vertexIn; \
                            textureOut = textureIn; \
                        }";

	bCompile = pVShader->compileSourceCode(vsrc);
	if(!bCompile)
	{
		qDebug() << __FILE__ << ":" << __func__<< ": Compile Vertex Shader fail";
	}

	pFShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
	const char *fsrc =  "#ifdef GL_ES               \n"
                        "   precision mediump float;   \n"
                        "#endif \n"
                        "varying vec2 textureOut; \
                        uniform sampler2D tex_y; \
                        uniform sampler2D tex_u; \
                        uniform sampler2D tex_v; \
                        void main(void) \
                        { \
                        vec3 yuv; \
                        vec3 rgb; \
                        yuv.x = texture2D(tex_y, textureOut).r; \
                        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
                        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
                        rgb = mat3( 1,       1,         1, \
                                    0,       -0.39465,  2.03211, \
                                    1.13983, -0.58060,  0) * yuv; \
                        gl_FragColor = vec4(rgb, 1); \
                        }";

	bCompile = pFShader->compileSourceCode(fsrc);
	if(!bCompile)
	{
		qDebug() << __FILE__ << ":" << __func__<< ": Compile Fragment Shader fail";
	}

	shaderProgram.addShader(pVShader);
	shaderProgram.addShader(pFShader);

	shaderProgram.bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
	shaderProgram.bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

        // Link shader pipeline
	if (!shaderProgram.link())
		close();

	// Bind shader pipeline for use
	if (!shaderProgram.bind())
		close();

	shaderProgram.enableAttributeArray(ATTRIB_VERTEX);
	shaderProgram.enableAttributeArray(ATTRIB_TEXTURE);

	shaderProgram.setAttributeBuffer(ATTRIB_VERTEX,GL_FLOAT,0,2,2*sizeof(GLfloat));
	shaderProgram.setAttributeBuffer(ATTRIB_TEXTURE,GL_FLOAT,8*sizeof(GLfloat),2,2*sizeof(GLfloat));

	textureUniformY = shaderProgram.uniformLocation("tex_y");
	textureUniformU = shaderProgram.uniformLocation("tex_u");
	textureUniformV = shaderProgram.uniformLocation("tex_v");

	textureY.create();
	textureU.create();
	textureV.create();

	id_y = textureY.textureId();
	id_u = textureU.textureId();
	id_v = textureV.textureId();

	glBindTexture(GL_TEXTURE_2D, id_y);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, id_u);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, id_v);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void ViewFinder::paintGL()
{
	if(yuvDataPtr)
	{
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// activate texture 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, id_y);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameW, frameH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvDataPtr);
		glUniform1i(textureUniformY, 0);

		// activate texture 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, id_u);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameW/2, frameH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+frameW*frameH);
		glUniform1i(textureUniformU, 1);

		// activate texture 2
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, id_v);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameW/2, frameH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+frameW*frameH*5/4);
		glUniform1i(textureUniformV, 2);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void ViewFinder::resizeGL(int w, int h)
{
	glViewport(0,0,w,h);

	int err = glGetError();
	if (err) qDebug("OpenGL Error 0x%x: %s.\n", err, __func__);
}

void ViewFinder::updateFrame(const unsigned char  *buffer)
{
	if(buffer)
	{
		yuvDataPtr = (unsigned char *)buffer;
		update();
	}
}

void ViewFinder::setFrameSize(int width, int height)
{
	if(width > 0 && height > 0)
	{
		frameW = width;
		frameH = height;
	}
}

void ViewFinder::setGPUConvertFlag()
{
	/* only support NV family at present */
	switch (format_) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_NV24:
	case DRM_FORMAT_NV42:
		isGPUConvert_ = isGPUConvert_ ? false : true;
		break;
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_MJPEG:
	default:
		isGPUConvert_ = false;
		break;
	}
}
