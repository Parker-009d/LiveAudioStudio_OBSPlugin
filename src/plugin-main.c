/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <math.h>
#include <stdatomic.h>

#include <obs-module.h>
#include <plugin-support.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct gain_filter_data {
	obs_source_t *context;
	_Atomic float gain_linear;
};

static const char *gain_filter_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "Live Audio Studio Gain";
}

static float gain_db_to_linear(double gain_db)
{
	return (float)pow(10.0, gain_db / 20.0);
}

static void *gain_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct gain_filter_data *filter = bzalloc(sizeof(*filter));
	filter->context = source;

	double gain_db = obs_data_get_double(settings, "gain_db");
	atomic_init(&filter->gain_linear, gain_db_to_linear(gain_db));

	return filter;
}

static void gain_filter_destroy(void *data)
{
	bfree(data);
}

static void gain_filter_update(void *data, obs_data_t *settings)
{
	struct gain_filter_data *filter = data;
	double gain_db = obs_data_get_double(settings, "gain_db");

	atomic_store(&filter->gain_linear, gain_db_to_linear(gain_db));
}

static void gain_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "gain_db", 0.0);
}

static obs_properties_t *gain_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *properties = obs_properties_create();

	obs_properties_add_float_slider(properties, "gain_db", "Gain (dB)", -24.0, 24.0, 0.5);

	return properties;
}

static struct obs_audio_data *gain_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct gain_filter_data *filter = data;
	const float gain = atomic_load(&filter->gain_linear);

	if (!audio || gain == 1.0f)
		return audio;

	const size_t channel_count = sizeof(audio->data) / sizeof(audio->data[0]);

	for (size_t channel = 0; channel < channel_count; ++channel) {
		if (!audio->data[channel])
			continue;

		float *samples = (float *)audio->data[channel];

		for (size_t frame = 0; frame < audio->frames; ++frame)
			samples[frame] *= gain;
	}

	return audio;
}

static struct obs_source_info gain_filter = {
	.id = "live_audio_studio_gain",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = gain_filter_get_name,
	.create = gain_filter_create,
	.destroy = gain_filter_destroy,
	.update = gain_filter_update,
	.get_defaults = gain_filter_defaults,
	.get_properties = gain_filter_properties,
	.filter_audio = gain_filter_audio,
};

bool obs_module_load(void)
{
	obs_register_source(&gain_filter);
	obs_log(LOG_INFO, "Live Audio Studio Gain filter registered (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
