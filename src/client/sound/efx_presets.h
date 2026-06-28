#pragma once

#include "al_helpers.h"

namespace sound {

struct ReverbPreset {
    const char *name;
    float density;
    float diffusion;
    float gain;
    float gainhf;
    float decay_time;
    float decay_hfratio;
    float reflections_gain;
    float reflections_delay;
    float late_reverb_gain;
    float late_reverb_delay;
    float air_absorption_gainhf;
    float room_rolloff_factor;
    int decay_hflimit;
};

static const ReverbPreset reverb_presets[] = {
    {"none",  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"cave",  1.0f, 1.0f, 0.3f, 0.8f, 2.8f, 0.7f, 0.3f, 0.02f, 0.5f, 0.04f, 0.994f, 0.0f, 1},
    {"room",  1.0f, 1.0f, 0.4f, 0.5f, 0.4f, 0.8f, 0.4f, 0.01f, 0.6f, 0.01f, 0.994f, 0.0f, 1},
    {"forest", 1.0f, 0.7f, 0.3f, 0.4f, 1.5f, 0.5f, 0.2f, 0.05f, 0.3f, 0.08f, 0.994f, 0.0f, 1},
    {"mountains", 1.0f, 0.5f, 0.3f, 0.3f, 3.5f, 0.4f, 0.1f, 0.10f, 0.2f, 0.15f, 0.994f, 0.0f, 1},
};

}
