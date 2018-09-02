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

#ifndef TRENCHBROOM_LINE_DECL
#define TRENCHBROOM_LINE_DECL

#include "vec_decl.h"

#include <vector>

template <typename T, size_t S>
class line {
public:
    typedef std::vector<line<T,S> > List;
    static const List EmptyList;
    
    vec<T,S> point;
    vec<T,S> direction;

    line();
    
    // Copy and move constructors
    line(const line<T,S>& line) = default;
    line(line<T,S>&& other) = default;
    
    // Assignment operators
    line<T,S>& operator=(const line<T,S>& other) = default;
    line<T,S>& operator=(line<T,S>&& other) = default;

    /**
     * Converts the given line by converting its component type using static_cast.
     *
     * @tparam U the component type of the given line
     * @param line the line to convert
     */
    template <typename U>
    line(const line<U,S>& line) :
    point(line.point),
    direction(line.direction) {}

    /**
     * Creates a new line with the given point and direction.
     *
     * @param i_point the point
     * @param i_direction the direction
     */
    line(const vec<T,S>& i_point, const vec<T,S>& i_direction);
public:
    /**
     * Projects the given point orthogonally onto this line and computes the distance from the line anchor point to
     * the projected point.
     *
     * @param i_point the point to project
     * @return the distance
     */
    T distance(const vec<T,S>& i_point) const;

    /**
     * Returns a point on this line at the given distance from its anchor point.
     *
     * @param distance the distance of the point (along the direction)
     * @return the point at the given distance
     */
    vec<T,S> pointAtDistance(T distance) const;
    

    /**
     * Orthogonally projects the given point onto this line.
     *
     * @param i_point the point to project
     * @return the projected point
     */
    vec<T,S> project(const vec<T,S>& i_point) const;

    /**
     * Returns a canonical representation of the given line. Since a line could be represented by any point on it
     * plus its direction, every line has an infinite number of representations. This function maps each representation
     * onto a unique representation.
     *
     * @return the canonical representation of this line
     */
    line<T,S> makeCanonical() const;
};

/**
 * Checks whether the two given lines are identical.
 *
 * @tparam T the component type
 * @tparam S the number of components
 * @param lhs the first line
 * @param rhs the second line
 * @return true if the given lines are identical, and false otherwise
 */
template <typename T, size_t S>
bool operator==(const line<T,S>& lhs, const line<T,S>& rhs);

/**
 * Checks whether the two given lines are identical.
 *
 * @tparam T the component type
 * @tparam S the number of components
 * @param lhs the first line
 * @param rhs the second line
 * @return false if the given lines are identical, and true otherwise
 */
template <typename T, size_t S>
bool operator!=(const line<T,S>& lhs, const line<T,S>& rhs);

/**
 * Prints a textual representation of the given line to the given stream.
 *
 * @tparam T the component type
 * @tparam S the number of components
 * @param stream the stream to print to
 * @param line the line to print
 * @return the given stream
 */
template <typename T, size_t S>
std::ostream& operator<<(std::ostream& stream, const line<T,S>& line);

#endif