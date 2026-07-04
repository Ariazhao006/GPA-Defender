#pragma once
#ifndef ENEMY_H
#define ENEMY_H

#include <string>
#include <vector>

#include "gpa_defender/PlayerStats.h"
#include "gpa_defender/Vector2D.h"

enum class EnemyState {
    MOVING,
    DEAD
};

class Enemy {
protected:
    std::string name;
    int maxHp;
    int currentHp;
    float speed;
    int dropGold;

    int dmgAcademic = 0;
    int dmgMental = 0;
    int dmgConnection = 0;
    int dmgPhysical = 0;

    EnemyState state;
    bool reachedBase = false;
    Vector2D position;
    Rect boundingBox;

    std::vector<Vector2D> waypoints;
    int currentWaypointIndex;

    float slowMultiplier;
    float slowTimeLeft;

public:
    Enemy(std::string name, int hp, float spd, int gold);
    virtual ~Enemy() = default;

    void setPath(const std::vector<Vector2D>& path);
    void setPosition(const Vector2D& pos);

    virtual void update(float deltaTime, PlayerStats* player);
    virtual int takeDamage(int damage);
    virtual void applySlowEffect(float speedMultiplier, float durationSeconds);
    virtual void draw() = 0;

    EnemyState getState() const { return state; }
    Rect getBoundingBox() const { return boundingBox; }
    int getHp() const { return currentHp; }
    int getMaxHp() const { return maxHp; }
    const std::string& getName() const { return name; }
    Vector2D getPosition() const { return position; }
    float getEffectiveMoveSpeed() const;
    bool hasReachedBase() const { return reachedBase; }
};

// Subject: basic learning burden
class SubjectEnemy : public Enemy {
public:
    SubjectEnemy();
    void draw() override;
};

// Research: research assignments
class ResearchEnemy : public Enemy {
public:
    ResearchEnemy();
    void draw() override;
};

// Social: social issues
class SocialEnemy : public Enemy {
public:
    SocialEnemy();
    void draw() override;
};

// MorningClass: early morning lectures, resists slow.
class MorningClassEnemy : public Enemy {
public:
    MorningClassEnemy();
    void applySlowEffect(float speedMultiplier, float durationSeconds) override;
    void draw() override;
};

// MidtermBoss: dual-phase boss.
class MidtermBossEnemy : public Enemy {
private:
    bool isEnraged;

public:
    MidtermBossEnemy();
    int takeDamage(int damage) override;
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

// GroupProject: armor-based damage reduction.
class GroupProjectEnemy : public Enemy {
private:
    int armor;

public:
    GroupProjectEnemy();
    int takeDamage(int damage) override;
    void draw() override;
};

// ShortVideo: regenerates health over time.
class ShortVideoEnemy : public Enemy {
private:
    float regenTimer;

public:
    ShortVideoEnemy();
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

// ExamSyllabus: dodge chance.
class ExamSyllabusEnemy : public Enemy {
private:
    int dodgeChance;

public:
    ExamSyllabusEnemy();
    int takeDamage(int damage) override;
    void draw() override;
};

// PeerPressure: gets stronger over time.
class PeerPressureEnemy : public Enemy {
private:
    float timeAlive;

public:
    PeerPressureEnemy();
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

class TreasureChestEnemy : public Enemy {
public:
    TreasureChestEnemy(std::string displayName, int hp, int gold);
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

#endif // ENEMY_H
