#pragma once

namespace MGDF { namespace core {

typedef struct
{
	double ActiveRenderTime;
	double RenderTime;
	double ExpectedSimTime;
	double ActiveSimTime;
	double SimTime;
	double SimInputTime;
	double SimAudioTime;
} SystemStats;

}}