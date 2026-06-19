#pragma once
// ========================================
// File: Enemy.h
// ========================================
#pragma once
#ifndef ENEMY_H
#define ENEMY_H

#include <vector>
#include <string>
#include "gpa_defender/Vector2D.h"
#include "gpa_defender/PlayerStats.h" // ����С���ָ����

// �з�״̬����ȥ����Arknights��ATTACKING�赲״̬����Ϊ��������ת
enum class EnemyState {
    MOVING,     // �о�״̬�����ڵ�Ѱ·��
    DEAD        // ����״̬�������ܻ��ѵ����յ�۳�ָ�꣩
};

/**
 * @class Enemy
 * @brief �з���Ӫ��������
 */
class Enemy {
protected:
    std::string name;       // �������� (�磺΢���֡����д���ҵ)
    int maxHp;              // �������ֵ
    int currentHp;          // ��ǰ����ֵ
    float speed;            // �ƶ��ٶ�
    int dropGold;           // ���ܺ��õĽ��

    // ��С�����ָ����ɵ��˺�ֵ
    int dmgAcademic = 0;
    int dmgMental = 0;
    int dmgConnection = 0;
    int dmgPhysical = 0;

    EnemyState state;       // ��ǰ״̬
    Vector2D position;      // ��ǰ��������
    Rect boundingBox;       // ������ײ��

    std::vector<Vector2D> waypoints;
    int currentWaypointIndex;

    float slowMultiplier;   // ͼ��ݼ��ٱ���
    float slowTimeLeft;

public:
    Enemy(std::string name, int hp, float spd, int gold);
    virtual ~Enemy() = default;

    void setPath(const std::vector<Vector2D>& path);

    // �����߼������� PlayerStats ʹ�ù��ﵽ���յ�ʱ����ֱ�ӿ۳�ָ��
    virtual void update(float deltaTime, PlayerStats* player);

    // �ܵ��������˺������ػ�ɱ����Ľ�ң�û���򷵻�0��
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
};

// --- ������ɫ������ ---

// 1. ѧ�ƹ��ޣ���΢���֡��ߴ�... -> ר��ѧҵ�ɼ�
class SubjectEnemy : public Enemy {
public:
    SubjectEnemy();
    void draw() override;
};

// 2. ���д���ҵ���� -> ��������������ѧҵ�ɼ�
class ResearchEnemy : public Enemy {
public:
    ResearchEnemy();
    void draw() override;
};

// 3. �罻Issue���� (���Ѷ���æ/��ѹ��) -> ��������С���������
class SocialEnemy : public Enemy {
public:
    SocialEnemy();
    void draw() override;
};

#endif // ENEMY_H
// 4. ��˹��ޣ����ټ��죬�ҡ����߼��١�������ͼ��ݣ�
class MorningClassEnemy : public Enemy {
public:
    MorningClassEnemy();
    // ��д���ٺ�����ʹ����Ч��
    void applySlowEffect(float speedMultiplier, float durationSeconds);
    void draw() override;
};

// 5. ���п��� Boss��˫�׶�״̬����Ѫ������50%ʱ�������񱩳�̡�
class MidtermBossEnemy : public Enemy {
private:
    bool isEnraged; // �Ƿ��ڿ񱩣����׶Σ�״̬
public:
    MidtermBossEnemy();
    // ��д�ܵ��˺����߼������ڼ��Ѫ�����л�״̬
    int takeDamage(int damage) override;
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

// 6. С����ҵ��������ѣ����Դ���Ӳ��װ�ס����̶����ˣ���ר����������
class GroupProjectEnemy : public Enemy {
private:
    int armor; // ÿ���ܻ��̶�������˺�ֵ
public:
    GroupProjectEnemy();
    // ��д�ܵ��˺����߼������뻤�׼���
    int takeDamage(int damage) override;
    void draw() override;
};
// ================= ����׷�ӵ� Enemy.h ����ĩβ =================

// 7. ����Ƶ�ڶ���ӵ�С��������ϡ����ơ�������������ͻ᲻�ϻ�Ѫ
class ShortVideoEnemy : public Enemy {
private:
    float regenTimer;
public:
    ShortVideoEnemy();
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};

// 8. Ѧ���̵Ŀ��٣�ӵ�С����ܡ����ơ��и�����ȫ���ӱ����˺�
class ExamSyllabusEnemy : public Enemy {
private:
    int dodgeChance; // ���ܸ��ʰٷֱ� (0-100)
public:
    ExamSyllabusEnemy();
    int takeDamage(int damage) override;
    void draw() override;
};

// 9. ͬ��ѹ��/�ھ��籩��ӵ�С����Ǽ��١����ơ����ʱ��Խ�ã��ܵ�Խ��
class PeerPressureEnemy : public Enemy {
private:
    float timeAlive;
public:
    PeerPressureEnemy();
    void update(float deltaTime, PlayerStats* player) override;
    void draw() override;
};
