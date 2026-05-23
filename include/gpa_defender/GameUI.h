///
/// GameUI.h - Simple text-based UI for testing and demonstration
///
#pragma once
#ifndef GAME_UI_H
#define GAME_UI_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "gpa_defender/GameEngine.h"

class GameUI {
private:
    GameEngine& engine;
    int screenWidth = 80;
    int screenHeight = 24;

    void clearScreen();
    void printSeparator(char ch = '=');
    void printBox(const std::string& title, const std::string& content);
    std::string statBar(int current, int threshold, int width = 20);
    
public:
    explicit GameUI(GameEngine& gameEngine);

    // Main display functions
    void displayMainMenu();
    void displayGameStatus();
    void displayPlayerStats();
    void displayGamePhaseInfo();
    void displayWaveInfo();
    void displayTowerInfo();
    void displayEnemyInfo();
    void displayGoldInfo();
    void displayGameOver();
    void displayVictory();
    
    // Interactive functions
    void runBuildPhaseUI();
    void runWavePhaseUI();
    void runPhaseTransitionUI();
    
    // Full game loop UI
    void runFullGameUI();
};

#endif // GAME_UI_H

