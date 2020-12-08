#include "EntityProperties.hpp"

EntityProperties::EntityProperties() { }
EntityProperties::EntityProperties(int size, int buildScore, int destroyScore, bool canMove, int populationProvide, int populationUse, int maxHealth, int initialCost, int sightRange, int resourcePerHealth, std::optional<BuildProperties> build, std::optional<AttackProperties> attack, std::optional<RepairProperties> repair) : size(size), buildScore(buildScore), destroyScore(destroyScore), canMove(canMove), populationProvide(populationProvide), populationUse(populationUse), maxHealth(maxHealth), initialCost(initialCost), sightRange(sightRange), resourcePerHealth(resourcePerHealth), build(build), attack(attack), repair(repair) { }
EntityProperties EntityProperties::readFrom(InputStream& stream) {
    EntityProperties result;
    result.size = stream.readInt();
    result.buildScore = stream.readInt();
    result.destroyScore = stream.readInt();
    result.canMove = stream.readBool();
    result.populationProvide = stream.readInt();
    result.populationUse = stream.readInt();
    result.maxHealth = stream.readInt();
    result.initialCost = stream.readInt();
    result.sightRange = stream.readInt();
    result.resourcePerHealth = stream.readInt();
    if (stream.readBool()) {
        result.build = BuildProperties::readFrom(stream);
    }
    if (stream.readBool()) {
        result.attack = AttackProperties::readFrom(stream);
    }
    if (stream.readBool()) {
        result.repair = RepairProperties::readFrom(stream);
    }
    return result;
}
void EntityProperties::writeTo(OutputStream& stream) const {
    stream.write(size);
    stream.write(buildScore);
    stream.write(destroyScore);
    stream.write(canMove);
    stream.write(populationProvide);
    stream.write(populationUse);
    stream.write(maxHealth);
    stream.write(initialCost);
    stream.write(sightRange);
    stream.write(resourcePerHealth);
    if (build) {
        stream.write(true);
        (*build).writeTo(stream);
    } else {
        stream.write(false);
    }
    if (attack) {
        stream.write(true);
        (*attack).writeTo(stream);
    } else {
        stream.write(false);
    }
    if (repair) {
        stream.write(true);
        (*repair).writeTo(stream);
    } else {
        stream.write(false);
    }
}
