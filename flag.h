#pragma once

namespace Tmpl8
{

class VerletFlag : public Actor
{
public:
	VerletFlag( int2 location, Surface* pattern );
	void Draw();
	bool Tick();
	uint GetType() { return Actor::FLAG; }
	void Remove();
	float2 polePos;
	float2* pos = 0;
	float2* prevPos = 0;
	uint* color = 0;
	uint* backup = 0;
	bool hasBackup = false;
	int width, height;
};

} // namespace Tmpl8#pragma once
