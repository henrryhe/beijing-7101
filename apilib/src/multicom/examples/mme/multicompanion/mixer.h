/*
 * mixer.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Header file for the example 8-bit mixer.
 */

#ifndef MIXER_H
#define MIXER_H

enum MixerParamsInit {
	MME_OFFSET_MixerParamsInit_volume,
	MME_LENGTH_MixerParamsInit = MME_OFFSET_MixerParamsInit_volume + 2

#define MME_TYPE_MixerParamsInit_volume float
};

typedef MME_GENERIC64 MixerParamsInit_t[MME_LENGTH(MixerParamsInit)];

MME_ERROR Mixer_RegisterTransformer(const char *);

#endif /* MIXER_H */
