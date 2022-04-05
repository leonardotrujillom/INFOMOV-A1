#pragma once

namespace Tmpl8
{

class MyApp;

class Actor
{
public:
	enum { TANK = 0, BULLET, FLAG };
	Actor() = default;
	virtual void Remove() { sprite.Remove(); }
	virtual bool Tick() = 0;
	virtual uint GetType() = 0;
	virtual void Draw() { sprite.Draw( Map::bitmap, make_int2( pos ), frame ); }
	SpriteInstance sprite;
	float2 pos, dir;
	int frame;
	static inline float2* directions = 0;
};

class Tank : public Actor
{
public:
	Tank( Sprite* s, int2 p, int2 t, int f, int a );
	bool Tick();
	uint GetType() { return Actor::TANK; }
	float2 target;
	int army, delay = 0;
	bool hitByBullet = false;
};

class Bullet : public Actor
{
public:
	Bullet( int2 p, int f, int a );
	void Remove();
	bool Tick();
	void Draw();
	uint GetType() { return Actor::BULLET; }
	SpriteInstance flashSprite;
	int frameCounter, army;
	static inline Sprite* flash = 0, *bullet = 0;
};

class Particle
{
public:
	Particle() = default;
	Particle( Sprite* s, int2 p, uint c, uint d );
	void Remove();
	void Tick();
	void Draw();
	uint backup[4], color = 0, frame, frameChange;
	bool hasBackup = false;
	SpriteInstance sprite;
	float2 pos, dir;
};

} // namespace Tmpl8