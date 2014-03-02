#pragma once
#ifndef INTERPOLATED_MOVEMENT_H
#define INTERPOLATED_MOVEMENT_H

#include <list>
#include <iostream>
#include <unordered_map>

#include "IComponent.h"
#include "glm/glm.hpp"
#include "Sigma.h"
#include "GLTransform.h"
#include "composites/ControllableMove.h"
#include "composites/PhysicalWorldLocation.h"

namespace Sigma{
    /** \brief A component for entities that have a linear interpolated movement
     *
     * Their target transformation matrix is stored and a function allows to compute
     * the intermediary position by linear interpolation, and to substract
     * the delta from the target transformation matrix for next iteration
     */
    class InterpolatedMovement {
    public:
		SET_STATIC_COMPONENT_TYPENAME("InterpolatedMovement");
        InterpolatedMovement() { } // Default ctor setting entity ID to 0.

        virtual ~InterpolatedMovement() {};

        static std::vector<std::unique_ptr<IECSComponent>> AddEntity(const id_t id) {
            if (getRotationTarget(id) == nullptr) {
                rotationtarget_map.emplace(id, glm::vec3());
                //TODO: return components
                return std::vector<std::unique_ptr<IECSComponent>>();
            }
            return std::vector<std::unique_ptr<IECSComponent>>();
        };

        static void RemoveEntity(const id_t id) {
            rotationtarget_map.erase(id);
        };

        /** \brief Compute interpolated forces.
         *
         * This function will compute the interpolated position of all entities that have this component
         * It will update the transformation matrix of the ControllableMove component
         *
         * \param delta const double Change in time since the last call.
         */
        static void ComputeInterpolatedForces(const double delta) {
            glm::vec3 deltavec(delta);
            for (auto it2 = rotationtarget_map.begin(); it2 != rotationtarget_map.end(); it2++) {
                auto transform = PhysicalWorldLocation::GetTransform(it2->first);
                auto rotationForces = ControllableMove::getRotationForces(it2->first);

                if (transform != nullptr) {

                    for (auto rotitr = rotationForces->begin(); rotitr != rotationForces->end(); ++rotitr) {
                        transform->Rotate((*rotitr) * deltavec);
                    }
                    rotationForces->clear();

                    glm::vec3 targetrvel;
                    auto _rotationtarget = it2->second;

                    targetrvel = _rotationtarget * deltavec;
                    if(fabs(targetrvel.x) > 0.0001f || fabs(targetrvel.y) > 0.0001f || fabs(targetrvel.z) > 0.0001f) {
                        targetrvel = transform->Restrict(targetrvel);
                        transform->Rotate(targetrvel);
                        _rotationtarget -= targetrvel;
                        it2->second = _rotationtarget;
                    }
                }
            }
        };

        static void RotateTarget(const id_t id, float x, float y, float z) {
            auto _rotationtarget = getRotationTarget(id);
            if (_rotationtarget == nullptr) {
                assert(0 && "id does not exist");
            }
            *_rotationtarget += glm::vec3(x,y,z);
        }

        static glm::vec3* getRotationTarget(const id_t id) {
            auto rts = rotationtarget_map.find(id);
            if (rts != rotationtarget_map.end()) {
                return &rts->second;
            }
            return nullptr;
        }

    private:
        static std::unordered_map<id_t, glm::vec3> rotationtarget_map;
    }; // class InterpolatedMovement
} // namespace Sigma

#endif // INTERPOLATED_MOVEMENT_H
