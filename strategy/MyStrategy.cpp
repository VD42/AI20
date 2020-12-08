#include "MyStrategy.hpp"
#include <memory>
#include <algorithm>

Action MyStrategy::getAction(PlayerView const& playerView, DebugInterface * debugInterface)
{
    static std::vector<std::vector<int>> placement;
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
        wrong_place = std::numeric_limits<int>::max();
        if (x < 0) return wrong_place;
        if (y < 0) return wrong_place;
        if (placement.size() <= x) return wrong_place;
        if (placement.size() <= y) return wrong_place;
        return placement[x][y];
    };

    static std::vector<std::vector<int>> placement_build;
    if (placement_build.empty())
    {
        placement_build.reserve(playerView.mapSize);
        for (int i = 0; i < playerView.mapSize; ++i)
            placement_build.emplace_back(playerView.mapSize, 0);
    }

    for (int i = 0; i < playerView.mapSize; ++i)
        for (int j = 0; j < playerView.mapSize; ++j)
            placement_build[i][j] = 0;

    const auto get_placement_build = [&] (size_t x, size_t y) -> int& {
        static int wrong_place;
        wrong_place = std::numeric_limits<int>::max();
        if (x < 0) return wrong_place;
        if (y < 0) return wrong_place;
        if (placement_build.size() <= x) return wrong_place;
        if (placement_build.size() <= y) return wrong_place;
        return placement_build[x][y];
    };

    static std::vector<std::vector<int>> dangers;
    if (dangers.empty())
    {
        dangers.reserve(playerView.mapSize);
        for (int i = 0; i < playerView.mapSize; ++i)
            dangers.emplace_back(playerView.mapSize, 0);
    }

    for (int i = 0; i < playerView.mapSize; ++i)
        for (int j = 0; j < playerView.mapSize; ++j)
            dangers[i][j] = 0;

    const auto get_dangers = [&] (size_t x, size_t y) -> int& {
        static int wrong_place;
        wrong_place = std::numeric_limits<int>::max() / 2;
        if (x < 0) return wrong_place;
        if (y < 0) return wrong_place;
        if (dangers.size() <= x) return wrong_place;
        if (dangers.size() <= y) return wrong_place;
        return dangers[x][y];
    };

    int units_limit = 0;
    int units = 0;

    std::vector<int> to_repair;
    std::unordered_map<int, size_t> id_to_index;
    id_to_index.reserve(playerView.entities.size());

    std::vector<int> to_attack;

    std::vector<int> resources;

    std::vector<std::tuple<int, int, int>> order;

    std::vector<int> danger;

    int builders = 0;
    std::vector<int> ranges;
    int houses = 0;

    int builder_bases = 0;
    int ranged_bases = 0;

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
        for (int i = entity.position.x - border; i < entity.position.x + playerView.entityProperties.at(entity.entityType).size + border; ++i)
            for (int j = entity.position.y - border; j < entity.position.y + playerView.entityProperties.at(entity.entityType).size + border; ++j)
                get_placement(i, j) = entity.id;
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_BASE || entity.entityType == EntityType::MELEE_BASE || entity.entityType == EntityType::RANGED_BASE || entity.entityType == EntityType::HOUSE || entity.entityType == EntityType::TURRET))
            border = 1;
        for (int i = entity.position.x - border; i < entity.position.x + playerView.entityProperties.at(entity.entityType).size + border; ++i)
            for (int j = entity.position.y - border; j < entity.position.y + playerView.entityProperties.at(entity.entityType).size + border; ++j)
                get_placement_build(i, j) = entity.id;
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_BASE || entity.entityType == EntityType::MELEE_BASE || entity.entityType == EntityType::RANGED_BASE || entity.entityType == EntityType::HOUSE))
        {
            units_limit += playerView.entityProperties.at(entity.entityType).populationProvide;
            if (entity.health < playerView.entityProperties.at(entity.entityType).maxHealth)
                to_repair.emplace_back(entity.id);
        }
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::RANGED_UNIT)
            if (entity.health < playerView.entityProperties.at(entity.entityType).maxHealth)
                to_repair.emplace_back(entity.id);
        if (entity.playerId && *entity.playerId == playerView.myId && (entity.entityType == EntityType::BUILDER_UNIT || entity.entityType == EntityType::MELEE_UNIT || entity.entityType == EntityType::RANGED_UNIT))
            units += 1;
        if (entity.playerId && *entity.playerId != playerView.myId)
            to_attack.emplace_back(entity.id);
        if (entity.entityType == EntityType::RESOURCE)
            resources.emplace_back(entity.id);
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::BUILDER_UNIT)
            ++builders;
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::RANGED_UNIT)
            ranges.emplace_back(entity.id);
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::HOUSE)
            ++houses;
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::BUILDER_BASE)
            ++builder_bases;
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::RANGED_BASE)
            ++ranged_bases;
        if (entity.playerId && *entity.playerId != playerView.myId && (entity.entityType == EntityType::MELEE_UNIT || entity.entityType == EntityType::RANGED_UNIT || entity.entityType == EntityType::TURRET))
        {
            danger.emplace_back(entity.id);
            const int border = 3;
            for (int i = entity.position.x - border - playerView.entityProperties.at(entity.entityType).attack->attackRange; i < entity.position.x + border + playerView.entityProperties.at(entity.entityType).size + playerView.entityProperties.at(entity.entityType).attack->attackRange; ++i)
                for (int j = entity.position.y - border - playerView.entityProperties.at(entity.entityType).attack->attackRange; j < entity.position.y + border + playerView.entityProperties.at(entity.entityType).size + playerView.entityProperties.at(entity.entityType).attack->attackRange; ++j)
                    if (distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(i, j), 0, false), entity) < playerView.entityProperties.at(entity.entityType).attack->attackRange + border)
                        get_dangers(i, j) += playerView.entityProperties.at(entity.entityType).attack->damage * (playerView.entityProperties.at(entity.entityType).attack->attackRange + border - distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(i, j), 0, false), entity));
        }
    }

    std::sort(order.begin(), order.end());

    const auto find_place_for_unit = [&] (Entity const& entity, Entity const& closest_to) {
        auto minx = entity.position.x;
        auto miny = entity.position.y;
        auto mind = playerView.mapSize * 2;
        for (int j = entity.position.y + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.y <= j; --j)
            if (get_placement(entity.position.x + playerView.entityProperties.at(entity.entityType).size, j) == 0)
            {
                const auto d = distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(entity.position.x + playerView.entityProperties.at(entity.entityType).size, j), 0, false), closest_to);
                if (d < mind)
                {
                    minx = entity.position.x + playerView.entityProperties.at(entity.entityType).size;
                    miny = j;
                    mind = d;
                }
            }
        for (int i = entity.position.x + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.x <= i; --i)
            if (get_placement(i, entity.position.y + playerView.entityProperties.at(entity.entityType).size) == 0)
            {
                const auto d = distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(i, entity.position.y + playerView.entityProperties.at(entity.entityType).size), 0, false), closest_to);
                if (d < mind)
                {
                    minx = i;
                    miny = entity.position.y + playerView.entityProperties.at(entity.entityType).size;
                    mind = d;
                }
            }
        for (int i = entity.position.x + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.x <= i; --i)
            if (get_placement(i, entity.position.y - 1) == 0)
            {
                const auto d = distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(i, entity.position.y - 1), 0, false), closest_to);
                if (d < mind)
                {
                    minx = i;
                    miny = entity.position.y - 1;
                    mind = d;
                }
            }
        for (int j = entity.position.y + playerView.entityProperties.at(entity.entityType).size - 1; entity.position.y <= j; --j)
            if (get_placement(entity.position.x - 1, j) == 0)
            {
                const auto d = distance(Entity(-1, std::nullopt, EntityType::WALL, Vec2Int(entity.position.x - 1, j), 0, false), closest_to);
                if (d < mind)
                {
                    minx = entity.position.x - 1;
                    miny = j;
                    mind = d;
                }
            }
        return Vec2Int(minx, miny);
    };

    const auto find_place_for_base = [&] (Entity const& entity, EntityType type) {
        const auto size = playerView.entityProperties.at(type).size;

        for (int start_y = entity.position.y - size + 1; start_y <= entity.position.y; ++start_y)
        {
            bool good = true;
            for (int i = entity.position.x - size; i < entity.position.x; ++i)
            {
                for (int j = start_y; j < start_y + size; ++j)
                    if (get_placement_build(i, j) != 0)
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
                    if (get_placement_build(i, j) != 0)
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
                    if (get_placement_build(i, j) != 0)
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
                    if (get_placement_build(i, j) != 0)
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

    const auto get_closest_to_base_entity = [&] (std::vector<int> const& entities) {
        auto mind = playerView.mapSize * 2;
        int result = 0;
        for (auto const& e : entities)
        {
            const auto d = playerView.entities[id_to_index[e]].position.x + playerView.entities[id_to_index[e]].position.y;
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

    for (auto const& entity : playerView.entities)
        if (entity.playerId && *entity.playerId == playerView.myId && entity.entityType == EntityType::BUILDER_UNIT && distance(entity, playerView.entities[id_to_index[get_closest_entity(entity, resources)]]) == 1)
            ++current_resources;

    int need_builder_bases = 1;
    int need_ranged_bases = 1;
    int need_melee_bases = 0;

    const auto builders_ok = [&] () {
        return (10 <= builders);
    };

    const auto need_houses = [&] () {
        return (builders_ok() && units_limit <= units + 4 * (builder_bases + ranged_bases));
    };

    const auto bases_ok = [&] () {
        if (!builders_ok()) return true;
        if (builder_bases < need_builder_bases) return false;
        if (ranged_bases < need_ranged_bases) return false;
        if (need_houses()) return false;
        return true;
    };

    const auto pos_is_danger = [&] (Vec2Int const& pos) {
        for (auto const& id : danger)
            if (std::abs(playerView.entities[id_to_index[id]].position.x - pos.x) + std::abs(playerView.entities[id_to_index[id]].position.y - pos.y) < 15)
                return true;
        return false;
    };

    const auto safe_move = [&] (Entity const& entity) {
        auto minx = entity.position.x;
        auto miny = entity.position.y;
        auto mind = get_dangers(minx, miny);

        auto posx = entity.position.x;
        auto posy = entity.position.y - 1;
        auto posd = get_dangers(posx, posy);
        auto pl = get_placement(posx, posy);
        if (posd < mind && pl < 1)
        {
            minx = posx;
            miny = posy;
            mind = posd;
        }

        posx = entity.position.x - 1;
        posy = entity.position.y;
        posd = get_dangers(posx, posy);
        pl = get_placement(posx, posy);
        if (posd < mind && pl < 1)
        {
            minx = posx;
            miny = posy;
            mind = posd;
        }

        posx = entity.position.x + 1;
        posy = entity.position.y;
        posd = get_dangers(posx, posy);
        pl = get_placement(posx, posy);
        if (posd < mind && pl < 1)
        {
            minx = posx;
            miny = posy;
            mind = posd;
        }

        posx = entity.position.x;
        posy = entity.position.y + 1;
        posd = get_dangers(posx, posy);
        pl = get_placement(posx, posy);
        if (posd < mind && pl < 1)
        {
            minx = posx;
            miny = posy;
            mind = posd;
        }

        if (entity.health <= mind || minx == entity.position.x && miny == entity.position.y)
            return EntityAction(std::nullopt, std::nullopt, std::nullopt, std::nullopt);
        return EntityAction(MoveAction(Vec2Int(minx, miny), false, true), std::nullopt, std::nullopt, std::nullopt);
    };

    const auto action_for_builder_unit = [&] (Entity const& entity) {
        const auto sm = safe_move(entity);
        if (sm.moveAction)
            return sm;
        if (builders_ok() && builder_bases < need_builder_bases)
        {
            const auto pos = find_place_for_base(entity, EntityType::BUILDER_BASE);
            if (pos.x != entity.position.x && pos.y != entity.position.y && !pos_is_danger(pos))
            {
                current_resources -= playerView.entityProperties.at(EntityType::BUILDER_BASE).initialCost;
                if (0 <= current_resources)
                {
                    units_limit += playerView.entityProperties.at(EntityType::BUILDER_BASE).populationProvide;
                    builder_bases += 1;
                    return EntityAction(std::nullopt, BuildAction(EntityType::BUILDER_BASE, pos), std::nullopt, std::nullopt);
                }
            }
        }
        else if (builders_ok() && ranged_bases < need_ranged_bases)
        {
            const auto pos = find_place_for_base(entity, EntityType::RANGED_BASE);
            if (pos.x != entity.position.x && pos.y != entity.position.y && !pos_is_danger(pos))
            {
                current_resources -= playerView.entityProperties.at(EntityType::RANGED_BASE).initialCost;
                if (0 <= current_resources)
                {
                    units_limit += playerView.entityProperties.at(EntityType::RANGED_BASE).populationProvide;
                    ranged_bases += 1;
                    return EntityAction(std::nullopt, BuildAction(EntityType::RANGED_BASE, pos), std::nullopt, std::nullopt);
                }
            }
        }
        else if (need_houses())
        {
            const auto pos = find_place_for_base(entity, EntityType::HOUSE);
            if (pos.x != entity.position.x && pos.y != entity.position.y && !pos_is_danger(pos))
            {
                current_resources -= playerView.entityProperties.at(EntityType::HOUSE).initialCost;
                if (0 <= current_resources)
                {
                    units_limit += playerView.entityProperties.at(EntityType::HOUSE).populationProvide;
                    return EntityAction(std::nullopt, BuildAction(EntityType::HOUSE, pos), std::nullopt, std::nullopt);
                }
            }
        }
        if (!to_repair.empty())
        {
            const auto id = get_closest_entity(entity, to_repair);
            const auto range = [&] () {
                switch (playerView.entities[id_to_index[id]].entityType)
                {
                case EntityType::BUILDER_BASE:
                case EntityType::RANGED_BASE:
                    return 3;
                default:
                    return 1;
                }
            }();
            if (playerView.entities[id_to_index[id]].entityType != EntityType::MELEE_BASE && (resources.empty() || distance(entity, playerView.entities[id_to_index[id]]) <= range))
            {
                return EntityAction(
                    MoveAction(find_place_for_unit(playerView.entities[id_to_index[id]], entity), true, false),
                    std::nullopt,
                    std::nullopt,
                    RepairAction(id)
                );
            }
        }
        return EntityAction(
            MoveAction((resources.empty() ? Vec2Int(0, 0) : playerView.entities[id_to_index[get_closest_to_base_entity(resources)]].position), true, false),
            std::nullopt,
            AttackAction(std::nullopt, AutoAttack(playerView.maxPathfindNodes, std::vector<EntityType> { EntityType::RESOURCE })),
            std::nullopt
        );
    };

    const auto need_builders = std::min(std::max(50, static_cast<int>(units_limit * 0.5)), static_cast<int>(resources.size()));

    int need_ranges = 0;
    int close_ranges = 0;
    for (auto const& d : danger)
        if (playerView.entities[id_to_index[d]].position.x <= playerView.mapSize / 2 && playerView.entities[id_to_index[d]].position.y <= playerView.mapSize / 2)
            ++need_ranges;
    for (auto const& r : ranges)
        if (playerView.entities[id_to_index[r]].position.x <= playerView.mapSize / 2 && playerView.entities[id_to_index[r]].position.y <= playerView.mapSize / 2)
            ++close_ranges;

    const auto action_for_builder_base = [&] (Entity const& entity) {
        if (bases_ok() && builders < need_builders)
        {
            auto available_resources = (need_ranges - close_ranges) * (playerView.entityProperties.at(EntityType::RANGED_UNIT).initialCost + static_cast<int>(ranges.size()));
            if (builders_ok() && 0 < available_resources)
                available_resources = current_resources - available_resources;
            else
                available_resources = current_resources;
            if ((playerView.entityProperties.at(EntityType::BUILDER_UNIT).initialCost + builders) <= available_resources)
            {
                current_resources -= playerView.entityProperties.at(EntityType::BUILDER_UNIT).initialCost + builders;
                if (0 <= current_resources)
                {
                    const auto pos = find_place_for_unit(entity, playerView.entities[id_to_index[get_closest_to_base_entity(resources)]]);
                    return EntityAction(std::nullopt, BuildAction(EntityType::BUILDER_UNIT, pos), std::nullopt, std::nullopt);
                }
            }
        }
        return EntityAction(std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    };

    const auto action_for_ranged_base = [&] (Entity const& entity) {
        if (bases_ok() && builders_ok())
        {
            current_resources -= playerView.entityProperties.at(EntityType::RANGED_UNIT).initialCost + static_cast<int>(ranges.size());
            if (0 <= current_resources)
            {
                const auto pos = find_place_for_unit(entity, playerView.entities[id_to_index[get_closest_to_base_entity(to_attack)]]);
                return EntityAction(std::nullopt, BuildAction(EntityType::RANGED_UNIT, pos), std::nullopt, std::nullopt);
            }
        }
        return EntityAction(std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    };

    const auto action_for_melee_base = [&] (Entity const& entity) {
        return EntityAction(std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    };

    std::unordered_map<int, int> to_attack_health_map;
    to_attack_health_map.reserve(to_attack.size());
    for (auto const& attack : to_attack)
        to_attack_health_map[attack] = playerView.entities[id_to_index[attack]].health;

    const auto action_for_ranged_unit = [&] (Entity const& entity) {
        const auto safe_zone = 5;
        int my_units = 0;
        int enemy_units = 0;
        for (int i = entity.position.x - safe_zone; i <= entity.position.x + safe_zone; ++i)
            for (int j = entity.position.y - safe_zone; j <= entity.position.y + safe_zone; ++j)
            {
                if (const auto id = get_placement(i, j); 0 < id && id != std::numeric_limits<int>::max() && playerView.entities[id_to_index[id]].playerId && *playerView.entities[id_to_index[id]].playerId == playerView.myId && (playerView.entities[id_to_index[id]].entityType == EntityType::RANGED_UNIT || playerView.entities[id_to_index[id]].entityType == EntityType::MELEE_UNIT))
                    ++my_units;
                if (const auto id = get_placement(i, j); 0 < id && id != std::numeric_limits<int>::max() && playerView.entities[id_to_index[id]].playerId && *playerView.entities[id_to_index[id]].playerId != playerView.myId && (playerView.entities[id_to_index[id]].entityType == EntityType::RANGED_UNIT || playerView.entities[id_to_index[id]].entityType == EntityType::MELEE_UNIT))
                    ++enemy_units;
            }
        if (my_units < enemy_units)
        {
            const auto sm = safe_move(entity);
            if (sm.moveAction)
                return sm;
        }

        std::vector<std::tuple<int, int, int>> candidates;
        for (auto const& attack : to_attack)
        {
            auto const& enemy = playerView.entities[id_to_index[attack]];
            if (to_attack_health_map[enemy.id] <= 0)
                continue;
            if (playerView.entityProperties.at(entity.entityType).attack->attackRange < distance(entity, enemy))
                continue;
            candidates.emplace_back(std::make_tuple(
                std::max(0, to_attack_health_map[enemy.id] - playerView.entityProperties.at(entity.entityType).attack->damage),
                [&] () {
                    switch (entity.entityType)
                    {
                    case EntityType::RANGED_BASE: return 0;
                    case EntityType::BUILDER_BASE: return 1;
                    case EntityType::MELEE_BASE: return 2;
                    case EntityType::RANGED_UNIT: return 3;
                    case EntityType::MELEE_UNIT: return 4;
                    case EntityType::TURRET: return 5;
                    case EntityType::BUILDER_UNIT: return 6;
                    case EntityType::HOUSE: return 7;
                    case EntityType::WALL: return 8;
                    }
                    return 100;
                }(),
                enemy.id
            ));
        }
        int id = -1;
        std::sort(candidates.begin(), candidates.end());
        if (0 < candidates.size())
        {
            id = std::get<2>(candidates.front());
            to_attack_health_map[id] -= playerView.entityProperties.at(entity.entityType).attack->damage;
        }

        auto move_id = get_closest_to_base_entity(to_attack);
        if (move_id == 0 || 15 < distance(entity, playerView.entities[id_to_index[move_id]]))
            move_id = get_closest_entity(entity, to_attack);

        return EntityAction(
            MoveAction(move_id == 0 ? Vec2Int(0, 0) : playerView.entities[id_to_index[move_id]].position, true, true),
            std::nullopt,
            (id == -1 ? std::optional<AttackAction>() : AttackAction(id, std::nullopt)),
            std::nullopt
        );
    };

    std::unordered_map<int, EntityAction> actions;
    actions.reserve(units + builder_bases + ranged_bases);

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
    //debugInterface.send(DebugCommand::Clear());
    //debugInterface.getState();
}