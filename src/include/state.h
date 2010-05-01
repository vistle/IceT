/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

#ifndef _ICET_STATE_H_
#define _ICET_STATE_H_

#include <IceT.h>

typedef IceTUnsignedInt64 IceTTimeStamp;

struct IceTStateValue {
    IceTEnum type;
    IceTInt size;
    void *data;
    IceTTimeStamp mod_time;
};

typedef struct IceTStateValue *IceTState;

IceTState icetStateCreate(void);
void      icetStateDestroy(IceTState state);
void      icetStateCopy(IceTState dest, const IceTState src);
void      icetStateSetDefaults(void);

ICET_EXPORT void icetStateSetDoublev(IceTEnum pname, IceTInt size,
                                     const IceTDouble *data);
ICET_EXPORT void icetStateSetFloatv(IceTEnum pname, IceTInt size,
                                    const IceTFloat *data);
ICET_EXPORT void icetStateSetIntegerv(IceTEnum pname, IceTInt size,
                                      const IceTInt *data);
ICET_EXPORT void icetStateSetBooleanv(IceTEnum pname, IceTInt size,
                                      const IceTBoolean *data);
ICET_EXPORT void icetStateSetPointerv(IceTEnum pname, IceTInt size,
                                      const IceTVoid **data);

ICET_EXPORT void icetStateSetDouble(IceTEnum pname, IceTDouble value);
ICET_EXPORT void icetStateSetFloat(IceTEnum pname, IceTFloat value);
ICET_EXPORT void icetStateSetInteger(IceTEnum pname, IceTInt value);
ICET_EXPORT void icetStateSetBoolean(IceTEnum pname, IceTBoolean value);
ICET_EXPORT void icetStateSetPointer(IceTEnum pname, const IceTVoid *value);

ICET_EXPORT IceTEnum icetStateGetType(IceTEnum pname);
ICET_EXPORT IceTInt icetStateGetSize(IceTEnum pname);
ICET_EXPORT IceTTimeStamp icetStateGetTime(IceTEnum pname);

ICET_EXPORT void icetUnsafeStateSet(IceTEnum pname, IceTInt size, IceTEnum type,
                                    IceTVoid *data);
ICET_EXPORT void *icetUnsafeStateGet(IceTEnum pname);
ICET_EXPORT IceTEnum icetStateType(IceTEnum pname);

ICET_EXPORT IceTTimeStamp icetGetTimeStamp(void);

void icetStateResetTiming(void);

void icetStateDump(void);

#endif /* _ICET_STATE_H_ */
