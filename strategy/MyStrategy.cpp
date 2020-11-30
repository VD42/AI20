#include "MyStrategy.hpp"
#include <memory>

Action MyStrategy::getAction(PlayerView const& playerView, DebugInterface * debugInterface)
{
    std::unordered_map<int, EntityAction> actions;

    int resources = [&] () {
        for (auto const& player : playerView.players)
            if (player.id == playerView.myId)
                return player.resource;
        return 0;
    }();

    auto attack = [&] () -> std::unique_ptr<AttackAction> {
        return std::make_unique<AttackAction>(nullptr, std::make_unique<AutoAttack>(playerView.maxPathfindNodes, std::vector<EntityType>{ EntityType::HOUSE, EntityType::BUILDER_BASE, EntityType::BUILDER_UNIT, EntityType::MELEE_BASE, EntityType::MELEE_UNIT, EntityType::RANGED_BASE, EntityType::RANGED_UNIT, EntityType::TURRET }));
    };

    auto move = [&] () -> std::unique_ptr<MoveAction> {
        for (auto const& entity : playerView.entities)
        {
            if (!entity.playerId || *entity.playerId == playerView.myId)
                continue;
            return std::make_unique<MoveAction>(entity.position, true, false);
        }
        return nullptr;
    };

    auto repair = [&] () -> std::unique_ptr<RepairAction> {
        for (auto const& entity : playerView.entities)
        {
            if (!entity.playerId || *entity.playerId != playerView.myId)
                continue;
            if (entity.active)
                continue;
            return std::make_unique<RepairAction>(entity.id);
        }
        return nullptr;
    };

    auto move_r = [&] () -> std::unique_ptr<MoveAction> {
        for (auto const& entity : playerView.entities)
        {
            if (entity.entityType != EntityType::RESOURCE)
                continue;
            return std::make_unique<MoveAction>(entity.position, true, false);
        }
        return nullptr;
    };

    auto build_r = [&] (Vec2Int pos) -> std::unique_ptr<BuildAction> {
        pos.x -= playerView.entityProperties.at(EntityType::HOUSE).size;
        int population =  -15;
        for (auto const& entity : playerView.entities)
        {
            if (!entity.playerId || *entity.playerId != playerView.myId)
                continue;
            if (entity.entityType == EntityType::HOUSE)
                population -= 5;
            else if (entity.entityType == EntityType::BUILDER_UNIT || entity.entityType == EntityType::MELEE_UNIT || entity.entityType == EntityType::RANGED_UNIT)
                population += 1;
        }
        if (-5 < population)
            return std::make_unique<BuildAction>(EntityType::HOUSE, pos);
        return nullptr;
    };

    for (auto const& entity : playerView.entities)
    {
        if (!entity.playerId || *entity.playerId != playerView.myId)
            continue;
        switch (entity.entityType)
        {
        case EntityType::BUILDER_UNIT:
            actions.emplace(entity.id, EntityAction(move_r(), build_r(entity.position), std::make_unique<AttackAction>(nullptr, std::make_unique<AutoAttack>(playerView.maxPathfindNodes, std::vector<EntityType>{ EntityType::RESOURCE })), repair()));
            break;
        case EntityType::RANGED_UNIT:
            actions.emplace(entity.id, EntityAction(move(), nullptr, attack(), nullptr));
            break;
        case EntityType::MELEE_UNIT:
            actions.emplace(entity.id, EntityAction(move(), nullptr, attack(), nullptr));
            break;
        case EntityType::BUILDER_BASE:
            {
                int engs = 0;
                for (auto const& entity : playerView.entities)
                {
                    if (!entity.playerId || *entity.playerId != playerView.myId)
                        continue;
                    if (entity.entityType != EntityType::BUILDER_UNIT)
                        continue;
                    ++engs;
                }
                actions.emplace(entity.id, EntityAction(nullptr, (engs < 10 ? std::make_unique<BuildAction>(EntityType::BUILDER_UNIT, Vec2Int(entity.position.x + playerView.entityProperties.at(EntityType::BUILDER_BASE).size, entity.position.y)) : nullptr), nullptr, nullptr));
            }
            break;
        case EntityType::RANGED_BASE:
            actions.emplace(entity.id, EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::RANGED_UNIT, Vec2Int(entity.position.x + playerView.entityProperties.at(EntityType::RANGED_BASE).size, entity.position.y)), nullptr, nullptr));
            break;
        case EntityType::MELEE_BASE:
            break;
        }
    }

    return Action(std::move(actions));
}

void MyStrategy::debugUpdate(PlayerView const& playerView, DebugInterface & debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    debugInterface.getState();
}