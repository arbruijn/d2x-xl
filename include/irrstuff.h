#ifndef _IRRSTUFF_H
#define _IRRSTUFF_H

#define USE_IRRLICHT	0

#if USE_IRRLICHT
#	include "irrlicht.h"

using namespace irr;
using namespace scene;
using namespace io;
using namespace gui;
using namespace video;
using namespace core;

//#pragma comment(lib, "..\..\irrlicht-1.4\lib\Irrlicht.lib")

typedef struct tIrrData {
	IrrlichtDevice		*deviceP;
	IVideoDriver		*videoP;
	ISceneManager		*sceneP;
	IGUIEnvironment	*guiP;
	} tIrrData;

extern tIrrData irrData;

#define IRRDEVICE	irrData.deviceP
#define IRRVIDEO	irrData.videoP
#define IRRSCENE	irrData.sceneP
#define IRRGUI		irrData.guiP

#endif //USE_IRRLICHT

#endif //_IRRSTUFF_H

