#ifndef _MODEL_ENTITY_ACTION_HPP_
#define _MODEL_ENTITY_ACTION_HPP_

#include "../Stream.hpp"
#include "AttackAction.hpp"
#include "AutoAttack.hpp"
#include "BuildAction.hpp"
#include "EntityType.hpp"
#include "MoveAction.hpp"
#include "RepairAction.hpp"
#include "Vec2Int.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

class EntityAction {
public:
    std::optional<MoveAction> moveAction;
    std::optional<BuildAction> buildAction;
    std::optional<AttackAction> attackAction;
    std::optional<RepairAction> repairAction;
    EntityAction();
    EntityAction(std::optional<MoveAction> moveAction, std::optional<BuildAction> buildAction, std::optional<AttackAction> attackAction, std::optional<RepairAction> repairAction);
    static EntityAction readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
};

#endif
