#pragma once

struct Vector2i
{
	int x;
	int y;
	Vector2i() {};
	Vector2i(int x, int y) : x(x), y(y) {}
};

class zone
{
public:
	Vector2i coordinates;
	enum painterList
	{
		CIRCLE = -1,
		NONE = 0,
		CROSS = 1
	};
	int painter = painterList::NONE;
	zone(Vector2i coordinates) : coordinates(coordinates) {}
};