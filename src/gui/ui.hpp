#pragma once

#include "imgui.h"
#include "../analyzer/analyzer.hpp"

namespace ui {
    /// @brief The main font used in the UI
    ImFont*& getMainFont();

    /// @brief The title font used in the UI
    ImFont*& getTitleFont();

    /// @brief Applies the custom styles to the ImGui context
    void applyStyles();

    /// @brief Resizes the UI based on the scale
    void resize();

    /// @brief Inspirational messages that are displayed when the game crashes
    const char *const RANDOM_MESSAGES[] = {
        "Everything's going to plan. No, really, that was supposed to happen.",
        "Uh... Did I do that?", "Oops.", "Why did you do that?", "I feel sad now :(",
        "My bad.", "Time to uninstall some mods!", "I let you down. Sorry :(",
        "On the bright side, I bought you a teddy bear!", "Your Mega Hack crashed! (Just kidding)",
        "Oh - I know what I did wrong!", "Hey, that tickles! Hehehe!",
        "I blame RobTop", "Don't be sad. I'll do better next time, I promise!",
        "Don't be sad, have a hug! <3", "I just don't know what went wrong :(",
        "Shall we play a game?", "I bet Cvolton wouldn't have this problem.",
        "Quite honestly, I wouldn't worry myself about that.", "Sorry :(",
        "Surprise! Haha. Well, this is awkward.", "Would you like a cupcake?",
        "This doesn't make any sense!", "Why is it breaking :(",
        "Don't do that.", "Ouch. That hurt :(", "You're mean.",
        "But it works on my machine.", "Hi Absolute!", "The chicken is on fire!",
        "I've accidentally crashed the game! /RubRub", "It's a feature, not a bug!",
        "Try turning it off and on again.", "Boo!", "Whoopsies! An unhandled exception has occurred.",
        "Geometry Dash isn't Geometry Dashing anymore", "Now in 64 bits!",
        "$ sudo rm -rf /", "To the mod developer: Please 309 yourself",
        "Let's pretend this didn't happen.", "I hope it's not GJRobotSprite::init this time",
        "Skill issue", "Out of cheese error.", "Error 404: Fun not found.",
        "A wild bug appeared!", "Congratulations! You found a bug.",
        "Don't panic! It's just a minor setback.", "Cosmic rays caused a bit flip, sorry!",
        "I'm not crying, you're crying.", "Insert witty error message here.",
        "Mission failed successfully.", "Game over. Insert coin to continue.",
        "You shall not pass! Oh wait, you did...", "The cake is a lie!",
        "Quick, look over there! *runs away*", "To be continued...",
        "In case of fire, use water.", "Did you try blowing on the cartridge?",
        "You found an easter egg! Just kidding, it's a crash."
    };

    /// @brief The number of inspirational messages
    const size_t RANDOM_MESSAGES_COUNT = sizeof(RANDOM_MESSAGES) / sizeof(RANDOM_MESSAGES[0]);

    /// @brief Picks a random quote from the list of inspirational messages and stores and always returns the same quote
    const char* pickRandomQuote();

    /// @brief Picks a new random quote
    void newQuote();

    /// @brief Contains the information about the exception that occurred
    void informationWindow(analyzer::Analyzer& analyzer);

    /// @brief Contains the metadata about the game and mod loader
    void metadataWindow();

    /// @brief Contains the stack trace of the exception
    void stackTraceWindow(analyzer::Analyzer& analyzer);

    /// @brief Contains the registers of the exception
    void registersWindow(analyzer::Analyzer& analyzer);

    /// @brief Contains a part of the stack found in the memory
    void stackWindow(analyzer::Analyzer& analyzer);

    /// @brief Contains the list of installed/loaded mods
    void modsWindow();

    /// @brief Contains disassembled code around the exception
    void disassemblyWindow(analyzer::Analyzer& analyzer);

    /// @brief Create a toast message that will be displayed on the screen for a short period of time
    void showToast(const std::string& message, float duration = 3.f);

    /// @brief Render the toast messages
    void renderToasts();

}