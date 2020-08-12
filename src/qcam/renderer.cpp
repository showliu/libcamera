/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * renderer.cpp - Render YUV format frame by OpenGL shader
 */

#include "renderer.h"

Renderer::Renderer()
	: fbo_(nullptr),
	  vbo_(QOpenGLBuffer::VertexBuffer),
	  pFShader_(nullptr),
	  pVShader_(nullptr),
	  textureU_(QOpenGLTexture::Target2D),
	  textureV_(QOpenGLTexture::Target2D),
	  textureY_(QOpenGLTexture::Target2D)
{
	/* offscreen format setup */
	setFormat(requestedFormat());
	create();

	/* create OpenGL context */
	if (ctx_.create()) {
		ctx_.makeCurrent(this);
		initializeOpenGLFunctions();
	} else {
		qWarning() << "[Renderer]: "
			   << "OpenGL renderer is not available.";
	}
}

Renderer::~Renderer()
{
	if (vbo_.isCreated()) {
		vbo_.release();
		vbo_.destroy();
	}

	if (fbo_) {
		fbo_->release();
		delete fbo_;
	}

	removeShader();

	if (textureY_.isCreated())
		textureY_.destroy();

	if (textureU_.isCreated())
		textureU_.destroy();

	if (textureV_.isCreated())
		textureV_.destroy();

	ctx_.doneCurrent();
}

void Renderer::initializeGL()
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	static const GLfloat vertices[]{
		-1.0f, -1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f, -1.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 1.0f
	};

	vbo_.create();
	vbo_.bind();
	vbo_.allocate(vertices, sizeof(vertices));

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

bool Renderer::selectShader(const libcamera::PixelFormat &format)
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
	default:
		ret = false;
		qWarning() << "[Renderer]: "
			   << "format not support yet.";
		break;
	};

	return ret;
}

void Renderer::removeShader()
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

bool Renderer::createShader()
{
	bool bCompile;

	/* Create Vertex Shader */
	pVShader_ = new QOpenGLShader(QOpenGLShader::Vertex, this);

	bCompile = pVShader_->compileSourceFile(vsrc_);
	if (!bCompile) {
		qWarning() << "[Renderer]: " << pVShader_->log();
		return bCompile;
	}

	shaderProgram_.addShader(pVShader_);

	/* Create Fragment Shader */
	pFShader_ = new QOpenGLShader(QOpenGLShader::Fragment, this);

	bCompile = pFShader_->compileSourceFile(fsrc_);
	if (!bCompile) {
		qWarning() << "[Renderer]: " << pFShader_->log();
		return bCompile;
	}

	shaderProgram_.addShader(pFShader_);

	// Link shader pipeline
	if (!shaderProgram_.link()) {
		qWarning() << "[Renderer]: " << shaderProgram_.log();
		return false;
	}

	// Bind shader pipeline for use
	if (!shaderProgram_.bind()) {
		qWarning() << "[Renderer]: " << shaderProgram_.log();
		return false;
	}
	return true;
}

bool Renderer::configure(const libcamera::PixelFormat &format, const QSize &size)
{
	bool ret = true;

	if (selectShader(format)) {
		ret = createShader();
		if (!ret)
			return ret;

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

		fbo_ = new QOpenGLFramebufferObject(size.width(),
						    size.height(),
						    GL_TEXTURE_2D);
		fbo_->bind();
		glViewport(0, 0, size.width(), size.height());

		format_ = format;
		size_ = size;
	} else {
		ret = false;
	}
	return ret;
}

void Renderer::configureTexture(unsigned int id)
{
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Renderer::render(unsigned char *buffer)
{
	QMutexLocker locker(&mutex_);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
			     buffer);
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
			     buffer + size_.width() * size_.height());
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
			     buffer);
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
			     buffer + size_.width() * size_.height());
		glUniform1i(textureUniformU_, 1);

		/* activate texture 2 */
		glActiveTexture(GL_TEXTURE2);
		configureTexture(id_v_);
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RG,
			     size_.width() / horzSubSample_,
			     size_.height() / vertSubSample_,
			     0, GL_RG,
			     GL_UNSIGNED_BYTE,
			     buffer + size_.width() * size_.height() * 5 / 4);
		glUniform1i(textureUniformV_, 2);
		break;
	default:
		break;
	};

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

QImage Renderer::toImage()
{
	QMutexLocker locker(&mutex_);
	return (fbo_->toImage(true));
}
