#ifndef _MY_STRATEGY_HPP_
#define _MY_STRATEGY_HPP_

#include "DebugInterface.hpp"
#include "model/Model.hpp"

class MyStrategy
{
public:
    MyStrategy() = default;
    Action getAction(PlayerView const& playerView, DebugInterface * debugInterface);
    void debugUpdate(PlayerView const& playerView, DebugInterface & debugInterface);
};

#endif