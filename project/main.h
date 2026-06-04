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
