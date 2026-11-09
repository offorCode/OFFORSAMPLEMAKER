#pragma once
#include "AudioEffect.h"

std::unique_ptr<AudioEffect> makeBuiltInEffect(const EffectSettings&);
