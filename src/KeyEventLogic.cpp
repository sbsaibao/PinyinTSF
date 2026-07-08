#include "KeyEventLogic.h"

namespace KeyEventLogic {

bool ShouldEatKeyDown(
    WPARAM key,
    bool hasComposition,
    bool hasCandidates,
    int candidateCount,
    bool ctrlDown,
    bool altDown,
    bool winDown) {
    if (ctrlDown || altDown || winDown) {
        return false;
    }

    if (key >= 'A' && key <= 'Z') {
        return true;
    }

    if (!hasComposition) {
        return false;
    }

    if (key >= '1' && key <= '5' && hasCandidates) {
        int index = static_cast<int>(key - '1');
        return index < candidateCount;
    }

    if (key == VK_BACK || key == VK_ESCAPE || key == VK_RETURN) {
        return true;
    }

    if (key == VK_SPACE) {
        return hasCandidates;
    }

    if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN) {
        return hasCandidates;
    }

    return false;
}

}
