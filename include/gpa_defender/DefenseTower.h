#pragma once
#ifndef DEFENSE_TOWER_H
#define DEFENSE_TOWER_H

#include <string>
#include <vector>
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/Enemy.h"

/**
 * @class DefenseTower
 * @brief Top-level defense tower base class (abstract, Level 1 inheritance).
 *
 * Common attributes: purchase cost, placed position, attack range,
 * damage, attack interval, cooldown timer.
 *
 * Common behaviors: per-frame update (cooldown + target picking + throttled
 * attack), range check. attack() and draw() are virtual so subclasses
 * implement their own behavior (polymorphism).
 *
 * Note: at this stage enemies do NOT attack towers, so towers do not
 * track HP or destruction.
 */
class DefenseTower {
protected:
    std::string towerName;     // tower display name
    int cost;                  // gold cost to buy
    Vector2D position;         // center coordinate on the map
    float range;               // attack range (radius around position)
    int damage;                // damage per attack
    float attackInterval;      // seconds between two attacks
    float cooldownTimer;       // time accumulated since last attack
    bool placed;               // whether the tower has been placed on the map

    // All living enemies within circular range (used by AI sweep / library slow).
    std::vector<Enemy*> collectAliveInRange(const std::vector<Enemy*>& enemies) const;

public:
    DefenseTower(std::string name, int cost, float range, int damage, float attackInterval);
    virtual ~DefenseTower() = default;

    // Place this tower at the given map position.
    void place(const Vector2D& pos);

    // Called every frame by the main loop:
    // accumulate cooldown -> pick a target -> attack.
    virtual void update(float deltaTime, const std::vector<Enemy*>& enemies);

    // Polymorphic attack: deal damage to one enemy. Default impl just
    // calls takeDamage(); subclasses can override to print themed lines
    // or add side effects.
    virtual void attack(Enemy& target);

    // Pure-virtual rendering hook for the front-end.
    virtual void draw() = 0;

    // Helpers
    bool canAttackNow() const;                                // is the cooldown ready?
    bool isInRange(const Enemy& enemy) const;                 // is this enemy within range?
    Enemy* pickTarget(const std::vector<Enemy*>& enemies) const; // nearest live enemy in range

    // Getters
    const std::string& getName() const { return towerName; }
    int getCost() const { return cost; }
    Vector2D getPosition() const { return position; }
    float getRange() const { return range; }
    int getDamage() const { return damage; }
    float getAttackInterval() const { return attackInterval; }
    bool isPlaced() const { return placed; }
};

// --- Five concrete subclasses (Level 2 inheritance) ---

/**
 * @class CoffeeTower
 * @brief Coffee: small radius, very high single-target damage (game design table).
 */
class CoffeeTower : public DefenseTower {
public:
    CoffeeTower();
    void attack(Enemy& target) override;
    void draw() override;
};

/**
 * @class AITower
 * @brief AI: 360-degree sweep �� damages every enemy in range each salvo.
 *        Upgrades (Doubao -> DeepSeek -> GPT) raise per-target damage / range / cadence.
 */
class AITower : public DefenseTower {
private:
    int level;                          // current level (starts at 1)
    int maxLevel;                       // max level
    std::vector<int> upgradeCosts;      // upgradeCosts[i] = gold to go from level (i+1) to (i+2)
    std::vector<std::string> levelNames;// display name per level

public:
    AITower();

    // Whether the tower can still upgrade and the player has enough gold.
    bool canUpgrade(int playerGold) const;

    // Perform the upgrade: deduct gold, raise level, grow stats.
    // Returns true on success, false if max level or not enough gold.
    bool upgrade(int& playerGold);

    // Gold required for the next upgrade; returns -1 when already maxed.
    int nextUpgradeCost() const;

    int getLevel() const { return level; }
    int getMaxLevel() const { return maxLevel; }

    void update(float deltaTime, const std::vector<Enemy*>& enemies) override;

    void attack(Enemy& target) override;
    void draw() override;
};

/**
 * @class LibraryTower
 * @brief Library: enemies inside radius are slowed (no direct HP damage here).
 */
class LibraryTower : public DefenseTower {
private:
    float slowFactor;      // multiplies enemy move speed (e.g. 0.45)
    float slowDurationSec; // how long each pulse refreshes slow

public:
    LibraryTower();

    void update(float deltaTime, const std::vector<Enemy*>& enemies) override;

    void attack(Enemy& target) override;
    void draw() override;
};

/**
 * @class ClassTower
 * @brief Class: one heavy hit, long downtime between attacks.
 */
class ClassTower : public DefenseTower {
public:
    ClassTower();
    void attack(Enemy& target) override;
    void draw() override;
};

/**
 * @class BilibiliTower
 * @brief Bilibili: straight-line attack from tower center along fireDirection.
 *        Place with setFireDirection; upgrade can rotate firing axis (spends gold).
 */
class BilibiliTower : public DefenseTower {
private:
    Vector2D fireDirection; // normalized; default +X
    static constexpr float kLaneHalfWidth = 38.0f;

    bool isEnemyOnAttackRay(const Enemy& enemy) const;
    Enemy* pickNearestEnemyOnRay(const std::vector<Enemy*>& enemies) const;

public:
    BilibiliTower();

    void setFireDirection(const Vector2D& dir);

    // After place(pos), call this to aim (required by design). If omitted, +X is used.
    Vector2D getFireDirection() const { return fireDirection; }

    // Pay gold to rotate firing direction 90 degrees (clockwise). For UI upgrade hook.
    bool purchaseDirectionChange(int& playerGold, int goldCost = 40);

    void update(float deltaTime, const std::vector<Enemy*>& enemies) override;

    void attack(Enemy& target) override;
    void draw() override;
};

#endif // DEFENSE_TOWER_H

