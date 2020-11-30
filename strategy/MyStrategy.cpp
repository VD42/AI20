#include "MyStrategy.hpp"
#include <memory>

Action MyStrategy::getAction(PlayerView const& playerView, DebugInterface * debugInterface)
{
    std::vector<std::vector<int>> placement;
    if (placement.empty())
    {
        placement.reserve(playerView.mapSize);
        for (int i = 0; i < playerView.mapSize; ++i)
            placement.emplace_back(playerView.mapSize, 0);
    }

    for (int i = 0; i < playerView.mapSize; ++i)
        for (int j = 0; j < playerView.mapSize; ++j)
            placement[i][j] = 0;

    const auto get_placement = [&] (size_t x, size_t y) -> int& {
        static int wrong_place;
        wrong_place = -std::numeric_limits<int>::min();
        if (x < 0) return wrong_place;
        if (y < 0) return wrong_place;
        if (placement.size() <= x) return wrong_place;
        if (placement.size() <= y) return wrong_place;
        return placement[x][y];
    };

    int units_limit = 0;
    int units = 0;

    std::vector<int> to_repair;
    std::unordered_map<int, size_t> id_to_index;

    std::vector<int> to_attack;

    std::vector<int> resources;

    int builders = 0;

    size_t index = 0;
    for (auto const& entity : playerView.entities)
    {
        id_to_index[entity.id] = index++;
        for (int i = entity.position.x; i < entity.position.x + playerView.entityProperties.at(entity.entityType).size; ++i)
            for (int j = entity.position.y; j < entity.position.y + playerView.entityProperties.at(entity.entityType).size; ++j)
                get_placement(i, j) = entity.id;
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_BASE || entity.entityType == EntityType::MELEE_BASE || entity.entityType == EntityType::RANGED_BASE))
        {
            for (int j = entity.position.y; j < entity.position.y + playerView.entityProperties.at(entity.entityType).size; ++j)
            {
                get_placement(entity.position.x - 1, j) = -entity.id;
                get_placement(entity.position.x + playerView.entityProperties.at(entity.entityType).size, j) = -entity.id;
            }
            for (int i = entity.position.x; i < entity.position.x + playerView.entityProperties.at(entity.entityType).size; ++i)
            {
                get_placement(i, entity.position.y - 1) = -entity.id;
                get_placement(i, entity.position.y + playerView.entityProperties.at(entity.entityType).size) = -entity.id;
            }
        }
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_BASE || entity.entityType == EntityType::MELEE_BASE || entity.entityType == EntityType::RANGED_BASE || entity.entityType == EntityType::HOUSE))
        {
            units_limit += 5;
            if (entity.health < playerView.entityProperties.at(entity.entityType).maxHealth)
                to_repair.emplace_back(entity.id);
        }
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_UNIT || entity.entityType == EntityType::MELEE_UNIT || entity.entityType == EntityType::RANGED_UNIT))
            units += 1;
        if (entity.playerId && *entity.playerId != playerView.myId)
            to_attack.emplace_back(entity.id);
        if (entity.entityType == EntityType::RESOURCE)
            resources.emplace_back(entity.id);
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::BUILDER_UNIT)
            ++builders;
    }

    const auto find_place_for_unit = [&] (Entity const& entity) {
        for (int j = entity.position.y + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.y <= j; --j)
            if (get_placement(entity.position.x + playerView.entityProperties.at(entity.entityType).size, j) == -entity.id)
                return Vec2Int(entity.position.x + playerView.entityProperties.at(entity.entityType).size, j);
        for (int i = entity.position.x + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.x <= i; --i)
            if (get_placement(i, entity.position.y + playerView.entityProperties.at(entity.entityType).size) == -entity.id)
                return Vec2Int(i, entity.position.y + playerView.entityProperties.at(entity.entityType).size);
        for (int i = entity.position.x + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.x <= i; --i)
            if (get_placement(i, entity.position.y - 1) == -entity.id)
                return Vec2Int(i, entity.position.y - 1);
        for (int j = entity.position.y + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.y <= j; --j)
            if (get_placement(entity.position.x - 1, j) == -entity.id)
                return Vec2Int(entity.position.x -1, j);
        return entity.position;
    };

    const auto find_place_for_house = [&] (Entity const& entity) {
        for (int start_y = entity.position.y - playerView.entityProperties.at(EntityType::HOUSE).size + 1; start_y <= entity.position.y; ++start_y)
        {
            bool good = true;
            for (int i = entity.position.x - playerView.entityProperties.at(EntityType::HOUSE).size; i < entity.position.x; ++i)
            {
                for (int j = start_y; j < start_y + playerView.entityProperties.at(EntityType::HOUSE).size; ++j)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(entity.position.x - playerView.entityProperties.at(EntityType::HOUSE).size, start_y);
        }

        for (int start_x = entity.position.x - playerView.entityProperties.at(EntityType::HOUSE).size + 1; start_x <= entity.position.x; ++start_x)
        {
            bool good = true;
            for (int j = entity.position.y - playerView.entityProperties.at(EntityType::HOUSE).size; j < entity.position.y; ++j)
            {
                for (int i = start_x; i < start_x + playerView.entityProperties.at(EntityType::HOUSE).size; ++i)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(start_x, entity.position.y - playerView.entityProperties.at(EntityType::HOUSE).size);
        }

        for (int start_x = entity.position.x - playerView.entityProperties.at(EntityType::HOUSE).size + 1; start_x <= entity.position.x; ++start_x)
        {
            bool good = true;
            for (int j = entity.position.y + 1; j <= entity.position.y + playerView.entityProperties.at(EntityType::HOUSE).size; ++j)
            {
                for (int i = start_x; i < start_x + playerView.entityProperties.at(EntityType::HOUSE).size; ++i)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(start_x, entity.position.y + 1);
        }

        for (int start_y = entity.position.y - playerView.entityProperties.at(EntityType::HOUSE).size + 1; start_y <= entity.position.y; ++start_y)
        {
            bool good = true;
            for (int i = entity.position.x + 1; i <= entity.position.x + playerView.entityProperties.at(EntityType::HOUSE).size; ++i)
            {
                for (int j = start_y; j < start_y + playerView.entityProperties.at(EntityType::HOUSE).size; ++j)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(entity.position.x + 1, start_y);
        }

        return entity.position;
    };

    const auto get_closest_entity = [&] (Entity const& entity, std::vector<int> const& entities) {
        auto distance = playerView.mapSize * 2;
        int result = 0;
        for (auto const& e : entities)
        {
            const auto d = std::abs(entity.position.x - playerView.entities[id_to_index[e]].position.x) + std::abs(entity.position.y - playerView.entities[id_to_index[e]].position.y);
            if (d < distance)
            {
                result = e;
                distance = d;
            }
        }
        return result;
    };

    int current_resources = [&] () {
        for (auto const& player : playerView.players)
            if (player.id == playerView.myId)
                return player.resource;
        return 0;
    }();

    const auto action_for_builder_unit = [&] (Entity const& entity) {
        if (!to_repair.empty())
        {
            const auto id = get_closest_entity(entity, to_repair);
            return EntityAction(nullptr, nullptr, nullptr, std::make_unique<RepairAction>(id));
        }
        if (units_limit <= units && playerView.entityProperties.at(EntityType::HOUSE).cost <= current_resources)
        {
            const auto pos = find_place_for_house(entity);
            if (pos.x != entity.position.x && pos.y != entity.position.y)
            {
                units_limit += 5;
                return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::HOUSE, pos), nullptr, nullptr);
            }
        }
        return EntityAction(
            std::make_unique<MoveAction>(playerView.entities[id_to_index[get_closest_entity(entity, resources)]].position, true, false),
            nullptr,
            std::make_unique<AttackAction>(nullptr, std::make_unique<AutoAttack>(playerView.mapSize * 2, std::vector<EntityType> { EntityType::RESOURCE })),
            nullptr
        );
    };

    auto need_builders = std::max(10, static_cast<int>(units_limit * 0.3));

    const auto action_for_builder_base = [&] (Entity const& entity) {
        if (builders < need_builders)
        {
            const auto pos = find_place_for_unit(entity);
            return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::BUILDER_UNIT, pos), nullptr, nullptr);
        }
        return EntityAction(nullptr, nullptr, nullptr, nullptr);
    };

    const auto action_for_ranged_base = [&] (Entity const& entity) {
        const auto pos = find_place_for_unit(entity);
        return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::RANGED_UNIT, pos), nullptr, nullptr);
    };

    const auto action_for_melee_base = [&] (Entity const& entity) {
        return EntityAction(nullptr, nullptr, nullptr, nullptr);
    };

    const auto action_for_ranged_unit = [&] (Entity const& entity) {
        return EntityAction(
            std::make_unique<MoveAction>(playerView.entities[id_to_index[get_closest_entity(entity, to_attack)]].position, true, false),
            nullptr,
            std::make_unique<AttackAction>(nullptr, std::make_unique<AutoAttack>(playerView.mapSize * 2, std::vector<EntityType> { EntityType::HOUSE, EntityType::BUILDER_BASE, EntityType::BUILDER_UNIT, EntityType::MELEE_BASE, EntityType::MELEE_UNIT, EntityType::RANGED_BASE, EntityType::RANGED_UNIT, EntityType::TURRET })),
            nullptr
        );
    };

    std::unordered_map<int, EntityAction> actions;

    for (auto const& entity : playerView.entities)
    {
        if (!entity.playerId || *entity.playerId != playerView.myId)
            continue;
        switch (entity.entityType)
        {
        case EntityType::BUILDER_UNIT:
            actions.emplace(entity.id, action_for_builder_unit(entity));
            break;
        case EntityType::BUILDER_BASE:
            actions.emplace(entity.id, action_for_builder_base(entity));
            break;
        case EntityType::RANGED_BASE:
            actions.emplace(entity.id, action_for_ranged_base(entity));
            break;
        case EntityType::MELEE_BASE:
            actions.emplace(entity.id, action_for_melee_base(entity));
            break;
        case EntityType::RANGED_UNIT:
            actions.emplace(entity.id, action_for_ranged_unit(entity));
            break;
        case EntityType::MELEE_UNIT:
            actions.emplace(entity.id, action_for_ranged_unit(entity));
            break;
        case EntityType::TURRET:
            actions.emplace(entity.id, action_for_ranged_unit(entity));
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