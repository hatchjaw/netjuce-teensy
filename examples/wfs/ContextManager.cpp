//
// Created by tar on 14/04/23.
//

#include "ContextManager.h"

size_t ContextManager::printTo(Print &p) const { return p.print("ContextManager"); }

bool ContextManager::init() {
    // Set up source positions.
    char path[5];
    for (int i{0}; i < context.numSources; ++i) {
        sprintf(path, "%d/x", i);
        context.sourcePositions.insert(SourcePositionsMap::value_type(
                path,
                SmoothedValue_V2<double>{0., .999f})
        );
        sprintf(path, "%d/y", i);
        context.sourcePositions.insert(SourcePositionsMap::value_type(
                path,
                SmoothedValue_V2<double>{0., .999f})
        );
    }

    return true;
}

void ContextManager::loop() {
    // Move smoothed values along.
//    for (auto &it: context.sourcePositions) {
//        it.second.getNext();
//    }
}
