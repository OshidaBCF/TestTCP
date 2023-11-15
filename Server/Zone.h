#pragma once

struct Vector2i
{
	int x;
	int y;
	Vector2i() : x(0), y(0) {};
	Vector2i(int x, int y) : x(x), y(y) {}
};

class zone
{
public:
	Vector2i coordinates;
	enum painterList
	{
		NONE = 0,
		CIRCLE = 1,
		CROSS = 2
	};
	int painter = painterList::NONE;
	zone(Vector2i coordinates) : coordinates(coordinates) {}
};