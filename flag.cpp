#include "precomp.h"

VerletFlag::VerletFlag( int2 location, Surface* pattern )
{
	width = pattern->width;
	height = pattern->height;
	polePos = make_float2( location );
	pos = new float2[width * height];
	prevPos = new float2[width * height];
	color = new uint[width * height];
	backup = new uint[width * height * 4];
	memcpy( color, pattern->pixels, width * height * 4 );
	for( int x = 0; x < width; x++ ) for( int y = 0; y < height; y++ )
		pos[x + y * width] = make_float2( location.x - x * 1.2f, y * 1.2f + location.y );
	memcpy( prevPos, pos, width * height * 8 );
}

void VerletFlag::Draw()
{
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++)
	{
		// plot each flag vertex bilinearly
		int index = x + y * width;
		int2 intPos = make_int2( pos[index] );
		backup[index * 4 + 0] = MyApp::map.bitmap->Read( intPos.x, intPos.y );
		backup[index * 4 + 1] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y );
		backup[index * 4 + 2] = MyApp::map.bitmap->Read( intPos.x, intPos.y + 1 );
		backup[index * 4 + 3] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y + 1 );
		hasBackup = true;
		MyApp::map.bitmap->PlotBilerp( pos[index].x, pos[index].y, color[index] );
	}
}

bool VerletFlag::Tick()
{
	// move vertices
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++)
	{
		int index = x + y * width;
		float2 delta = pos[index] - prevPos[index];
		prevPos[index] = pos[index];
		pos[index] += delta;
	}
	// apply forces
	float windForce = 0.1f + 0.05f * RandomFloat();
	float2 wind = windForce * normalize( make_float2( -1, (RandomFloat() * 0.5f) - 0.25f ) );
	for (int x = 1; x < width; x++) for (int y = 0; y < height; y++)
	{
		int index = x + y * width;
		pos[index] += wind;
		if ((RandomUInt() & 31) == 31) 
		{
			// small chance of a random nudge to add a bit of noise to the animation
			float2 nudge = make_float2( RandomFloat() - 0.5f, RandomFloat() - 0.5f );
			pos[index] += nudge;
		}
	}
	// constraints: limit distance
	for( int i = 0; i < 25; i++)
	{
		for( int x = 1; x < width; x++ ) for( int y = 0; y < height; y++ )
		{
			int index = x + y * width;
			float2 right = pos[index - 1] - pos[index];
			if (sqrLength( right ) > sqrf( 1.15f ))
			{
				float2 excess = right - normalize( right ) * 1.15f;
				pos[index] += excess * 0.5f;
				pos[index - 1] -= excess * 0.5f;
			}
		}
		for (int y = 0; y < height; y++) pos[y * width] = polePos + make_float2( 0, y * 1.2f );
	}
	// all done
	return true; // flags don't die
}

void VerletFlag::Remove()
{
	if (hasBackup) for (int x = width - 1; x >= 0; x--) for (int y = height - 1; y >= 0; y--)
	{
		int index = x + y * width;
		int2 intPos = make_int2( pos[index] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y, backup[index * 4 + 0] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y, backup[index * 4 + 1] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y + 1, backup[index * 4 + 2] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y + 1, backup[index * 4 + 3] );
	}
}