#include "stdafx.h"
#include "BasePlayer.hpp"

BasePlayer::BasePlayer() {
    // Player.x and Player.y are initialized by SetPlayerPos or default constructor of Point
}

Point BasePlayer::Move(int _x, int _y, Grid<int32>& mapData) {
    Point targetPos = Player + Point{_x, _y}; // Calculate target position

    // Check map boundaries
    if (targetPos.x >= 0 && targetPos.x < mapData.width() &&
        targetPos.y >= 0 && targetPos.y < mapData.height()) {

        int targetTileType = mapData[targetPos.y][targetPos.x]; // Get type of the tile player wants to move to

        if (targetTileType == 0) { // Moving to a standard floor tile (0)
            mapData[Player.y][Player.x] = 0;  // Set old player position on map to floor
            Player = targetPos;               // Update player's internal position
            mapData[Player.y][Player.x] = 2;  // Mark new player position on map as player tile (2)
            return Point{ -1,-1 };            // Successful move, no interaction
        }
        else if (targetTileType == 3) { // Moving onto an enemy tile (3) - In this game, it means attacking.
                                        // The problem description implies enemies are type 3 on the map.
                                        // However, current enemy spawning logic doesn't set tile type 3.
                                        // This branch might be unused if enemies are not marked on map grid.
                                        // For now, keeping logic as specified by problem.
            return targetPos; // Player intends to attack, does not move. Return enemy position.
        }
        else if (targetTileType == 4) { // Moving onto a goal tile (4)
            mapData[Player.y][Player.x] = 0;  // Set old player position on map to floor
            Player = targetPos;               // Update player's internal position
            // The tile at mapData[Player.y][Player.x] (new position) remains 4 (Goal).
            // Game::InputMove will check this tile type to trigger scene change.
            // Game::draw will render tile 4 as yellow, and player sprite can be overlaid.
            return Point{ -2,-2 };            // Signal that goal was reached
        }
        // If targetTileType is 1 (wall) or any other non-explicitly handled type (e.g. 2 if somehow trying to move onto self), movement is blocked.
        // No change in player position or mapData needed for blocked moves.
    }

    // Indicates no move occurred (hit boundary or blocked by unhandled/wall tile type)
    return Point{ -1,-1 };
}

