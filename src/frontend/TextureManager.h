#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace frontend {

struct SpriteDef {
	Rectangle src;
	Texture2D* texture;
};

class TextureManager {
public:
	bool loadAll();
	void unloadAll();

	const SpriteDef* getSprite(const std::string& name) const;

	Texture2D mapTilesheet{};      // 1088x768, grid of 64x64 tiles
	Texture2D charSpritesheet{};   // character body parts
	Texture2D highlandTile017{};   // placeable tower tile, primary variant
	Texture2D highlandTile015{};   // placeable tower tile, accent variant
	Texture2D pathTile{};          // enemy path tile
	Texture2D spawnTile{};         // spawn marker tile

private:
	void addSprite(const std::string& name, int x, int y, int w, int h, Texture2D* tex);
	bool loadAiTexture(const std::string& name, const std::string& path);

	std::unordered_map<std::string, SpriteDef> sprites;
	std::vector<Texture2D> aiTextures;  // owns individual AI textures
};

}  // namespace frontend
