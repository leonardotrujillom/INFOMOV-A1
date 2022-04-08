#include "precomp.h"

TheApp* CreateApp() { return new MyApp(); }

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void MyApp::Init()
{
	// load tank sprites
	tank1 = new Sprite( "assets/tanks.png", make_int2( 128, 100 ), make_int2( 310, 360 ), 36, 256 );
	tank2 = new Sprite( "assets/tanks.png", make_int2( 327, 99 ), make_int2( 515, 349 ), 36, 256 );
	// load bush sprite for dust streams
	bush[0] = new Sprite( "assets/bush1.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 10, 256 );
	bush[1] = new Sprite( "assets/bush2.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 14, 256 );
	bush[2] = new Sprite( "assets/bush3.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 20, 256 );
	bush[0]->ScaleAlpha( 96 );
	bush[1]->ScaleAlpha( 64 );
	bush[2]->ScaleAlpha( 128 );
	// pointer
	pointer = new SpriteInstance( new Sprite( "assets/pointer.png" ) );
	// create armies
	for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++)
	{
		Actor* army1Tank = new Tank( tank1, make_int2( 1500 + x * 40 + 20, 1800 - y * 40 + 20 ), make_int2( 2000, 1500 ), 0, 0 );
		Actor* army2Tank = new Tank( tank2, make_int2( 2300 - x * 40, y * 40 + 700 ), make_int2( 2000, 1500 ), 10, 1 );
		actorPool.push_back( army1Tank );
		actorPool.push_back( army2Tank );
	}
	// load mountain peaks
	Surface mountains( "assets/peaks.png" );
	for (int y = 0; y < mountains.height; y++) for (int x = 0; x < mountains.width; x++)
	{
		uint p = mountains.pixels[x + y * mountains.width];
		if ((p & 0xffff) == 0) peaks.push_back( make_float3( make_int3( x * 8, y * 8, (p >> 16) & 255 ) ) );
	}
	// add sandstorm
	for( int i = 0; i < 7500; i++ )
	{
		int x = RandomUInt() % map.bitmap->width;
		int y = RandomUInt() % map.bitmap->height;
		int d = (RandomUInt() & 15) - 8;
		sand.push_back( new Particle( bush[i % 3], make_int2( x, y ), map.bitmap->pixels[x + y * map.bitmap->width], d ) );
	}
	// place flags
	Surface* flagPattern = new Surface( "assets/flag.png" );
	VerletFlag* flag1 = new VerletFlag( make_int2( 3000, 848 ), flagPattern );
	actorPool.push_back( flag1 );
	VerletFlag* flag2 = new VerletFlag( make_int2( 1076, 1870 ), flagPattern );
	actorPool.push_back( flag2 );
	// initialize map view
	map.UpdateView( screen, zoom );
}

// -----------------------------------------------------------
// Advanced zooming
// -----------------------------------------------------------
void MyApp::MouseWheel( float y )
{
	// fetch current pointer location
	int2 pointerPos = map.ScreenToMap( mousePos );
	// adjust zoom
	zoom -= 10 * y; 
	if (zoom < 20) zoom = 20; 
	if (zoom > 100) zoom = 100;
	// adjust focus so that pointer remains stationary, if possible
	map.UpdateView( screen, zoom );
	int2 newPointerPos = map.ScreenToMap( mousePos );
	map.SetFocus( map.GetFocus() + (pointerPos - newPointerPos) );
	map.UpdateView( screen, zoom );
}

// -----------------------------------------------------------
// Process mouse input
// -----------------------------------------------------------
void MyApp::HandleInput()
{
	// anything that happens only once at application start goes here
	static bool wasDown = false, dragging = false;
	if (mouseDown && !wasDown) dragging = true, dragStart = mousePos, focusStart = map.GetFocus();
	if (!mouseDown) dragging = false;
	wasDown = mouseDown;
	if (dragging)
	{
		int2 delta = dragStart - mousePos;
		delta.x = (int)((delta.x * zoom) / 32);
		delta.y = (int)((delta.y * zoom) / 32);
		map.SetFocus( focusStart + delta );
		map.UpdateView( screen, zoom );
	}
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void MyApp::Tick( float deltaTime )
{
	Timer t;
	// draw the map
	map.Draw( screen );
	// rebuild actor grid
	grid.Clear();
	grid.Populate( actorPool );
	// update and render actors
	pointer->Remove();
	for (int s = (int)sand.size(), i = s - 1; i >= 0; i--) sand[i]->Remove();
	for (int s = (int)actorPool.size(), i = s - 1; i >= 0; i--) actorPool[i]->Remove();
	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Tick();
	for (int i = 0; i < (int)actorPool.size(); i++) if (!actorPool[i]->Tick())
	{
		// actor got deleted, replace by last in list
		Actor* lastActor = actorPool.back();
		Actor* toDelete = actorPool[i];
		actorPool.pop_back();
		if (lastActor != toDelete) actorPool[i] = lastActor;
		delete toDelete;
		i--;
	}
	for (int s = (int)actorPool.size(), i = 0; i < s; i++) actorPool[i]->Draw();
	for (int s = (int)sand.size(), i = 0; i < s; i++) sand[i]->Draw();
	int2 cursorPos = map.ScreenToMap( mousePos );
	pointer->Draw( map.bitmap, make_float2( cursorPos ), 0 );
	// handle mouse
	HandleInput();
	// report frame time
	static float frameTimeAvg = 10.0f; // estimate
	frameTimeAvg = 0.95f * frameTimeAvg + 0.05f * t.elapsed() * 1000;
	printf( "frame time: %5.2fms\n", frameTimeAvg );
}