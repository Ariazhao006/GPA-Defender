///
/// GameUI.cpp - Implementation of text-based UI for testing
///
#include "gpa_defender/GameUI.h"
#include <cstdlib>
#include <sstream>
#include <algorithm>

GameUI::GameUI(GameEngine& gameEngine) : engine(gameEngine) {}

void GameUI::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void GameUI::printSeparator(char ch) {
    for (int i = 0; i < screenWidth; ++i) {
        std::cout << ch;
    }
    std::cout << "\n";
}

void GameUI::printBox(const std::string& title, const std::string& content) {
    std::cout << "  [" << title << "]\n";
    std::cout << "  " << content << "\n";
}

std::string GameUI::statBar(int current, int threshold, int width) {
    float ratio = (threshold > 0) ? static_cast<float>(current) / static_cast<float>(threshold) : 1.0f;
    ratio = std::max(0.0f, std::min(1.0f, ratio));
    int filled = static_cast<int>(ratio * width);
    
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        bar += (i < filled) ? "=" : "-";
    }
    bar += "] " + std::to_string(current) + "/" + std::to_string(threshold);
    
    return bar;
}

void GameUI::displayMainMenu() {
    clearScreen();
    printSeparator('=');
    std::cout << "              GPA Defender - Backend UI Test\n";
    printSeparator('=');
    std::cout << "\n  Game Phase: " << GameEngine::phaseName(engine.getPhase()) << "\n";
    std::cout << "  Gold: " << engine.getGold() << "\n";
    std::cout << "  Wave: " << (engine.getWaveIndex() + 1) << "\n\n";
    
    const PlayerStats& player = engine.getPlayerStats();
    std::cout << "  Player Stats:\n";
    std::cout << "    Academic:   " << statBar(player.getCurrentAcademic(), 
                                               player.getThresholdAcademic()) << "\n";
    std::cout << "    Physical:   " << statBar(player.getCurrentPhysical(),
                                               player.getThresholdPhysical()) << "\n";
    std::cout << "    Mental:     " << statBar(player.getCurrentMental(),
                                               player.getThresholdMental()) << "\n";
    std::cout << "    Connection: " << statBar(player.getCurrentConnection(),
                                               player.getThresholdConnection()) << "\n\n";
    
    const GameSnapshot snapshot = engine.getSnapshot();
    std::cout << "  Wave Status:\n";
    std::cout << "    Enemies: " << snapshot.activeEnemies << "/" << snapshot.totalWaveSpawns << "\n";
    std::cout << "    Time: " << std::fixed << std::setprecision(2) 
              << snapshot.waveTimeSec << " sec\n";
    std::cout << "    Exercise Mode: " << (snapshot.exerciseMode ? "ON" : "OFF") << "\n";
}

void GameUI::displayPlayerStats() {
    const PlayerStats& player = engine.getPlayerStats();
    std::cout << "\n=== PLAYER STATS ===\n";
    std::cout << "Academic   (Threshold: " << player.getThresholdAcademic() << "): "
              << statBar(player.getCurrentAcademic(), player.getThresholdAcademic(), 30) << "\n";
    std::cout << "Physical   (Threshold: " << player.getThresholdPhysical() << "): "
              << statBar(player.getCurrentPhysical(), player.getThresholdPhysical(), 30) << "\n";
    std::cout << "Mental     (Threshold: " << player.getThresholdMental() << "): "
              << statBar(player.getCurrentMental(), player.getThresholdMental(), 30) << "\n";
    std::cout << "Connection (Threshold: " << player.getThresholdConnection() << "): "
              << statBar(player.getCurrentConnection(), player.getThresholdConnection(), 30) << "\n";
    
    std::cout << "\nAlive: " << (player.isAlive() ? "YES" : "NO") << "\n";
    std::cout << "Exercise Mode: " << (player.getExerciseMode() ? "ON" : "OFF") << "\n";
}

void GameUI::displayGamePhaseInfo() {
    std::cout << "\n=== GAME PHASE ===\n";
    std::cout << "Current Phase: " << GameEngine::phaseName(engine.getPhase()) << "\n\n";
    std::cout << "Phase Descriptions:\n";
    std::cout << "  PreGame: Initialization state\n";
    std::cout << "  Build: Building towers\n";
    std::cout << "  WaveRunning: Enemies spawning and moving\n";
    std::cout << "  WaveCleared: Wave completed, ready for next\n";
    std::cout << "  Victory: All waves cleared, player alive\n";
    std::cout << "  GameOver: Player died or lost condition\n";
}

void GameUI::displayWaveInfo() {
    const GameSnapshot snapshot = engine.getSnapshot();
    std::cout << "\n=== WAVE INFO ===\n";
    std::cout << "Current Wave: " << (engine.getWaveIndex() + 1) << "\n";
    std::cout << "Active Enemies: " << snapshot.activeEnemies << "\n";
    std::cout << "Total Spawned: " << snapshot.spawnedEnemies << "\n";
    std::cout << "Wave Size: " << snapshot.totalWaveSpawns << "\n";
    std::cout << "Elapsed Time: " << std::fixed << std::setprecision(2) 
              << snapshot.waveTimeSec << " seconds\n";
    
    if (snapshot.activeEnemies == 0 && snapshot.totalWaveSpawns > 0) {
        std::cout << "\n[+] All enemies eliminated!\n";
    }
}

void GameUI::displayTowerInfo() {
    const auto& towers = engine.getTowers();
    std::cout << "\n=== TOWERS ===\n";
    std::cout << "Total Towers: " << towers.size() << "\n\n";
    
    for (size_t i = 0; i < towers.size(); ++i) {
        if (towers[i]) {
            std::cout << "  Tower " << (i + 1) << ": " << towers[i]->getName()
                      << " @ (" << towers[i]->getPosition().x << ", " 
                      << towers[i]->getPosition().y << ")\n";
            std::cout << "    Cost: " << towers[i]->getCost() 
                      << ", Damage: " << towers[i]->getDamage()
                      << ", Range: " << towers[i]->getRange() << "\n";
        }
    }
}

void GameUI::displayEnemyInfo() {
    const WaveManager& wm = engine.getWaveManager();
    std::cout << "\n=== ENEMIES ===\n";
    std::cout << "Active Count: " << wm.getActiveEnemyCount() << "\n";
    std::cout << "Total Spawned: " << wm.getSpawnedCount() << "\n";
    std::cout << "Total in Wave: " << wm.getTotalSpawnCount() << "\n";
}

void GameUI::displayGoldInfo() {
    std::cout << "\n=== GOLD ===\n";
    std::cout << "Current Gold: " << engine.getGold() << "\n";
}

void GameUI::displayGameOver() {
    clearScreen();
    printSeparator('*');
    std::cout << "\n              GAME OVER - YOU DIED!\n\n";
    const PlayerStats& player = engine.getPlayerStats();
    std::cout << "  Final Stats:\n";
    std::cout << "    Academic: " << player.getCurrentAcademic() 
              << " (threshold: " << player.getThresholdAcademic() << ")\n";
    std::cout << "    Physical: " << player.getCurrentPhysical()
              << " (threshold: " << player.getThresholdPhysical() << ")\n";
    std::cout << "    Mental: " << player.getCurrentMental()
              << " (threshold: " << player.getThresholdMental() << ")\n";
    std::cout << "    Connection: " << player.getCurrentConnection()
              << " (threshold: " << player.getThresholdConnection() << ")\n\n";
    printSeparator('*');
}

void GameUI::displayVictory() {
    clearScreen();
    printSeparator('*');
    std::cout << "\n              VICTORY - YOU SURVIVED!\n\n";
    std::cout << "  Congratulations! You completed all waves!\n";
    std::cout << "  Final Gold: " << engine.getGold() << "\n";
    const PlayerStats& player = engine.getPlayerStats();
    std::cout << "  Final Stats:\n";
    std::cout << "    Academic: " << player.getCurrentAcademic() << "\n";
    std::cout << "    Physical: " << player.getCurrentPhysical() << "\n";
    std::cout << "    Mental: " << player.getCurrentMental() << "\n";
    std::cout << "    Connection: " << player.getCurrentConnection() << "\n\n";
    printSeparator('*');
}

void GameUI::runBuildPhaseUI() {
    std::cout << "\n=== BUILD PHASE ===\n";
    std::cout << "Current Gold: " << engine.getGold() << "\n";
    std::cout << "(In a full UI, you would select towers and positions here)\n";
    std::cout << "\nPress Enter to proceed...\n";
    std::cin.ignore();
}

void GameUI::runWavePhaseUI() {
    std::cout << "\n=== WAVE RUNNING ===\n";
    std::cout << "Simulating wave...\n";
    
    const int deltaTime = 16; // ~60 FPS (1/60 ≈ 16ms)
    int frames = 0;
    
    while (engine.getPhase() == GamePhase::WaveRunning && frames < 1000) {
        engine.update(deltaTime / 1000.0f);
        ++frames;
        
        if (frames % 60 == 0) {
            const GameSnapshot snap = engine.getSnapshot();
            std::cout << "  [t=" << std::fixed << std::setprecision(1) 
                      << (frames * deltaTime / 1000.0f) << "s] "
                      << "Enemies: " << snap.activeEnemies << "/" << snap.totalWaveSpawns
                      << ", Gold: " << snap.gold << "\n";
        }
    }
}

void GameUI::runFullGameUI() {
    displayMainMenu();
    
    std::cout << "\n  Commands:\n";
    std::cout << "    (1) View Player Stats\n";
    std::cout << "    (2) View Wave Info\n";
    std::cout << "    (3) View Tower Info\n";
    std::cout << "    (4) View Enemy Info\n";
    std::cout << "    (5) View Gold Info\n";
    std::cout << "    (6) View Phase Info\n";
    std::cout << "    (7) Start Next Wave\n";
    std::cout << "    (8) Run Wave Simulation\n";
    std::cout << "    (0) Exit\n\n";
    std::cout << "  Enter command: ";
    
    char cmd;
    while (std::cin >> cmd) {
        switch (cmd) {
        case '1':
            displayPlayerStats();
            break;
        case '2':
            displayWaveInfo();
            break;
        case '3':
            displayTowerInfo();
            break;
        case '4':
            displayEnemyInfo();
            break;
        case '5':
            displayGoldInfo();
            break;
        case '6':
            displayGamePhaseInfo();
            break;
        case '7':
            if (engine.getPhase() == GamePhase::Build || engine.getPhase() == GamePhase::WaveCleared) {
                std::cout << "\nStarting wave " << (engine.getWaveIndex() + 2) << "...\n";
                if (engine.startWave()) {
                    runWavePhaseUI();
                    displayWaveInfo();
                }
            }
            break;
        case '8':
            if (engine.getPhase() == GamePhase::WaveRunning) {
                runWavePhaseUI();
            }
            break;
        case '0':
            std::cout << "\nExiting...\n";
            return;
        default:
            std::cout << "Unknown command.\n";
        }
        
        if (engine.getPhase() == GamePhase::GameOver) {
            displayGameOver();
            return;
        }
        if (engine.getPhase() == GamePhase::Victory) {
            displayVictory();
            return;
        }
        
        std::cout << "\n  Enter command: ";
    }
}

