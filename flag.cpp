#include "precomp.h"

VerletFlag::VerletFlag(int2 location, Surface* pattern)
{
	width = pattern->width; //128
	height = pattern->height; //48
	dims = height << 7;
	polePos = make_float2(location);
	pos = new float2[dims];
	prevPos = new float2[dims];
	color = new uint[dims];
	backup = new uint[dims << 2];
	memcpy(color, pattern->pixels, (dims << 2));
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++)
		pos[x + (y << 7)] = make_float2(polePos.x - x * 1.2f, y * 1.2f + polePos.y);
	memcpy(prevPos, pos, (dims << 3));
}

void VerletFlag::Draw()
{
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++)
	{
		// plot each flag vertex bilinearly
		int index = x + (y << 7);
		int2 intPos = make_int2(pos[index]);
		backup[(index << 2)] = MyApp::map.bitmap->Read(intPos.x, intPos.y);
		backup[(index << 2) + 1] = MyApp::map.bitmap->Read(intPos.x + 1, intPos.y);
		backup[(index << 2) + 2] = MyApp::map.bitmap->Read(intPos.x, intPos.y + 1);
		backup[(index << 2) + 3] = MyApp::map.bitmap->Read(intPos.x + 1, intPos.y + 1);
		hasBackup = true;
		MyApp::map.bitmap->PlotBilerp(pos[index].x, pos[index].y, color[index]);
	}
}

bool VerletFlag::Tick()
{
	// apply forces
	float windForce = 0.1f + 0.05f * RandomFloat();
	float2 wind = windForce * normalize(make_float2(-1, (RandomFloat() * 0.5f) - 0.25f));
	int x, y;

	for (y = 0; y < dims; y += width)
	{
		float2 delta = pos[y] - prevPos[y];
		prevPos[y] = pos[y];
		pos[y] += delta;
	}

	for (x = 1; x < width; x++) for (y = 0; y < height; y++)
	{
		// move vertices
		int index = x + (y << 7);
		float2 delta = pos[index] - prevPos[index];
		prevPos[index] = pos[index];
		if ((RandomUInt() & 31) == 31)
		{
			// small chance of a random nudge to add a bit of noise to the animation
			float2 nudge = make_float2(RandomFloat() - 0.5f, RandomFloat() - 0.5f);
			delta += nudge;
		}
		pos[index] += delta + wind;
	}
	// constraints: limit distance
	const float maxDistSqrf = 1.3225f;
	for (int i = 0; i < 25; i++)
	{
		for (x = 1; x < width; x++) for (y = 0; y < height; y++)
		{
			int index = x + (y << 7);
			float2 right = pos[index - 1] - pos[index];
			if (sqrLength(right) > maxDistSqrf)
			{
				float2 excess = right - normalize(right) * 1.15f;
				pos[index] += excess * 0.5f;
				pos[index - 1] -= excess * 0.5f;
			}
		}
		for (y = 0; y < height; y++)
		{
			pos[y << 7] = polePos + make_float2(0, y * 1.2f);
		}
	}
	// all done
	return true; // flags don't die
}

void VerletFlag::Remove()
{
	if (hasBackup) for (int x = width - 1; x >= 0; x--) for (int y = height - 1; y >= 0; y--)
	{
		int index = x + (y << 7);
		int2 intPos = make_int2(pos[index]);
		MyApp::map.bitmap->Plot(intPos.x, intPos.y, backup[(index << 2)]);
		MyApp::map.bitmap->Plot(intPos.x + 1, intPos.y, backup[(index << 2) + 1]);
		MyApp::map.bitmap->Plot(intPos.x, intPos.y + 1, backup[(index << 2) + 2]);
		MyApp::map.bitmap->Plot(intPos.x + 1, intPos.y + 1, backup[(index << 2) + 3]);
	}
}