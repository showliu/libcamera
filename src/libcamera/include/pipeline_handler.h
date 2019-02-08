/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * pipeline_handler.h - Pipeline handler infrastructure
 */
#ifndef __LIBCAMERA_PIPELINE_HANDLER_H__
#define __LIBCAMERA_PIPELINE_HANDLER_H__

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace libcamera {

class BufferPool;
class Camera;
class CameraManager;
class DeviceEnumerator;
class MediaDevice;
class Request;
class Stream;
class StreamConfiguration;

class CameraData
{
public:
	virtual ~CameraData() {}

protected:
	CameraData() {}

private:
	CameraData(const CameraData &) = delete;
	CameraData &operator=(const CameraData &) = delete;
};

class PipelineHandler : public std::enable_shared_from_this<PipelineHandler>
{
public:
	PipelineHandler(CameraManager *manager);
	virtual ~PipelineHandler();

	virtual bool match(DeviceEnumerator *enumerator) = 0;

	virtual std::map<Stream *, StreamConfiguration>
	streamConfiguration(Camera *camera, std::vector<Stream *> &streams) = 0;
	virtual int configureStreams(Camera *camera,
				     std::map<Stream *, StreamConfiguration> &config) = 0;

	virtual int allocateBuffers(Camera *camera, Stream *stream) = 0;
	virtual int freeBuffers(Camera *camera, Stream *stream) = 0;

	virtual int start(const Camera *camera) = 0;
	virtual void stop(const Camera *camera) = 0;

	virtual int queueRequest(const Camera *camera, Request *request) = 0;

protected:
	void registerCamera(std::shared_ptr<Camera> camera);
	void hotplugMediaDevice(MediaDevice *media);

	CameraData *cameraData(const Camera *camera);
	void setCameraData(const Camera *camera, std::unique_ptr<CameraData> data);

	CameraManager *manager_;

private:
	void mediaDeviceDisconnected(MediaDevice *media);
	virtual void disconnect();

	std::vector<std::weak_ptr<Camera>> cameras_;
	std::map<const Camera *, std::unique_ptr<CameraData>> cameraData_;
};

class PipelineHandlerFactory
{
public:
	PipelineHandlerFactory(const char *name);
	virtual ~PipelineHandlerFactory() { };

	virtual std::shared_ptr<PipelineHandler> create(CameraManager *manager) = 0;

	const std::string &name() const { return name_; }

	static void registerType(PipelineHandlerFactory *factory);
	static std::vector<PipelineHandlerFactory *> &factories();

private:
	std::string name_;
};

#define REGISTER_PIPELINE_HANDLER(handler)				\
class handler##Factory final : public PipelineHandlerFactory		\
{									\
public:									\
	handler##Factory() : PipelineHandlerFactory(#handler) {}	\
	std::shared_ptr<PipelineHandler> create(CameraManager *manager)	\
	{								\
		return std::make_shared<handler>(manager);		\
	}								\
};									\
static handler##Factory global_##handler##Factory;

} /* namespace libcamera */

#endif /* __LIBCAMERA_PIPELINE_HANDLER_H__ */
