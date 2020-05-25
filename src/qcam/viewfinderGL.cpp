/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * viewfinderGL.cpp - Render YUV format frame by OpenGL shader
 */
#include <QImage>

#include "fshader.h"
#include "viewfinderGL.h"


#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

ViewFinderGL::ViewFinderGL(QWidget *parent)
	: QOpenGLWidget(parent),
	glBuffer(QOpenGLBuffer::VertexBuffer),
	pFShader(nullptr),
	pVShader(nullptr),
	textureU(QOpenGLTexture::Target2D),
	textureV(QOpenGLTexture::Target2D),
	textureY(QOpenGLTexture::Target2D),
	yuvDataPtr(nullptr)

{
}

ViewFinderGL::~ViewFinderGL()
{
	removeShader();

	if(textureY.isCreated())
		textureY.destroy();

	if(textureU.isCreated())
		textureU.destroy();

	if(textureV.isCreated())
		textureV.destroy();

	glBuffer.destroy();
}

int ViewFinderGL::setFormat(const libcamera::PixelFormat &format,
			    const QSize &size)
{
	format_ = format;
	size_ = size;

	updateGeometry();
	return 0;
}

void ViewFinderGL::stop()
{
	if (buffer_) {
		renderComplete(buffer_);
		buffer_ = nullptr;
	}
}

QImage ViewFinderGL::getCurrentImage()
{
	QMutexLocker locker(&mutex_);

	return(grabFramebuffer());
}

void ViewFinderGL::render(libcamera::FrameBuffer *buffer, MappedBuffer *map)
{
	if (buffer->planes().size() != 1) {
		qWarning() << "Multi-planar buffers are not supported";
		return;
	}

	unsigned char *memory = static_cast<unsigned char *>(map->memory);
	if(memory) {
		yuvDataPtr = memory;
		update();
		buffer_ = buffer;
	}

	if (buffer)
		renderComplete(buffer);
}

void ViewFinderGL::updateFrame(unsigned char *buffer)
{
	if(buffer) {
		yuvDataPtr = buffer;
		update();
	}
}

void ViewFinderGL::setFrameSize(int width, int height)
{
	if(width > 0 && height > 0) {
		width_ = width;
		height_ = height;
	}
}

char *ViewFinderGL::selectFragmentShader(unsigned int format)
{
	char *fsrc = nullptr;

	switch(format) {
	case DRM_FORMAT_NV12:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		fsrc = NV_2_planes_UV;
		break;
	case DRM_FORMAT_NV21:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		fsrc = NV_2_planes_VU;
		break;
	case DRM_FORMAT_NV16:
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		fsrc = NV_2_planes_UV;
		break;
	case DRM_FORMAT_NV61:
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		fsrc = NV_2_planes_VU;
		break;
	case DRM_FORMAT_NV24:
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		fsrc = NV_2_planes_UV;
		break;
	case DRM_FORMAT_NV42:
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		fsrc = NV_2_planes_VU;
		break;
	case DRM_FORMAT_YUV420:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		fsrc = NV_3_planes_UV;
		break;
	case DRM_FORMAT_YVU420:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		fsrc =  NV_3_planes_VU;
		break;
	default:
		break;
	};

	if(fsrc == nullptr) {
		qDebug() << __func__ << "[ERROR]:" <<" No suitable fragment shader can be select.";
	}
	return fsrc;
}

void ViewFinderGL::createFragmentShader()
{
	bool bCompile;

	pFShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
	char *fsrc = selectFragmentShader(format_);

	bCompile = pFShader->compileSourceCode(fsrc);
	if(!bCompile)
	{
		qDebug() << __func__ << ":" << pFShader->log();
	}

	shaderProgram.addShader(pFShader);

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

	shaderProgram.enableAttributeArray(ATTRIB_VERTEX);
	shaderProgram.enableAttributeArray(ATTRIB_TEXTURE);

	shaderProgram.setAttributeBuffer(ATTRIB_VERTEX,GL_FLOAT,0,2,2*sizeof(GLfloat));
	shaderProgram.setAttributeBuffer(ATTRIB_TEXTURE,GL_FLOAT,8*sizeof(GLfloat),2,2*sizeof(GLfloat));

	textureUniformY = shaderProgram.uniformLocation("tex_y");
	textureUniformU = shaderProgram.uniformLocation("tex_u");
	textureUniformV = shaderProgram.uniformLocation("tex_v");

	if(!textureY.isCreated())
		textureY.create();

	if(!textureU.isCreated())
		textureU.create();

	if(!textureV.isCreated())
		textureV.create();

	id_y = textureY.textureId();
	id_u = textureU.textureId();
	id_v = textureV.textureId();
}

void ViewFinderGL::configureTexture(unsigned int id)
{
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void ViewFinderGL::removeShader()
{
	if (shaderProgram.isLinked()) {
		shaderProgram.release();
		shaderProgram.removeAllShaders();
	}

	if(pFShader)
		delete pFShader;

	if(pVShader)
		delete pVShader;
}

void ViewFinderGL::initializeGL()
{
	bool bCompile;

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

	/* Create Vertex Shader */
	pVShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
	const char *vsrc =  "attribute vec4 vertexIn;\n"
                        "attribute vec2 textureIn;\n"
                        "varying vec2 textureOut;\n"
                        "void main(void)\n"
                        "{\n"
                        "gl_Position = vertexIn;\n"
                        "textureOut = textureIn;\n"
                        "}\n";

	bCompile = pVShader->compileSourceCode(vsrc);
	if(!bCompile) {
		qDebug() << __func__<< ":" << pVShader->log();
	}

	shaderProgram.addShader(pVShader);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

void ViewFinderGL::render()
{
	switch(format_) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_NV24:
	case DRM_FORMAT_NV42:
		/* activate texture 0 */
		glActiveTexture(GL_TEXTURE0);
		configureTexture(id_y);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size_.width(), size_.height(), 0, GL_RED, GL_UNSIGNED_BYTE, yuvDataPtr);
		glUniform1i(textureUniformY, 0);

		/* activate texture 1 */
		glActiveTexture(GL_TEXTURE1);
		configureTexture(id_u);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, size_.width()/horzSubSample_, size_.height()/vertSubSample_, 0, GL_RG, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+size_.width()*size_.height());
		glUniform1i(textureUniformU, 1);
		break;
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		/* activate texture 0 */
		glActiveTexture(GL_TEXTURE0);
		configureTexture(id_y);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size_.width(), size_.height(), 0, GL_RED, GL_UNSIGNED_BYTE, yuvDataPtr);
		glUniform1i(textureUniformY, 0);

		/* activate texture 1 */
		glActiveTexture(GL_TEXTURE1);
		configureTexture(id_u);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, size_.width()/horzSubSample_, size_.height()/vertSubSample_, 0, GL_RG, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+size_.width()*size_.height());
		glUniform1i(textureUniformU, 1);

		/* activate texture 2 */
		glActiveTexture(GL_TEXTURE2);
		configureTexture(id_v);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, size_.width()/horzSubSample_, size_.height()/vertSubSample_, 0, GL_RG, GL_UNSIGNED_BYTE, (char*)yuvDataPtr+size_.width()*size_.height()*5/4);
		glUniform1i(textureUniformV, 2);
		break;
	default:
		break;
	};
}

void ViewFinderGL::paintGL()
{
	if(pFShader == nullptr)
		createFragmentShader();

	if(yuvDataPtr)
	{
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		render();
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void ViewFinderGL::resizeGL(int w, int h)
{
	glViewport(0,0,w,h);
}

QSize ViewFinderGL::sizeHint() const
{
	return size_.isValid() ? size_ : QSize(640, 480);
}
