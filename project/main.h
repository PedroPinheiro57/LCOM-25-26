/**
 * @mainpage Battleship Project (LCOM)
 *
 * @section intro_sec Introduction
 * This project implements the classic Battleship game, developed within the scope of the 
 * Computer Laboratory (LCOM) course. The system uses serial communication (UART) to 
 * enable real-time multiplayer mode between two computers and includes a "Hunt and Target" 
 * artificial intelligence algorithm for the Single Player mode.
 *
 * @section arch_sec Architecture
 * The project follows a State Machine-based architecture, divided into three layers:
 * - @b Model: Data structures, board logic, and real-time clock (RTC) management.
 * - @b View: Graphical rendering (XPMs), menu management, and font handling.
 * - @b Controller: Event handling (Keyboard, Mouse, Serial) and game/Bot logic.
 *
 * @section feature_sec Key Features
 * - **Single Player Mode**: Battle against an AI that learns from its hits.
 * - **Multiplayer Mode**: Real-time synchronization between two clients via serial port.
 * - **Communication Protocol**: Robust message definition for exchanging game states and actions.
 * - **Input Management**: Full control via Keyboard and Mouse with conflict resolution.
 *
 * @author Pedro Pinheiro (up202405055)
 * @author Jorge Cunha (up202405044)
 * @date June 2026
 */


/**
 * @file main.h
 * @brief Top-level declarations shared across the application entry point.
 *
 * The application is launched as either the @e host or the @e client VM.
 * The role is parsed from the command-line arguments in @c main() and
 * stored in a module-level variable that this header exposes.
 */

#pragma once

/**
 * @brief Returns whether this instance is running as the client (non-host) role.
 *
 * The role is determined at startup from the command-line arguments.
 * Use this anywhere a quick role check is needed without pulling in
 * the full game state.
 *
 * @return @c true  if this VM was launched as the client,
 *         @c false if it is the host.
 */
bool role_is_client(void);
