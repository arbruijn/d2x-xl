#ifndef OBJBUFIO_H_
#define OBJBUFIO_H_
#include "object.h"

#define OBJECT_BUFFER_SIZE 0x108

// Only supports network sync'ed objects: robot, hostage, player, weapon, powerup, reactor, ghost, marker

void ObjectWriteToBuffer(CObject *pObj, void *pBuffer);
void ObjectReadFromBuffer(CObject *pObj, void *pBuffer, bool compat = false);

#endif
