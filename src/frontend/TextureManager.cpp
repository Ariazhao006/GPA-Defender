#include "frontend/TextureManager.h"
#include "frontend/AssetPaths.h"
#include "raylib.h"

namespace frontend {

void TextureManager::addSprite(const std::string& name, int x, int y, int w, int h,
                                Texture2D* tex) {
	sprites[name] = {{static_cast<float>(x), static_cast<float>(y),
	                  static_cast<float>(w), static_cast<float>(h)}, tex};
}

bool TextureManager::loadAiTexture(const std::string& name, const std::string& path) {
	const std::string resolvedPath = resolveAssetPath(path);
	Texture2D tex = LoadTexture(resolvedPath.c_str());
	if (tex.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", resolvedPath.c_str());
		return false;
	}
	aiTextures.push_back(tex);
	// Register the full image as a sprite
	addSprite(name, 0, 0, tex.width, tex.height, &aiTextures.back());
	return true;
}

const SpriteDef* TextureManager::getSprite(const std::string& name) const {
	auto it = sprites.find(name);
	return (it != sprites.end()) ? &it->second : nullptr;
}

bool TextureManager::loadAll() {
	bool ok = true;

	const std::string mapTilesheetPath = resolveAssetPath(
	    "assets/image/kenney_map-pack/Tilesheet/mapPack_tilesheet.png");
	mapTilesheet = LoadTexture(mapTilesheetPath.c_str());
	if (mapTilesheet.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", mapTilesheetPath.c_str());
		ok = false;
	}

	const std::string charSpritesheetPath = resolveAssetPath(
	    "assets/image/kenney_shape-characters/Spritesheet/spritesheet_default.png");
	charSpritesheet = LoadTexture(charSpritesheetPath.c_str());
	if (charSpritesheet.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", charSpritesheetPath.c_str());
		ok = false;
	}

	const std::string highlandTile017Path = resolveAssetPath(
	    "assets/image/kenney_map-pack/PNG/mapTile_017.png");
	highlandTile017 = LoadTexture(highlandTile017Path.c_str());
	if (highlandTile017.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", highlandTile017Path.c_str());
		ok = false;
	}

	const std::string highlandTile015Path = resolveAssetPath(
	    "assets/image/kenney_map-pack/PNG/mapTile_015.png");
	highlandTile015 = LoadTexture(highlandTile015Path.c_str());
	if (highlandTile015.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", highlandTile015Path.c_str());
		ok = false;
	}

	const std::string pathTilePath = resolveAssetPath(
	    "assets/image/kenney_map-pack/PNG/mapTile_010.png");
	pathTile = LoadTexture(pathTilePath.c_str());
	if (pathTile.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", pathTilePath.c_str());
		ok = false;
	}

	const std::string spawnTilePath = resolveAssetPath(
	    "assets/image/kenney_shape-characters/PNG/Default/tile_exclamation.png");
	spawnTile = LoadTexture(spawnTilePath.c_str());
	if (spawnTile.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load texture: %s", spawnTilePath.c_str());
		ok = false;
	}

	// --- Character body sprites (80x80 each) ---
	addSprite("blue_body_circle",    0,   147, 80, 80, &charSpritesheet);
	addSprite("blue_body_rhombus",   240, 480, 80, 80, &charSpritesheet);
	addSprite("blue_body_square",    176, 0,   80, 80, &charSpritesheet);
	addSprite("blue_body_squircle",  240, 240, 80, 80, &charSpritesheet);
	addSprite("green_body_circle",   336, 0,   80, 80, &charSpritesheet);
	addSprite("green_body_rhombus",  240, 320, 80, 80, &charSpritesheet);
	addSprite("green_body_square",   240, 160, 80, 80, &charSpritesheet);
	addSprite("green_body_squircle", 160, 480, 80, 80, &charSpritesheet);
	addSprite("pink_body_circle",    240, 400, 80, 80, &charSpritesheet);
	addSprite("pink_body_rhombus",   160, 240, 80, 80, &charSpritesheet);
	addSprite("pink_body_square",    320, 80,  80, 80, &charSpritesheet);
	addSprite("pink_body_squircle",  320, 240, 80, 80, &charSpritesheet);
	addSprite("purple_body_circle",  320, 160, 80, 80, &charSpritesheet);
	addSprite("purple_body_rhombus", 400, 160, 80, 80, &charSpritesheet);
	addSprite("purple_body_square",  400, 80,  80, 80, &charSpritesheet);
	addSprite("purple_body_squircle",320, 480, 80, 80, &charSpritesheet);
	addSprite("red_body_circle",     320, 400, 80, 80, &charSpritesheet);
	addSprite("red_body_rhombus",    320, 320, 80, 80, &charSpritesheet);
	addSprite("red_body_square",     256, 0,   80, 80, &charSpritesheet);
	addSprite("red_body_squircle",   160, 400, 80, 80, &charSpritesheet);
	addSprite("yellow_body_circle",  0,   387, 80, 80, &charSpritesheet);
	addSprite("yellow_body_rhombus", 80,  227, 80, 80, &charSpritesheet);
	addSprite("yellow_body_square",  0,   227, 80, 80, &charSpritesheet);
	addSprite("yellow_body_squircle",0,   307, 80, 80, &charSpritesheet);

	// --- Faces (used for enemy expressions) ---
	addSprite("face_a", 109, 547, 50, 29, &charSpritesheet);
	addSprite("face_b", 59,  547, 50, 24, &charSpritesheet);
	addSprite("face_c", 0,   547, 59, 30, &charSpritesheet);
	addSprite("face_d", 400, 531, 53, 33, &charSpritesheet);
	addSprite("face_e", 400, 350, 55, 40, &charSpritesheet);
	addSprite("face_f", 400, 422, 55, 33, &charSpritesheet);
	addSprite("face_g", 400, 390, 55, 32, &charSpritesheet);
	addSprite("face_h", 400, 455, 55, 36, &charSpritesheet);

	// --- Decorations ---
	addSprite("shadow",       96,  122, 48, 20, &charSpritesheet);
	addSprite("tile_coin",    453, 531, 40, 40, &charSpritesheet);
	addSprite("tile_exclamation", 160, 80, 80, 80, &charSpritesheet);
	addSprite("tile_cloud",   455, 350, 40, 40, &charSpritesheet);
	addSprite("tile_bg_grass",455, 504, 32, 18, &charSpritesheet);

	// --- AI-generated custom tower/enemy textures ---
	aiTextures.reserve(15);  // prevent reallocation invalidating SpriteDef pointers
	const char* aiDir = "assets/image/no_background/";
	ok = loadAiTexture("ai_coffee",        std::string(aiDir) + "coffee.png") && ok;
	ok = loadAiTexture("ai_ai",            std::string(aiDir) + "ai.png") && ok;
	ok = loadAiTexture("ai_library",       std::string(aiDir) + "library.png") && ok;
	ok = loadAiTexture("ai_class",         std::string(aiDir) + "class.png") && ok;
	ok = loadAiTexture("ai_bilibili",      std::string(aiDir) + "bilibili.png") && ok;
	ok = loadAiTexture("ai_boss",          std::string(aiDir) + "boss.png") && ok;
	ok = loadAiTexture("ai_calculus",      std::string(aiDir) + "calculus.png") && ok;
	ok = loadAiTexture("ai_research",      std::string(aiDir) + "research.png") && ok;
	ok = loadAiTexture("ai_social",        std::string(aiDir) + "social_contact.png") && ok;
	ok = loadAiTexture("ai_morning",       std::string(aiDir) + "morning_eight.png") && ok;
	ok = loadAiTexture("ai_group",         std::string(aiDir) + "group_work.png") && ok;
	ok = loadAiTexture("ai_video",         std::string(aiDir) + "short_vedio.png") && ok;
	ok = loadAiTexture("ai_exam_outline",  std::string(aiDir) + "exam_outline.png") && ok;
	ok = loadAiTexture("ai_peer_pressure", std::string(aiDir) + "peer_pressure.png") && ok;
	ok = loadAiTexture("chest_case",       std::string(aiDir) + "case.png") && ok;

	return ok;
}

void TextureManager::unloadAll() {
	UnloadTexture(mapTilesheet);
	UnloadTexture(charSpritesheet);
	UnloadTexture(highlandTile017);
	UnloadTexture(highlandTile015);
	UnloadTexture(pathTile);
	UnloadTexture(spawnTile);
	for (auto& tex : aiTextures) {
		UnloadTexture(tex);
	}
	aiTextures.clear();
	sprites.clear();
}

}  // namespace frontend
