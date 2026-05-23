#include "gpa_defender/Enemy.h"
#include <iostream>

Enemy::Enemy(std::string name, int hp, float spd, int gold)
    : name(name), maxHp(hp), currentHp(hp), speed(spd), dropGold(gold),
    state(EnemyState::MOVING), currentWaypointIndex(0),
    slowMultiplier(1.0f), slowTimeLeft(0.0f) {
    boundingBox.width = 40.0f;
    boundingBox.height = 40.0f;
}

void Enemy::setPath(const std::vector<Vector2D>& path) {
    waypoints = path;
    if (!waypoints.empty()) {
        position = waypoints[0];
        boundingBox.x = position.x - boundingBox.width / 2.0f;
        boundingBox.y = position.y - boundingBox.height / 2.0f;
    }
}

void Enemy::update(float deltaTime, PlayerStats* player) {
    if (state == EnemyState::DEAD) return;

    if (state == EnemyState::MOVING) {
        if (waypoints.empty() || currentWaypointIndex >= waypoints.size()) return;

        slowTimeLeft -= deltaTime;
        if (slowTimeLeft <= 0.0f) {
            slowTimeLeft = 0.0f;
            slowMultiplier = 1.0f;
        }
        const float moveSpeed = speed * slowMultiplier;

        Vector2D target = waypoints[currentWaypointIndex];
        float dist = position.distanceTo(target);

        if (dist <= moveSpeed * deltaTime) {
            position = target;
            currentWaypointIndex++;

            if (currentWaypointIndex >= waypoints.size()) {
                // ע�����ĩβ����Ӣ�Ŀո��̾�ţ���ֹ C2001 ����
                std::cout << std::endl << "[����] " << name << " �����С�������! " << std::endl;

                if (player != nullptr) {
                    if (dmgAcademic > 0) {
                        player->changeAcademic(-dmgAcademic);
                        std::cout << "  -> ѧҵ�ɼ��ܴ�: -" << dmgAcademic << std::endl;
                    }
                    if (dmgMental > 0) {
                        player->changeMental(-dmgMental);
                        std::cout << "  -> ���������ܻ�: -" << dmgMental << std::endl;
                    }
                    if (dmgConnection > 0) {
                        player->changeConnection(-dmgConnection);
                        std::cout << "  -> ������½�: -" << dmgConnection << std::endl;
                    }
                    if (dmgPhysical > 0) {
                        player->changePhysical(-dmgPhysical);
                        std::cout << "  -> ���彡��͸֧: -" << dmgPhysical << std::endl;
                    }
                }
                state = EnemyState::DEAD;
            }
        }
        else {
            Vector2D dir = { target.x - position.x, target.y - position.y };
            dir = dir.normalize();
            position.x += dir.x * moveSpeed * deltaTime;
            position.y += dir.y * moveSpeed * deltaTime;
        }

        boundingBox.x = position.x - boundingBox.width / 2.0f;
        boundingBox.y = position.y - boundingBox.height / 2.0f;
    }
}

int Enemy::takeDamage(int damage) {
    if (state == EnemyState::DEAD) return 0;

    currentHp -= damage;
    if (currentHp <= 0) {
        currentHp = 0;
        state = EnemyState::DEAD;
        // ע�����ͬ������Ӣ�ı��Ϳո�
        std::cout << "[ս��] �ɹ����� " << name << " ! ��û��� +" << dropGold << std::endl;
        return dropGold;
    }
    return 0;
}

void Enemy::applySlowEffect(float speedMultiplier, float durationSeconds) {
    if (state == EnemyState::DEAD) return;
    slowMultiplier = speedMultiplier;
    slowTimeLeft = durationSeconds;
}

float Enemy::getEffectiveMoveSpeed() const {
    if (state != EnemyState::MOVING) return 0.0f;
    return speed * slowMultiplier;
}

SubjectEnemy::SubjectEnemy() : Enemy("��΢����I��", 120, 100.0f, 20) {
    dmgAcademic = 25;
}
void SubjectEnemy::draw() {}

ResearchEnemy::ResearchEnemy() : Enemy("������ĩ����ҵ", 450, 40.0f, 50) {
    dmgAcademic = 15;
    dmgMental = 20;
}
void ResearchEnemy::draw() {}

SocialEnemy::SocialEnemy() : Enemy("���Ѷ�û������", 80, 160.0f, 15) {
    dmgConnection = 30;
    dmgMental = 10;
}
void SocialEnemy::draw() {}
// --- ���׻����������ʵ�� ---

// ����ˡ������Ӽ���Ч��
MorningClassEnemy::MorningClassEnemy() : Enemy("���ϰ˵�����.", 90, 180.0f, 15) {
    dmgPhysical = 20;
    dmgMental = 15;
}
void MorningClassEnemy::applySlowEffect(float speedMultiplier, float durationSeconds) {
    // ���ǻ��෽����ʲô��������ʵ��ħ�⣨���߼��٣�
}
void MorningClassEnemy::draw() {}

// �����п��� Boss�������׶ο񱩻���
MidtermBossEnemy::MidtermBossEnemy() : Enemy("�����п��Դ�ħ����.", 800, 50.0f, 150) {
    dmgAcademic = 50;
    dmgMental = 30;
    isEnraged = false;
}
int MidtermBossEnemy::takeDamage(int damage) {
    if (state == EnemyState::DEAD) return 0;

    currentHp -= damage;

    // ��̬״̬�л���Ѫ������һ����δ��ʱ���������׶�
    if (currentHp > 0 && currentHp <= maxHp / 2 && !isEnraged) {
        isEnraged = true;
        speed *= 2.5f;           // ���ٱ���2.5��
        slowMultiplier = 1.0f;   // ˲���أ�������м���
        slowTimeLeft = 0.0f;
        // ���ܣ�ĩβ��Ӣ�ľ��
        std::cout << "[Boss ����] ���п��Խ����ˡ���̸�ϰ�׶Ρ�! ���ٱ���! ." << std::endl;
    }

    if (currentHp <= 0) {
        currentHp = 0;
        state = EnemyState::DEAD;
        // ���ܣ�ð�Ż���
        std::cout << "[ս��] �漣��ع�����: " << name << " ! ��ý�ѧ��: +" << dropGold << std::endl;
        return dropGold;
    }
    return 0;
}
void MidtermBossEnemy::update(float deltaTime, PlayerStats* player) {
    // ���û�����ƶ��߼�
    Enemy::update(deltaTime, player);
}
void MidtermBossEnemy::draw() {}

// ��С����ҵ�������׼��˻���
GroupProjectEnemy::GroupProjectEnemy() : Enemy("С����ҵ����������.", 300, 60.0f, 80) {
    dmgConnection = 40;
    dmgMental = 40;
    armor = 8;          // ÿ�α�����ǿ�ƿۼ� 8 ���˺�
}
int GroupProjectEnemy::takeDamage(int damage) {
    if (state == EnemyState::DEAD) return 0;

    // ���׼����㷨��������� 1 ���˺�����ֹ��ȫ���Ʒ�
    int finalDamage = damage - armor;
    if (finalDamage < 1) finalDamage = 1;

    currentHp -= finalDamage;

    if (currentHp <= 0) {
        currentHp = 0;
        state = EnemyState::DEAD;
        // ���ܣ�ð�Ż��壬���׽�� 157 �е� C2001 ����
        std::cout << "[ս��] �ɹ�������: " << name << " ! ��û���: +" << dropGold << std::endl;
        return dropGold;
    }
    return 0;
}
void GroupProjectEnemy::draw() {}
// ================= ����׷�ӵ� Enemy.cpp ����ĩβ =================
#include <cstdlib> // ���� rand() ����

// ������Ƶ�ڶ�������������
ShortVideoEnemy::ShortVideoEnemy() : Enemy("ˢ����Ķ���Ƶ�ڶ�.", 250, 70.0f, 30) {
    dmgAcademic = 15;
    dmgPhysical = 20;   // ��ҹˢ��Ƶ������
    regenTimer = 0.0f;
}
void ShortVideoEnemy::update(float deltaTime, PlayerStats* player) {
    if (state != EnemyState::DEAD) {
        regenTimer += deltaTime;
        // ÿ�� 1 �룬�ָ� 15 ������ֵ
        if (regenTimer >= 1.0f) {
            currentHp += 15;
            if (currentHp > maxHp) currentHp = maxHp;
            regenTimer = 0.0f;
        }
    }
    // ���û����߼������ƶ�
    Enemy::update(deltaTime, player);
}
void ShortVideoEnemy::draw() {}

// ��Ѧ���̵Ŀ��١������ܻ���
ExamSyllabusEnemy::ExamSyllabusEnemy() : Enemy("Ѧ���̵���ĩ����.", 150, 90.0f, 40) {
    dmgAcademic = 35; // û��ϰ��ֱ�ӱ�ը
    dmgMental = 20;
    dodgeChance = 30; // 30% ��������
}
int ExamSyllabusEnemy::takeDamage(int damage) {
    if (state == EnemyState::DEAD) return 0;

    // �����ж�
    if (std::rand() % 100 < dodgeChance) {
        std::cout << "[����] " << name << " ����������! �ɹ��ܿ��˸�ϰ��Χ! ." << std::endl;
        return 0; // �ܵ� 0 �˺�
    }

    // û��������������Ѫ�߼�
    return Enemy::takeDamage(damage);
}
void ExamSyllabusEnemy::draw() {}

// ��ͬ��ѹ���������Ǽ��ٻ���
PeerPressureEnemy::PeerPressureEnemy() : Enemy("�޴����ڵ�ͬ��ѹ��(�ھ�).", 200, 50.0f, 50) {
    dmgMental = 50; // ���ȴ����������
    dmgConnection = 20; // �ƻ�ͬѧ��ϵ
    timeAlive = 0.0f;
}
void PeerPressureEnemy::update(float deltaTime, PlayerStats* player) {
    if (state != EnemyState::DEAD) {
        timeAlive += deltaTime;
        // ���Ļ��ƣ��������� 50��ÿ��� 1 �룬�������� 8 �㣨Խ��Խ�죩
        speed = 50.0f + (timeAlive * 8.0f);
    }
    // ���û����߼������ƶ�
    Enemy::update(deltaTime, player);
}
void PeerPressureEnemy::draw() {}
