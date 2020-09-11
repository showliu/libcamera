/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * viewfinder.h - qcam - Viewfinder base class
 */
#ifndef __QCAM_VIEWFINDER_H__
#define __QCAM_VIEWFINDER_H__

#include <QImage>
#include <QList>
#include <QSize>

#include <libcamera/buffer.h>
#include <libcamera/formats.h>

struct MappedBuffer {
	void *memory;
	size_t size;
};

class ViewFinder
{
public:
	virtual ~ViewFinder() {}

	virtual const QList<libcamera::PixelFormat> &nativeFormats() const = 0;

	virtual int setFormat(const libcamera::PixelFormat &format, const QSize &size) = 0;
	virtual void render(libcamera::FrameBuffer *buffer, MappedBuffer *map) = 0;
	virtual void stop() = 0;

	virtual QImage getCurrentImage() = 0;
};

#endif /* __QCAM_VIEWFINDER_H__ */
