#include "MyStrategy.hpp"
#include <memory>
#include <algorithm>

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

    std::vector<std::tuple<int, int, int>> order;

    int builders = 0;

    int builder_bases = 0;
    int ranged_bases = 0;

    size_t index = 0;
    for (auto const& entity : playerView.entities)
    {
        if (entity.playerId && *entity.playerId == playerView.myId)
        {
            order.emplace_back(std::make_tuple(
                [&] () {
                    switch (entity.entityType)
                    {
                    case EntityType::BUILDER_UNIT: return 0;
                    case EntityType::BUILDER_BASE: return 1;
                    case EntityType::RANGED_BASE: return 2;
                    case EntityType::MELEE_BASE: return 3;
                    case EntityType::RANGED_UNIT: return 4;
                    case EntityType::MELEE_UNIT: return 5;
                    case EntityType::TURRET: return 6;
                    }
                    return 100;
                }(),
                entity.position.x + entity.position.y,
                entity.id
            ));
        }
        id_to_index[entity.id] = index++;
        int border = 0;
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_BASE || entity.entityType == EntityType::MELEE_BASE || entity.entityType == EntityType::RANGED_BASE || entity.entityType == EntityType::HOUSE || entity.entityType == EntityType::TURRET))
            border = 1;
        for (int i = entity.position.x - border; i < entity.position.x + playerView.entityProperties.at(entity.entityType).size + border; ++i)
            for (int j = entity.position.y - border; j < entity.position.y + playerView.entityProperties.at(entity.entityType).size + border; ++j)
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
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::BUILDER_BASE)
            ++builder_bases;
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::RANGED_BASE)
            ++ranged_bases;
    }

    std::sort(order.begin(), order.end());

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

    const auto find_place_for_base = [&] (Entity const& entity, EntityType type) {
        const auto size = playerView.entityProperties.at(type).size;

        for (int start_y = entity.position.y - size + 1; start_y <= entity.position.y; ++start_y)
        {
            bool good = true;
            for (int i = entity.position.x - size; i < entity.position.x; ++i)
            {
                for (int j = start_y; j < start_y + size; ++j)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(entity.position.x - size, start_y);
        }

        for (int start_x = entity.position.x - size + 1; start_x <= entity.position.x; ++start_x)
        {
            bool good = true;
            for (int j = entity.position.y - size; j < entity.position.y; ++j)
            {
                for (int i = start_x; i < start_x + size; ++i)
                    if (get_placement(i, j) != 0)
                    {
                        good = false;
                        break;
                    }
                if (!good)
                    break;
            }
            if (good)
                return Vec2Int(start_x, entity.position.y - size);
        }

        for (int start_x = entity.position.x - size + 1; start_x <= entity.position.x; ++start_x)
        {
            bool good = true;
            for (int j = entity.position.y + 1; j <= entity.position.y + size; ++j)
            {
                for (int i = start_x; i < start_x + size; ++i)
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

        for (int start_y = entity.position.y - size + 1; start_y <= entity.position.y; ++start_y)
        {
            bool good = true;
            for (int i = entity.position.x + 1; i <= entity.position.x + size; ++i)
            {
                for (int j = start_y; j < start_y + size; ++j)
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

    const auto distance = [&] (Entity const& entity1, Entity const& entity2) {
        int mind = playerView.mapSize * 2;
        for (int i = entity2.position.x; i < entity2.position.x + playerView.entityProperties.at(entity2.entityType).size; ++i)
            for (int j = entity2.position.y; j < entity2.position.y + playerView.entityProperties.at(entity2.entityType).size; ++j)
            {
                const auto d = std::abs(entity1.position.x - i) + std::abs(entity1.position.y - j);
                if (d < mind)
                    mind = d;
            }
        return mind;
    };

    const auto get_closest_entity = [&] (Entity const& entity, std::vector<int> const& entities) {
        auto mind = playerView.mapSize * 2;
        int result = 0;
        for (auto const& e : entities)
        {
            const auto d = distance(entity, playerView.entities[id_to_index[e]]);
            if (d < mind)
            {
                result = e;
                mind = d;
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

    int need_builder_bases = 1;
    int need_ranged_bases = 1 + (builders / 30);
    int need_melee_bases = 0;

    const auto bases_ok = [&] () {
        if (builder_bases < need_builder_bases) return false;
        if (ranged_bases < need_ranged_bases) return false;
        if (units_limit <= units + 4 * (builder_bases + ranged_bases)) return false;
        return true;
    };

    const auto action_for_builder_unit = [&] (Entity const& entity) {
        if (builder_bases < need_builder_bases)
        {
            const auto pos = find_place_for_base(entity, EntityType::BUILDER_BASE);
            if (pos.x != entity.position.x && pos.y != entity.position.y)
            {
                current_resources -= playerView.entityProperties.at(EntityType::BUILDER_BASE).cost;
                if (0 <= current_resources)
                {
                    units_limit += 5;
                    builder_bases += 1;
                    return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::BUILDER_BASE, pos), nullptr, nullptr);
                }
            }
        }
        else if (ranged_bases < need_ranged_bases)
        {
            const auto pos = find_place_for_base(entity, EntityType::RANGED_BASE);
            if (pos.x != entity.position.x && pos.y != entity.position.y)
            {
                current_resources -= playerView.entityProperties.at(EntityType::RANGED_BASE).cost;
                if (0 <= current_resources)
                {
                    units_limit += 5;
                    ranged_bases += 1;
                    return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::RANGED_BASE, pos), nullptr, nullptr);
                }
            }
        }
        else if (units_limit <= units + 4 * (builder_bases + ranged_bases))
        {
            const auto pos = find_place_for_base(entity, EntityType::HOUSE);
            if (pos.x != entity.position.x && pos.y != entity.position.y)
            {
                current_resources -= playerView.entityProperties.at(EntityType::HOUSE).cost;
                if (0 <= current_resources)
                {
                    units_limit += 5;
                    return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::HOUSE, pos), nullptr, nullptr);
                }
            }
        }
        if (!to_repair.empty())
        {
            const auto id = get_closest_entity(entity, to_repair);
            if (playerView.entities[id_to_index[id]].entityType != EntityType::MELEE_BASE && distance(entity, playerView.entities[id_to_index[id]]) <= (playerView.entities[id_to_index[id]].entityType == EntityType::HOUSE ? 1 : 5))
            {
                return EntityAction(
                    std::make_unique<MoveAction>(playerView.entities[id_to_index[id]].position, true, false),
                    nullptr,
                    nullptr,
                    std::make_unique<RepairAction>(id)
                );
            }
        }
        return EntityAction(
            std::make_unique<MoveAction>(playerView.entities[id_to_index[get_closest_entity(entity, resources)]].position, true, false),
            nullptr,
            std::make_unique<AttackAction>(nullptr, std::make_unique<AutoAttack>(playerView.mapSize * 2, std::vector<EntityType> { EntityType::RESOURCE })),
            nullptr
        );
    };

    auto need_builders = std::max(50, static_cast<int>(units_limit * 0.5));

    const auto action_for_builder_base = [&] (Entity const& entity) {
        if (bases_ok() && builders < need_builders)
        {
            current_resources -= playerView.entityProperties.at(EntityType::BUILDER_UNIT).cost;
            if (0 <= current_resources)
            {
                const auto pos = find_place_for_unit(entity);
                return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::BUILDER_UNIT, pos), nullptr, nullptr);
            }
        }
        return EntityAction(nullptr, nullptr, nullptr, nullptr);
    };

    const auto action_for_ranged_base = [&] (Entity const& entity) {
        if (bases_ok())
        {
            current_resources -= playerView.entityProperties.at(EntityType::RANGED_UNIT).cost;
            if (0 <= current_resources)
            {
                const auto pos = find_place_for_unit(entity);
                return EntityAction(nullptr, std::make_unique<BuildAction>(EntityType::RANGED_UNIT, pos), nullptr, nullptr);
            }
        }
        return EntityAction(nullptr, nullptr, nullptr, nullptr);
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

    for (auto const& o : order)
    {
        auto const& entity = playerView.entities[id_to_index[std::get<2>(o)]];
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