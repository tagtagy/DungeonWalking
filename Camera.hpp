#pragma once
class Camera
{
public:

	Camera(Point _pos);

	void MoveCamera(int _PieceSize, int _WallThickness,Point _Player);

	Point GetCamera() { return CameraPos; }

private:
	Point CameraPos = { 0,0 };
};

