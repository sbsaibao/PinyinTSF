#pragma once

#ifndef KEY_EVENT_LOGIC_H
#define KEY_EVENT_LOGIC_H

#include <windows.h>

namespace KeyEventLogic {
    bool ShouldEatKeyDown(
        WPARAM key,
        bool hasComposition,
        bool hasCandidates,
        int candidateCount,
        bool ctrlDown,
        bool altDown,
        bool winDown);
}

#endif // KEY_EVENT_LOGIC_H
