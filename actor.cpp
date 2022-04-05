#include "precomp.h"

Tank::Tank( Sprite* s, int2 p, int2 t, int f, int a )
{
	sprite = SpriteInstance( s );
	pos = make_float2( p );
	target = make_float2( t );
	frame = f; // 0..255; 0: north; 64: east; 128: south; 192: east
	army = a;
	if (!directions)
	{
		directions = new float2[256];
		for (int a = 0; a < 256; a++)
			directions[a] = make_float2( sinf( a * PI / 128 ), -cosf( a * PI / 128 ) );
	}
	dir = directions[f];
}

bool Tank::Tick()
{
	// behaviour: accumulate forces for steering left or right
	// 0. handle incoming bullets
	if (hitByBullet)
	{
		return false;
	}
	// 1. target attracts
	float2 toTarget = normalize( target - pos );
	float2 toRight = make_float2( -dir.y, dir.x );
	float steer = 2 * dot( toRight, toTarget );
	// 2. mountains repel
	float2 probePos = pos + 8 * dir;
	for (int s = (int)MyApp::peaks.size(), i = 0; i < s; i++)
	{
		float peakMag = MyApp::peaks[i].z / 2;
		float2 toPeak = make_float2( MyApp::peaks[i].x, MyApp::peaks[i].y ) - probePos;
		float sqrDist = dot( toPeak, toPeak );
		if (sqrDist < sqrf( peakMag ))
			toPeak = normalize( toPeak ),
			steer -= dot( toRight, toPeak ) * peakMag / sqrtf( sqrDist );
	}
	// 3. evade other tanks
	ActorList& nearby = MyApp::grid.FindNearbyTanks( this );
	for( int i = 0; i < nearby.count; i++ )
	{
		Tank* tank = nearby.tank[i];
		float2 toActor = tank->pos - this->pos;
		float sqrDist = dot( toActor, toActor );
		if (sqrDist < 400 && dot( toActor, dir ) > 0.35f)
		{
			steer -= (400 - sqrDist) * 0.02f * dot( toActor, toRight ) > 0 ? 1 : -1;
			break;
		}
	}
	// 4. fire
	if (delay++ > 200)
	{
		ActorList& nearby = MyApp::grid.FindNearbyTanks( pos + dir * 200 );
		for (int i = 0; i < nearby.count; i++) if (nearby.tank[i]->army != this->army)
		{
			float2 toActor = normalize( nearby.tank[i]->pos - this->pos );
			if (dot( toActor, dir ) > 0.8f) // spawn bullet
			{
				Bullet* newBullet = new Bullet( make_int2( pos + 20 * dir ), frame, army );
				MyApp::actorPool.push_back( newBullet );
				delay = 0;
				break;
			}
		}
	}
	// adjust heading
	float speed = 1.0f;
	if (steer < -0.2f) frame = (frame + 255 /* i.e. -1 */) & 255, dir = directions[frame], speed = 0.35f;
	if (steer > 0.2f) frame = (frame + 1) & 255, dir = directions[frame], speed = 0.35f;
	pos += dir * speed * 0.25f;
	// tanks never die
	return true;
}

Bullet::Bullet( int2 p, int f, int a )
{
	pos = make_float2( p );
	dir = directions[f];
	frame = f;
	frameCounter = 0;
	army = a;
	if (!flash)
	{
		// load sprites
		flash = new Sprite( "assets/flash.png" );
		bullet = new Sprite( "assets/bullet.png", make_int2( 2, 2 ), make_int2( 31, 31 ), 32, 256 );
	}
	sprite = SpriteInstance( bullet );
	flashSprite = SpriteInstance( flash );
}

void Bullet::Remove() 
{ 
	if (frameCounter == 1 || frameCounter == 159) 
		flashSprite.Remove(); 
	else 
		sprite.Remove(); 
}

bool Bullet::Tick()
{
	// update bullet position
	pos += dir * 8;
	// destroy bullet if it travelled too long
	frameCounter++;
	if (frameCounter == 160) return false;
	// destroy bullet if it leaves the map
	if (pos.x < 0 || pos.y < 0 || pos.x > MyApp::map.width || pos.y > MyApp::map.height) return false;
	// check if the bullet hit a tank
	ActorList& tanks = MyApp::grid.FindNearbyTanks( pos, 0 );
	for( int s = (int)tanks.count, i = 0; i < s; i++ )
	{
		Tank* tank = tanks.tank[i]; // a tank, thankfully
		if (tank->army != this->army) continue; // no friendly fire. Disable for madness.
		float dist = length( this->pos - tank->pos );
		if (dist < 10) 
		{
			tank->hitByBullet = true; // tank will need to draw it's own conclusion
			return false; // bees die from stinging. Disable for rail gun.
		}
	}
	// stayin' alive
	return true;
}

void Bullet::Draw() 
{ 
	if (frameCounter == 1 || frameCounter == 159) 
		flashSprite.Draw( Map::bitmap, make_int2( pos ), 0 ); 
	else 
		sprite.Draw( Map::bitmap, make_int2( pos ), frame ); 
}

Particle::Particle( Sprite* s, int2 p, uint c, uint d )
{
	pos = make_float2( p );
	dir = make_float2( -1 - RandomFloat() * 4, 0 );
	color = c;
	frameChange = d;
	sprite = SpriteInstance( s );
}

void Particle::Remove()
{
#if 1
	sprite.Remove();
#else
	if (!hasBackup) return;
	int2 intPos = make_int2( pos );
	MyApp::map.bitmap->Plot( intPos.x, intPos.y, backup[0] );
	MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y, backup[1] );
	MyApp::map.bitmap->Plot( intPos.x, intPos.y + 1, backup[2] );
	MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y + 1, backup[3] );
#endif
}

void Particle::Tick()
{
	pos += dir;
	dir.y *= 0.95f;
	if (pos.x < 0) 
	{
		pos.x = (float)(MyApp::map.bitmap->width - 1);
		pos.y = (float)(RandomUInt() % MyApp::map.bitmap->height);
		dir = make_float2( -1 - RandomFloat() * 2, 0 );
	}
	for (int s = (int)MyApp::peaks.size(), i = 0; i < s; i++)
	{
		float2 toPeak = make_float2( MyApp::peaks[i].x, MyApp::peaks[i].y ) - pos;
		float g = MyApp::peaks[i].z * 0.02f / sqrtf( dot( toPeak, toPeak ) );
		toPeak = normalize( toPeak );
		dir.y -= toPeak.y * g;
	}
	dir.y += RandomFloat() * 0.05f - 0.025f;
	frame = (frame + frameChange + 256) & 255;
}

void Particle::Draw()
{
	int2 intPos = make_int2( pos );
#if 1
	sprite.Draw( Map::bitmap, intPos, frame );
#else
	backup[0] = MyApp::map.bitmap->Read( intPos.x, intPos.y );
	backup[1] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y );
	backup[2] = MyApp::map.bitmap->Read( intPos.x, intPos.y + 1 );
	backup[3] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y + 1 );
	hasBackup = true;
	float frac_x = pos.x - intPos.x;
	float frac_y = pos.y - intPos.y;
	int w1 = (int)(256 * ((1 - frac_x) * (1 - frac_y)));
	int w2 = (int)(256 * (frac_x * (1 - frac_y)));
	int w3 = (int)(256 * ((1 - frac_x) * frac_y));
	int w4 = (int)(256 * (frac_x * frac_y));
	MyApp::map.bitmap->Blend( intPos.x, intPos.y, color, w1 );
	MyApp::map.bitmap->Blend( intPos.x + 1, intPos.y, color, w2 );
	MyApp::map.bitmap->Blend( intPos.x, intPos.y + 1, color, w3 );
	MyApp::map.bitmap->Blend( intPos.x + 1, intPos.y + 1, color, w4 );
#endif
}