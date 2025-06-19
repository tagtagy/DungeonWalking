#include "stdafx.h"
#include "Camera.hpp"


Camera::Camera(Point _pos) {
	CameraPos = _pos;
}

void Camera::MoveCamera(int _PieceSize, int _WallThickness, Point _Player) {
	Point pos = Point{ (_PieceSize * _Player.x) + (_WallThickness * (_Player.x + 1)),
					   (_PieceSize * _Player.y) + (_WallThickness * (_Player.y + 1))};

	CameraPos = pos - Point((800 - 150) / 2, (600 - 150) / 2);
}
