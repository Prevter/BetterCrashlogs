#pragma once

#include "imgui.h"

namespace ui {
    /// @brief The main font used in the UI
    extern ImFont* mainFont;
    /// @brief The title font used in the UI
    extern ImFont* titleFont;

    /// @brief Applies the custom styles to the ImGui context
    void applyStyles();

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
        "Try turning it off and on again.", "Boo!", "Whoopsies! An unhandled exception has occurred."
    };

    /// @brief The number of inspirational messages
    const size_t RANDOM_MESSAGES_COUNT = sizeof(RANDOM_MESSAGES) / sizeof(RANDOM_MESSAGES[0]);

    /// @brief Picks a random quote from the list of inspirational messages and stores and always returns the same quote
    const char* pickRandomQuote();

    /// @brief Contains the information about the exception that occurred
    void informationWindow();

    /// @brief Contains the metadata about the game and mod loader
    void metadataWindow();

    /// @brief Contains the stack trace of the exception
    void stackTraceWindow();

    /// @brief Contains the registers of the exception
    void registersWindow();

    /// @brief Contains a part of the stack found in the memory
    void stackWindow();

    /// @brief Contains the list of installed/loaded mods
    void modsWindow();

}