#ifndef _MODEL_ATTACK_ACTION_HPP_
#define _MODEL_ATTACK_ACTION_HPP_

#include "../Stream.hpp"
#include "AutoAttack.hpp"
#include "EntityType.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

class AttackAction {
public:
    std::optional<int> target;
    std::optional<AutoAttack> autoAttack;
    AttackAction();
    AttackAction(std::optional<int> target, std::optional<AutoAttack> autoAttack);
    static AttackAction readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
};

#endif
