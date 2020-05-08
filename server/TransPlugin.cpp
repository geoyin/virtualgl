// Copyright (C)2009-2011, 2014, 2019-2020 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include "TransPlugin.h"
#include "fakerconfig.h"
#include <dlfcn.h>
#include <string.h>
#include "Error.h"
#include "faker.h"

using namespace vglutil;
using namespace vglserver;


#undef THROW
#define THROW(m)  throw(Error("transport plugin", m, -1))
#define DISABLE_FAKER() \
{ \
	vglfaker::setFakerLevel(vglfaker::getFakerLevel() + 1); \
	vglfaker::setExcludeCurrent(true); \
}
#define ENABLE_FAKER() \
{ \
	vglfaker::setFakerLevel(vglfaker::getFakerLevel() - 1); \
	vglfaker::setExcludeCurrent(false); \
}


static void *loadsym(void *dllhnd, const char *symbol)
{
	void *sym = NULL;  const char *err = NULL;
	sym = dlsym(dllhnd, (char *)symbol);
	if(!sym)
	{
		err = dlerror();
		if(err) THROW(err);
		else THROW("Could not load symbol");
	}
	return sym;
}


TransPlugin::TransPlugin(Display *dpy, Window win, char *name)
{
	if(!name || strlen(name) < 1) THROW("Transport name is empty or NULL!");
	const char *err = NULL;
	CriticalSection::SafeLock l(mutex);
	dlerror();  // Clear error state
	char filename[MAXSTR];
	snprintf(filename, MAXSTR - 1, "libvgltrans_%s.so", name);
	dllhnd = dlopen(filename, RTLD_NOW);
	if(!dllhnd)
	{
		err = dlerror();
		if(err) THROW(err);
		else THROW("Could not open transport plugin");
	}
	_RRTransInit = (_RRTransInitType)loadsym(dllhnd, "RRTransInit");
	_RRTransConnect = (_RRTransConnectType)loadsym(dllhnd, "RRTransConnect");
	_RRTransGetFrame = (_RRTransGetFrameType)loadsym(dllhnd, "RRTransGetFrame");
	_RRTransReady = (_RRTransReadyType)loadsym(dllhnd, "RRTransReady");
	_RRTransSynchronize =
		(_RRTransSynchronizeType)loadsym(dllhnd, "RRTransSynchronize");
	_RRTransSendFrame =
		(_RRTransSendFrameType)loadsym(dllhnd, "RRTransSendFrame");
	_RRTransDestroy = (_RRTransDestroyType)loadsym(dllhnd, "RRTransDestroy");
	_RRTransGetError = (_RRTransGetErrorType)loadsym(dllhnd, "RRTransGetError");

	DISABLE_FAKER();
	handle = _RRTransInit(dpy, win, &fconfig);
	if(!handle) THROW(_RRTransGetError());
	ENABLE_FAKER();
}


TransPlugin::~TransPlugin(void)
{
	CriticalSection::SafeLock l(mutex);
	destroy();
	if(dllhnd) dlclose(dllhnd);
}


void TransPlugin::connect(char *receiverName, int port)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	int ret = _RRTransConnect(handle, receiverName, port);
	ENABLE_FAKER();
	if(ret < 0) THROW(_RRTransGetError());
}


void TransPlugin::destroy(void)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	int ret = _RRTransDestroy(handle);
	ENABLE_FAKER();
	if(ret < 0) THROW(_RRTransGetError());
}


int TransPlugin::ready(void)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	int ret = _RRTransReady(handle);
	ENABLE_FAKER();
	if(ret < 0) THROW(_RRTransGetError());
	return ret;
}


void TransPlugin::synchronize(void)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	int ret = _RRTransSynchronize(handle);
	ENABLE_FAKER();
	if(ret < 0) THROW(_RRTransGetError());
}


RRFrame *TransPlugin::getFrame(int width, int height, int format, bool stereo)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	RRFrame *ret = _RRTransGetFrame(handle, width, height, format, stereo);
	ENABLE_FAKER();
	if(!ret) THROW(_RRTransGetError());
	return ret;
}


void TransPlugin::sendFrame(RRFrame *frame, bool sync)
{
	CriticalSection::SafeLock l(mutex);
	DISABLE_FAKER();
	int ret = _RRTransSendFrame(handle, frame, sync);
	ENABLE_FAKER();
	if(ret < 0) THROW(_RRTransGetError());
}
