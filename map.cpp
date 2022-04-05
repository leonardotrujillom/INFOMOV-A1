#include "precomp.h"

Map::Map()
{
	// load color map
	bitmap = new Surface( "assets/colours.png" );
	width = bitmap->width;
	height = bitmap->height;
	// load height map
	Surface heightMap( "assets/heightmap.png" );
	elevation = new int[width * height];
	for (int i = 0; i < width * height; i++) elevation[i] = heightMap.pixels[i] & 255;
	// set intial focus to centre of map
	focus = make_int2( width / 2, height / 2 );
	// all done; original maps will be deleted when leaving scope.
}

void Map::UpdateView( Surface* target, float scale )
{
	// determine what map square to draw: centered at location 'focus', clamped to the edges of the map
	const float aspectRatio = (float)target->height / (float)target->width;
	const int mapSliceSizeX = (int)((width * scale) / 100);
	const int mapSliceSizeY = (int)((float)mapSliceSizeX * aspectRatio);
	int mapX1 = max( 0, focus.x - mapSliceSizeX / 2 );
	int mapY1 = max( 0, focus.y - mapSliceSizeY / 2 );
	int mapX2 = mapX1 + mapSliceSizeX;
	int mapY2 = mapY1 + mapSliceSizeY;
	if (mapX2 >= (width - 1))
	{
		int shift = mapX2 - (width - 2);
		mapX1 = max( 0, mapX1 - shift );
		mapX2 -= shift;
	}
	if (mapY2 >= (height - 1))
	{
		int shift = mapY2 - (height - 2);
		mapY1 = max( 0, mapY1 - shift );
		mapY2 -= shift;
	}
	focus.x = (mapX1 + mapX2) / 2;
	focus.y = (mapY1 + mapY2) / 2;
	view.x = mapX1, view.y = mapY1;
	view.z = mapX2, view.w = mapY2;
}

void Map::Draw( Surface* target )
{
	// draw pixels
	int dx = ((view.z - view.x) * 16384) / target->width;
	int dy = ((view.w - view.y) * 16384) / target->height;
#pragma omp parallel for schedule(static)
	for (int y = 0; y < target->height; y++)
	{
		uint y_fp = (view.y << 14) + y * dy;
		uint* mapLine = bitmap->pixels + (y_fp >> 14) * width;
		uint* dst = target->pixels + y * target->width;
		uint x_fp = view.x << 14;
		const uint y_frac = y_fp & 16383;
		for (int x = 0; x < target->width; x++, x_fp += dx)
		{
			const uint mapPos = x_fp >> 14;
			const uint p1 = mapLine[mapPos];
			const uint p2 = mapLine[mapPos + 1];
			const uint p3 = mapLine[mapPos + width];
			const uint p4 = mapLine[mapPos + width + 1];
			const uint x_frac = x_fp & 16383;
			const uint w1 = ((16383 - x_frac) * (16383 - y_frac)) >> 20;
			const uint w2 = (x_frac * (16383 - y_frac)) >> 20;
			const uint w3 = ((16383 - x_frac) * y_frac) >> 20;
			const uint w4 = 255 - (w1 + w2 + w3);
			*dst++ = ScaleColor( p1, w1 ) + ScaleColor( p2, w2 ) +
				ScaleColor( p3, w3 ) + ScaleColor( p4, w4 );
		}
	}
}

int2 Map::ScreenToMap( int2 pos )
{
	float u = (float)pos.x / SCRWIDTH;
	float v = (float)pos.y / SCRHEIGHT;
	return make_int2( view.x + (int)(u * (view.z - view.x)), view.y + (int)(v * (view.w - view.y)) );
}