/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * viewfinderGL.cpp - Render YUV format frame by OpenGL shader
 */

#include "viewfinder_gl.h"

#include <QImage>

#include <libcamera/formats.h>

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

static const QList<libcamera::PixelFormat> supportFormats{
	libcamera::formats::NV12,
	libcamera::formats::NV21,
	libcamera::formats::NV16,
	libcamera::formats::NV61,
	libcamera::formats::NV24,
	libcamera::formats::NV42,
	libcamera::formats::YUV420,
	libcamera::formats::YVU420
};

ViewFinderGL::ViewFinderGL(QWidget *parent)
	: QOpenGLWidget(parent),
	  buffer_(nullptr),
	  pFShader_(nullptr),
	  pVShader_(nullptr),
	  vbuf_(QOpenGLBuffer::VertexBuffer),
	  yuvDataPtr_(nullptr),
	  textureU_(QOpenGLTexture::Target2D),
	  textureV_(QOpenGLTexture::Target2D),
	  textureY_(QOpenGLTexture::Target2D)
{
}

ViewFinderGL::~ViewFinderGL()
{
	removeShader();

	if (textureY_.isCreated())
		textureY_.destroy();

	if (textureU_.isCreated())
		textureU_.destroy();

	if (textureV_.isCreated())
		textureV_.destroy();

	vbuf_.destroy();
}

const QList<libcamera::PixelFormat> &ViewFinderGL::nativeFormats() const
{
	return (::supportFormats);
}

int ViewFinderGL::setFormat(const libcamera::PixelFormat &format,
			    const QSize &size)
{
	int ret = 0;

	if (isFormatSupport(format)) {
		format_ = format;
		size_ = size;
	} else {
		ret = -1;
	}
	updateGeometry();
	return ret;
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

	return (grabFramebuffer());
}

void ViewFinderGL::render(libcamera::FrameBuffer *buffer, MappedBuffer *map)
{
	if (buffer->planes().size() != 1) {
		qWarning() << "Multi-planar buffers are not supported";
		return;
	}

	if (buffer_)
		renderComplete(buffer_);

	unsigned char *memory = static_cast<unsigned char *>(map->memory);
	if (memory) {
		yuvDataPtr_ = memory;
		update();
		buffer_ = buffer;
	}
}

bool ViewFinderGL::isFormatSupport(const libcamera::PixelFormat &format)
{
	bool ret = true;
	switch (format) {
	case libcamera::formats::NV12:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_UV_f.glsl";
		break;
	case libcamera::formats::NV21:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_VU_f.glsl";
		break;
	case libcamera::formats::NV16:
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_UV_f.glsl";
		break;
	case libcamera::formats::NV61:
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_VU_f.glsl";
		break;
	case libcamera::formats::NV24:
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_UV_f.glsl";
		break;
	case libcamera::formats::NV42:
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_2_planes_VU_f.glsl";
		break;
	case libcamera::formats::YUV420:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_3_planes_UV_f.glsl";
		break;
	case libcamera::formats::YVU420:
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		vsrc_ = ":NV_vertex_shader.glsl";
		fsrc_ = ":NV_3_planes_VU_f.glsl";
		break;
	default:
		ret = false;
		qWarning() << "[ViewFinderGL]:"
			   << "format not support yet.";
		break;
	};

	return ret;
}

void ViewFinderGL::createVertexShader()
{
	bool bCompile;
	/* Create Vertex Shader */
	pVShader_ = new QOpenGLShader(QOpenGLShader::Vertex, this);

	bCompile = pVShader_->compileSourceFile(vsrc_);
	if (!bCompile) {
		qWarning() << "[ViewFinderGL]:" << pVShader_->log();
	}

	shaderProgram_.addShader(pVShader_);
}

bool ViewFinderGL::createFragmentShader()
{
	bool bCompile;

	/* Create Fragment Shader */
	pFShader_ = new QOpenGLShader(QOpenGLShader::Fragment, this);

	bCompile = pFShader_->compileSourceFile(fsrc_);
	if (!bCompile) {
		qWarning() << "[ViewFinderGL]:" << pFShader_->log();
		return bCompile;
	}

	shaderProgram_.addShader(pFShader_);

	/* Link shader pipeline */
	if (!shaderProgram_.link()) {
		qWarning() << "[ViewFinderGL]:" << shaderProgram_.log();
		close();
	}

	/* Bind shader pipeline for use */
	if (!shaderProgram_.bind()) {
		qWarning() << "[ViewFinderGL]:" << shaderProgram_.log();
		close();
	}

	shaderProgram_.enableAttributeArray(ATTRIB_VERTEX);
	shaderProgram_.enableAttributeArray(ATTRIB_TEXTURE);

	shaderProgram_.setAttributeBuffer(ATTRIB_VERTEX,
					  GL_FLOAT,
					  0,
					  2,
					  2 * sizeof(GLfloat));
	shaderProgram_.setAttributeBuffer(ATTRIB_TEXTURE,
					  GL_FLOAT,
					  8 * sizeof(GLfloat),
					  2,
					  2 * sizeof(GLfloat));

	textureUniformY_ = shaderProgram_.uniformLocation("tex_y");
	textureUniformU_ = shaderProgram_.uniformLocation("tex_u");
	textureUniformV_ = shaderProgram_.uniformLocation("tex_v");

	if (!textureY_.isCreated())
		textureY_.create();

	if (!textureU_.isCreated())
		textureU_.create();

	if (!textureV_.isCreated())
		textureV_.create();

	id_y_ = textureY_.textureId();
	id_u_ = textureU_.textureId();
	id_v_ = textureV_.textureId();
	return true;
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
	if (shaderProgram_.isLinked()) {
		shaderProgram_.release();
		shaderProgram_.removeAllShaders();
	}

	if (pFShader_)
		delete pFShader_;

	if (pVShader_)
		delete pVShader_;
}

void ViewFinderGL::initializeGL()
{
	initializeOpenGLFunctions();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	static const GLfloat vertices[]{
		-1.0f, -1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f, -1.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 1.0f
	};

	vbuf_.create();
	vbuf_.bind();
	vbuf_.allocate(vertices, sizeof(vertices));

	/* Create Vertex Shader */
	createVertexShader();

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

void ViewFinderGL::doRender()
{
	switch (format_) {
	case libcamera::formats::NV12:
	case libcamera::formats::NV21:
	case libcamera::formats::NV16:
	case libcamera::formats::NV61:
	case libcamera::formats::NV24:
	case libcamera::formats::NV42:
		/* activate texture 0 */
		glActiveTexture(GL_TEXTURE0);
		configureTexture(id_y_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width(),
			     size_.height(),
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     yuvDataPtr_);
		glUniform1i(textureUniformY_, 0);

		/* activate texture 1 */
		glActiveTexture(GL_TEXTURE1);
		configureTexture(id_u_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RG,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0,
			     GL_RG,
			     GL_UNSIGNED_BYTE,
			     (char *)yuvDataPtr_ + size_.width() * size_.height());
		glUniform1i(textureUniformU_, 1);
		break;
	case libcamera::formats::YUV420:
		/* activate texture 0 */
		glActiveTexture(GL_TEXTURE0);
		configureTexture(id_y_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width(),
			     size_.height(),
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     yuvDataPtr_);
		glUniform1i(textureUniformY_, 0);

		/* activate texture 1 */
		glActiveTexture(GL_TEXTURE1);
		configureTexture(id_u_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     (char *)yuvDataPtr_ + size_.width() * size_.height());
		glUniform1i(textureUniformU_, 1);

		/* activate texture 2 */
		glActiveTexture(GL_TEXTURE2);
		configureTexture(id_v_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     (char *)yuvDataPtr_ + size_.width() * size_.height() * 5 / 4);
		glUniform1i(textureUniformV_, 2);
		break;
	case libcamera::formats::YVU420:
		/* activate texture 0 */
		glActiveTexture(GL_TEXTURE0);
		configureTexture(id_y_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width(),
			     size_.height(),
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     yuvDataPtr_);
		glUniform1i(textureUniformY_, 0);

		/* activate texture 1 */
		glActiveTexture(GL_TEXTURE2);
		configureTexture(id_v_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     (char *)yuvDataPtr_ + size_.width() * size_.height());
		glUniform1i(textureUniformV_, 1);

		/* activate texture 2 */
		glActiveTexture(GL_TEXTURE1);
		configureTexture(id_u_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RED,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0,
			     GL_RED,
			     GL_UNSIGNED_BYTE,
			     (char *)yuvDataPtr_ + size_.width() * size_.height() * 5 / 4);
		glUniform1i(textureUniformU_, 2);
	default:
		break;
	};
}

void ViewFinderGL::paintGL()
{
	if (pFShader_ == nullptr)
		createFragmentShader();

	if (yuvDataPtr_) {
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		doRender();
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void ViewFinderGL::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

QSize ViewFinderGL::sizeHint() const
{
	return size_.isValid() ? size_ : QSize(640, 480);
}
