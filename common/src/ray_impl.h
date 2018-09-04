/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRENCHBROOM_RAY_IMPL_H
#define TRENCHBROOM_RAY_IMPL_H

#include "ray_decl.h"
#include "vec_decl.h"
#include "vec_impl.h"

#include "MathUtils.h"

#include <cstddef>

template <typename T, size_t S>
ray<T,S>::ray() :
origin(vec<T,S>::zero),
direction(vec<T,S>::zero) {}

template <typename T, size_t S>
ray<T,S>::ray(const vec<T,S>& i_origin, const vec<T,S>& i_direction) :
origin(i_origin),
direction(i_direction) {}

template <typename T, size_t S>
vec<T,S> ray<T,S>::pointAtDistance(const T distance) const {
    return origin + direction * distance;
}

template <typename T, size_t S>
Math::PointStatus::Type ray<T,S>::pointStatus(const vec<T,S>& point) const {
    const auto scale = dot(direction, point - origin);
    if (scale >  Math::Constants<T>::pointStatusEpsilon()) {
        return Math::PointStatus::PSAbove;
    } else if (scale < -Math::Constants<T>::pointStatusEpsilon()) {
        return Math::PointStatus::PSBelow;
    } else {
        return Math::PointStatus::PSInside;
    }
}

template <typename T, size_t S>
T ray<T,S>::distanceToPointOnRay(const vec<T,S>& point) const {
    return dot(point - origin, direction);
}

template <typename T, size_t S>
bool operator==(const ray<T,S>& lhs, const ray<T,S>& rhs) {
    return lhs.origin == rhs.origin && lhs.direction == rhs.direction;
}

template <typename T, size_t S>
bool operator!=(const ray<T,S>& lhs, const ray<T,S>& rhs) {
    return lhs.origin != rhs.origin || lhs.direction != rhs.direction;
}

template <typename T, size_t S>
std::ostream& operator<<(std::ostream& stream, const ray<T,S>& ray) {
    stream << "{ origin: (" << ray.origin << "), direction: (" << ray.direction << ") }";
    return stream;
}

#endif //TRENCHBROOM_RAY_IMPL_H
