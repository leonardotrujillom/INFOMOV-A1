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
		MyApp::actorPool.push_back( new ParticleExplosion( this ) );
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
	for (int i = 0; i < nearby.count; i++)
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
	else if (steer > 0.2f) frame = (frame + 1) & 255, dir = directions[frame], speed = 0.35f;
	else {
		// tank tracks, only when not turning
		float2 perp( -dir.y, dir.x );
		float2 trackPos1 = pos - 9 * dir + 4.5f * perp;
		float2 trackPos2 = pos - 9 * dir - 5.5f * perp;
		MyApp::map.bitmap->BlendBilerp( trackPos1.x, trackPos1.y, 0, 12 );
		MyApp::map.bitmap->BlendBilerp( trackPos2.x, trackPos2.y, 0, 12 );
	}
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
	if (frameCounter == 1)
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
	if (frameCounter == 110) 
	{
		MyApp::actorPool.push_back( new SpriteExplosion( this ) );
		return false;
	}
	// destroy bullet if it leaves the map
	if (pos.x < 0 || pos.y < 0 || pos.x > MyApp::map.width || pos.y > MyApp::map.height) return false;
	// check if the bullet hit a tank
	ActorList& tanks = MyApp::grid.FindNearbyTanks( pos, 0 );
	for (int s = (int)tanks.count, i = 0; i < s; i++)
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
		flashSprite.Draw( Map::bitmap, pos, 0 );
	else
		sprite.Draw( Map::bitmap, pos, frame );
}

ParticleExplosion::ParticleExplosion( Tank* tank )
{
	// read the pixels from the sprite of the specified tank
	Sprite* sprite = tank->sprite.sprite;
	uint size = sprite->frameSize;
	uint stride = sprite->frameSize * sprite->frameCount;
	uint* src = sprite->pixels + tank->frame * size;
	for (uint y = 0; y < size; y++) for (uint x = 0; x < size; x++)
	{
		uint pixel = src[x + y * stride];
		uint alpha = pixel >> 24;
		if (alpha > 64) for( int i = 0; i < 2; i++ )
		{
			color.push_back( pixel & 0xffffff );
			float fx = tank->pos.x - size * 0.5f + x;
			float fy = tank->pos.y - size * 0.5f + y;
			pos.push_back( make_float2( fx, fy ) );
			dir.push_back( make_float2( 0, 0 ) );
		}
	}
}

void ParticleExplosion::Draw()
{
	if (!backup) backup = new uint[(int)pos.size() * 4];
	for (int s = (int)pos.size(), i = 0; i < s; i++)
	{
		int2 intPos = make_int2( pos[i] );
		backup[i * 4 + 0] = MyApp::map.bitmap->Read( intPos.x, intPos.y );
		backup[i * 4 + 1] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y );
		backup[i * 4 + 2] = MyApp::map.bitmap->Read( intPos.x, intPos.y + 1 );
		backup[i * 4 + 3] = MyApp::map.bitmap->Read( intPos.x + 1, intPos.y + 1 );
		MyApp::map.bitmap->BlendBilerp( pos[i].x, pos[i].y, color[i], fade );
	}
}

bool ParticleExplosion::Tick()
{
	for (int s = (int)pos.size(), i = 0; i < s; i++)
	{
		pos[i] += dir[i];
		dir[i] -= make_float2( RandomFloat() * 0.05f + 0.02f, RandomFloat() * 0.02f - 0.01f );
	}
	if (fade-- == 0) return false; else return true;
}

void ParticleExplosion::Remove()
{
	for (int s = (int)pos.size(), i = s - 1; i >= 0; i--)
	{
		int2 intPos = make_int2( pos[i] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y, backup[i * 4 + 0] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y, backup[i * 4 + 1] );
		MyApp::map.bitmap->Plot( intPos.x, intPos.y + 1, backup[i * 4 + 2] );
		MyApp::map.bitmap->Plot( intPos.x + 1, intPos.y + 1, backup[i * 4 + 3] );
	}
}

SpriteExplosion::SpriteExplosion( Bullet* bullet )
{
	if (!anim) anim = new Sprite( "assets/explosion1.png", 16 );
	sprite = SpriteInstance( anim );
	pos = bullet->pos;
	frame = 0;
}

Particle::Particle( Sprite* s, int2 p, uint c, uint d )
{
	pos = make_float2( p );
	dir = make_float2( -1 - RandomFloat() * 4, 0 );
	color = c;
	frameChange = d;
	sprite = SpriteInstance( s );
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
