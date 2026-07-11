#include "gpa_defender/DefenseTower.h"
#include <algorithm>
#include <cmath>
#include <iostream> // only for console testing without a UI
#include <utility>

namespace {

constexpr float kTowerRangeScale = 1.5f;

Vector2D enemyCenter(const Enemy& enemy) {
    Rect box = enemy.getBoundingBox();
    return {box.x + box.width / 2.0f, box.y + box.height / 2.0f};
}

} // namespace

// --- DefenseTower base class ---

DefenseTower::DefenseTower(std::string name, int cost, float range, int damage, float attackInterval)
    : towerName(std::move(name)), cost(cost), position{ 0.0f, 0.0f },
    range(range), damage(damage), attackInterval(attackInterval),
    cooldownTimer(0.0f), placed(false) {}

void DefenseTower::place(const Vector2D& pos) {
    position = pos;
    placed = true;
    cooldownTimer = attackInterval;
}

void DefenseTower::restoreBaseState(const Vector2D& pos, float cooldown) {
    position = pos;
    placed = true;
    cooldownTimer = cooldown;
    if (cooldownTimer < 0.0f) cooldownTimer = 0.0f;
    if (cooldownTimer > attackInterval) cooldownTimer = attackInterval;
}

void DefenseTower::setDamageMultiplier(float multiplier) {
    damageMultiplier = std::max(0.0f, multiplier);
}

int DefenseTower::effectiveDamage(int baseDamage) const {
    if (baseDamage <= 0) return 0;
    return std::max(1, static_cast<int>(baseDamage * damageMultiplier + 0.5f));
}

int DefenseTower::dealDamage(Enemy& target, int baseDamage) const {
    return target.takeDamage(effectiveDamage(baseDamage));
}

std::vector<Enemy*> DefenseTower::collectAliveInRange(const std::vector<Enemy*>& enemies) const {
    std::vector<Enemy*> out;
    for (Enemy* e : enemies) {
        if (e == nullptr) continue;
        if (e->getState() == EnemyState::DEAD) continue;
        if (isInRange(*e)) {
            out.push_back(e);
        }
    }
    return out;
}

bool DefenseTower::canAttackNow() const {
    return cooldownTimer >= attackInterval;
}

bool DefenseTower::isInRange(const Enemy& enemy) const {
    Rect box = enemy.getBoundingBox();
    Vector2D enemyCenter = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };
    return position.distanceTo(enemyCenter) <= range;
}

Enemy* DefenseTower::pickTarget(const std::vector<Enemy*>& enemies) const {
    Enemy* nearest = nullptr;
    float nearestDist = range;

    for (Enemy* e : enemies) {
        if (e == nullptr) continue;
        if (e->getState() == EnemyState::DEAD) continue;

        Rect box = e->getBoundingBox();
        Vector2D enemyCenter = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };
        float d = position.distanceTo(enemyCenter);

        if (d <= nearestDist) {
            nearestDist = d;
            nearest = e;
        }
    }
    return nearest;
}

void DefenseTower::update(float deltaTime, const std::vector<Enemy*>& enemies,
    std::vector<TowerAttackEvent>* events) {
    if (!placed) return;

    cooldownTimer += deltaTime;
    if (!canAttackNow()) return;

    Enemy* target = pickTarget(enemies);
    if (target == nullptr) return;

    if (events != nullptr) {
        TowerAttackEvent event;
        event.kind = effectKind();
        event.origin = position;
        event.targets.push_back(enemyCenter(*target));
        event.range = range;
        events->push_back(std::move(event));
    }
    attack(*target);
    cooldownTimer = 0.0f;
}

void DefenseTower::attack(Enemy& target) {
    dealDamage(target, damage);
}

void AreaEffectTower::update(float deltaTime, const std::vector<Enemy*>& enemies,
    std::vector<TowerAttackEvent>* events) {
    if (!placed) return;

    cooldownTimer += deltaTime;
    if (!canAttackNow()) return;

    std::vector<Enemy*> targets = collectAliveInRange(enemies);
    if (targets.empty()) return;

    if (events != nullptr) {
        TowerAttackEvent event;
        event.kind = effectKind();
        event.origin = position;
        event.range = range;
        for (Enemy* target : targets) {
            event.targets.push_back(enemyCenter(*target));
        }
        events->push_back(std::move(event));
    }

    applyAreaEffect(targets);
    cooldownTimer = 0.0f;
}

// --- Coffee: small radius, very high burst damage ---

CoffeeTower::CoffeeTower()
    : DefenseTower("Coffee", 50, 92.0f * kTowerRangeScale, 88, 0.85f) {}

void CoffeeTower::attack(Enemy& target) {
    const int dealt = effectiveDamage(damage);
    std::cout << "[Tower] Coffee (small arc, huge hit) -> " << dealt << " damage\n";
    target.takeDamage(dealt);
}

void CoffeeTower::draw() {}

// --- Library: aura slow in radius (no HP damage from this tower) ---

LibraryTower::LibraryTower()
    : AreaEffectTower("Library", 120, 228.0f * kTowerRangeScale, 0, 1.05f),
    slowFactor(0.45f),
    slowDurationSec(2.85f) {}

void LibraryTower::applyAreaEffect(const std::vector<Enemy*>& targets) {
    for (Enemy* e : targets) {
        e->applySlowEffect(slowFactor, slowDurationSec);
    }

    std::cout << "[Tower] Library atmosphere slows " << targets.size()
        << " enemy stack(s)\n";
}

void LibraryTower::attack(Enemy&) {
    // Damage comes from other towers; library only applies slow via update().
}

void LibraryTower::draw() {}

// --- Class: heavy hit, long recharge ---

ClassTower::ClassTower()
    : DefenseTower("Class", 80, 172.0f * kTowerRangeScale, 8, 0.25f) {}

Enemy* ClassTower::pickLockedOrNewTarget(const std::vector<Enemy*>& enemies) {
    Enemy* locked = nullptr;
    float lockedDist = 34.0f;
    if (hasLockedTarget) {
        for (Enemy* e : enemies) {
            if (e == nullptr || e->getState() == EnemyState::DEAD) continue;
            if (!isInRange(*e)) continue;
            const Vector2D center = enemyCenter(*e);
            const float dist = lockedTargetCenter.distanceTo(center);
            if (dist <= lockedDist) {
                locked = e;
                lockedDist = dist;
            }
        }
    }

    if (locked != nullptr) {
        return locked;
    }

    Enemy* next = pickTarget(enemies);
    if (next == nullptr) {
        hasLockedTarget = false;
        focusTime = 0.0f;
        formulaPulseTimer = 0.0f;
    }
    return next;
}

void ClassTower::update(float deltaTime, const std::vector<Enemy*>& enemies,
    std::vector<TowerAttackEvent>* events) {
    if (!placed) return;

    Enemy* target = pickLockedOrNewTarget(enemies);
    if (target == nullptr) return;

    const Vector2D targetCenter = enemyCenter(*target);
    if (!hasLockedTarget || lockedTargetCenter.distanceTo(targetCenter) > 34.0f) {
        hasLockedTarget = true;
        focusTime = 0.0f;
        formulaPulseTimer = 0.0f;
    }

    lockedTargetCenter = targetCenter;
    focusTime += deltaTime;
    formulaPulseTimer += deltaTime;

    constexpr float kPulseInterval = 0.25f;
    constexpr float kRampTime = 2.5f;
    constexpr int kStartDamage = 7;
    constexpr int kMaxDamage = 26;

    while (formulaPulseTimer >= kPulseInterval && target->getState() != EnemyState::DEAD) {
        const float ramp = std::min(1.0f, focusTime / kRampTime);
        damage = static_cast<int>(kStartDamage + (kMaxDamage - kStartDamage) * ramp + 0.5f);

        if (events != nullptr) {
            TowerAttackEvent event;
            event.kind = TowerEffectKind::Class;
            event.origin = position;
            event.targets.push_back(targetCenter);
            event.range = range;
            events->push_back(std::move(event));
        }

        attack(*target);
        formulaPulseTimer -= kPulseInterval;
    }
}

void ClassTower::attack(Enemy& target) {
    const int dealt = effectiveDamage(damage);
    std::cout << "[Tower] Class formula beam -> " << dealt << " damage\n";
    target.takeDamage(dealt);
}

void ClassTower::draw() {}

// --- Bilibili: straight lane shot ---

BilibiliTower::BilibiliTower()
    : DefenseTower("Bilibili", 65, 305.0f * kTowerRangeScale, 15, 0.34f) {
    fireDirection = { 1.0f, 0.0f };
}

void BilibiliTower::setFireDirection(const Vector2D& dir) {
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 1e-4f) return;
    fireDirection.x = dir.x / len;
    fireDirection.y = dir.y / len;
}

bool BilibiliTower::purchaseDirectionChange(int& playerGold, int goldCost) {
    if (playerGold < goldCost) return false;
    playerGold -= goldCost;
    // 90 degrees counter-clockwise so upgrades cycle cleanly on a grid.
    setFireDirection({ -fireDirection.y, fireDirection.x });
    std::cout << "[Tower] Bilibili rotated lane to (" << fireDirection.x << ", "
        << fireDirection.y << ")\n";
    return true;
}

bool BilibiliTower::isEnemyOnAttackRay(const Enemy& enemy) const {
    Rect box = enemy.getBoundingBox();
    Vector2D ec = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };

    float vx = ec.x - position.x;
    float vy = ec.y - position.y;
    float t = vx * fireDirection.x + vy * fireDirection.y;
    if (t < 0.0f || t > range) return false;

    float px = fireDirection.x * t;
    float py = fireDirection.y * t;
    float lx = vx - px;
    float ly = vy - py;
    float perp = std::sqrt(lx * lx + ly * ly);
    return perp <= kLaneHalfWidth;
}

Enemy* BilibiliTower::pickNearestEnemyOnRay(const std::vector<Enemy*>& enemies) const {
    Enemy* best = nullptr;
    float bestT = 0.0f;

    for (Enemy* e : enemies) {
        if (e == nullptr) continue;
        if (e->getState() == EnemyState::DEAD) continue;
        if (!isEnemyOnAttackRay(*e)) continue;

        Rect box = e->getBoundingBox();
        Vector2D ec = { box.x + box.width / 2.0f, box.y + box.height / 2.0f };
        float vx = ec.x - position.x;
        float vy = ec.y - position.y;
        float t = vx * fireDirection.x + vy * fireDirection.y;

        if (best == nullptr || t < bestT) {
            best = e;
            bestT = t;
        }
    }
    return best;
}

void BilibiliTower::update(float deltaTime, const std::vector<Enemy*>& enemies,
    std::vector<TowerAttackEvent>* events) {
    if (!placed) return;

    cooldownTimer += deltaTime;
    if (!canAttackNow()) return;

    Enemy* target = pickNearestEnemyOnRay(enemies);
    if (target == nullptr) return;

    if (events != nullptr) {
        TowerAttackEvent event;
        event.kind = TowerEffectKind::Bilibili;
        event.origin = position;
        event.targets.push_back(enemyCenter(*target));
        event.direction = fireDirection;
        event.range = range;
        events->push_back(std::move(event));
    }
    attack(*target);
    cooldownTimer = 0.0f;
}

void BilibiliTower::attack(Enemy& target) {
    const int dealt = effectiveDamage(damage);
    std::cout << "[Tower] Bilibili straight barrage -> " << dealt << " damage\n";
    target.takeDamage(dealt);
}

void BilibiliTower::draw() {}

// --- AI tower: 360 sweep + upgrades ---

AITower::AITower()
    : AreaEffectTower("AI(Doubao)", 100, 205.0f * kTowerRangeScale, 15, 1.08f),
    level(1), maxLevel(3),
    upgradeCosts{ 80, 150 },
    levelNames{ "Doubao", "DeepSeek", "GPT" }
{}

int AITower::nextUpgradeCost() const {
    if (level >= maxLevel) return -1;
    return upgradeCosts[level - 1];
}

bool AITower::canUpgrade(int playerGold) const {
    if (level >= maxLevel) return false;
    return playerGold >= upgradeCosts[level - 1];
}

bool AITower::upgrade(int& playerGold) {
    if (!canUpgrade(playerGold)) return false;

    playerGold -= upgradeCosts[level - 1];
    level++;

    damage += 8;
    range += 26.0f * kTowerRangeScale;
    attackInterval *= 0.9f;

    towerName = std::string("AI(") + levelNames[level - 1] + ")";

    std::cout << "[Upgrade] AI tower -> Lv." << level
        << " (" << levelNames[level - 1] << ")"
        << "  per-target sweep dmg=" << damage
        << "  range=" << range
        << "  interval=" << attackInterval << "s\n";
    return true;
}

void AITower::restoreLevelForSave(int savedLevel) {
    if (savedLevel < 1) savedLevel = 1;
    if (savedLevel > maxLevel) savedLevel = maxLevel;
    while (level < savedLevel) {
        int freeGold = 1000000;
        upgrade(freeGold);
    }
}

void AITower::applyAreaEffect(const std::vector<Enemy*>& targets) {
    const int dealt = effectiveDamage(damage);
    std::cout << "[Tower] " << towerName << " 360 sweep nails "
        << targets.size() << " foe(s) for " << dealt << " each\n";

    for (Enemy* e : targets) {
        e->takeDamage(dealt);
    }
}

void AITower::attack(Enemy& target) {
    dealDamage(target, damage);
}

void AITower::draw() {}

