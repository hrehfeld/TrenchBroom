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

#ifndef TrenchBroom_Algorithms_h
#define TrenchBroom_Algorithms_h

#include "CoordinatePlane.h"
#include "Vec.h"
#include "Plane.h"
#include "Ray.h"

struct Identity {
    template<typename U>
    constexpr auto operator()(U&& v) const noexcept -> decltype(std::forward<U>(v)) {
        return std::forward<U>(v);
    }
};

template <typename T, typename I, typename F = Identity>
bool getPlane(I cur, I end, Plane<T,3>& plane, const F& getPosition = F()) {
    if (cur == end)
        return false;
    const Vec<T,3> p2 = *cur++;
    if (cur == end)
        return false;
    const Vec<T,3> p0 = *cur++;
    if (cur == end)
        return false;
    const Vec<T,3> p1 = *cur++;
    return setPlanePoints(plane, p0, p1, p2);
}

template <typename T, typename I, typename F = Identity>
T intersectPolygonWithRay(const Ray<T,3>& ray, const Plane<T,3> plane, I cur, I end, const F& getPosition = F()) {
    const T distance = plane.intersectWithRay(ray);
    if (Math::isnan(distance))
        return distance;
    
    const Vec<T,3> point = ray.pointAtDistance(distance);
    if (polygonContainsPoint(point, plane.normal, cur, end, getPosition))
        return distance;
    return Math::nan<T>();
}

template <typename T, typename I, typename F = Identity>
T intersectPolygonWithRay(const Ray<T,3>& ray, I cur, I end, const F& getPosition = F()) {
    Plane<T,3> plane;
    if (!getPlane(cur, end, plane, getPosition))
        return Math::nan<T>();
    return intersectPolygonWithRay(ray, plane, cur, end, getPosition);
}


template <typename T, typename I, typename F = Identity>
bool polygonContainsPoint(const Vec<T,3>& point, I cur, I end, const F& getPosition = F()) {
    I temp = cur;
    
    assert(temp != end); const Vec<T,3> p1 = getPosition(*temp++);
    assert(temp != end); const Vec<T,3> p2 = getPosition(*temp++);
    assert(temp != end); const Vec<T,3> p3 = getPosition(*temp);
    
    Vec<T,3> normal;
    assertResult(planeNormal(normal, p1, p2, p3));
    
    return polygonContainsPoint(point, normal.firstComponent(), cur, end, getPosition);
}

template <typename T, typename I, typename F = Identity>
bool polygonContainsPoint(const Vec<T,3>& point, const Vec<T,3>& normal, I cur, I end, const F& getPosition = F()) {
    return polygonContainsPoint(point, normal.firstComponent(), cur, end, getPosition);
}

template <typename T, typename I, typename F = Identity>
bool polygonContainsPoint(const Vec<T,3>& point, const size_t axis, I cur, I end, const F& getPosition = F()) {
    const Vec<T,3> o = swizzle(point, axis);
    
    const Vec<T,3> f = swizzle(getPosition(*cur++), axis) - o; // The first vertex.
    Vec<T,3> p = f; // The previous vertex.
    
    int d = 0;
    while (cur != end) {
        const Vec<T,3> c = swizzle(getPosition(*cur++), axis) - o; // The current vertex.
        const int s = handlePolygonEdgeIntersection(p, c);
        if (s == -1)
            return true;
        d += s;
        p = c;
    }
    
    // Handle the edge from the last to the first vertex.
    const int s = handlePolygonEdgeIntersection(p, f);
    if (s == -1)
        return true;
    
    d += s;
    if (d % 2 == 0)
        return false;
    return true;
}

template <typename T>
int handlePolygonEdgeIntersection(const Vec<T,3>& v0, const Vec<T,3>& v1) {
    if (Math::zero(v0.x()) && Math::zero(v0.y())) {
        // the point is identical to a polygon vertex, cancel search
        return -1;
    }
    
    /*
     * A polygon edge intersects with the positive X axis if the
     * following conditions are met: The Y coordinates of its
     * vertices must have different signs (we assign a negative sign
     * to 0 here in order to count it as a negative number) and one
     * of the following two conditions must be met: Either the X
     * coordinates of the vertices are both positive or the X
     * coordinates of the edge have different signs (again, we
     * assign a negative sign to 0 here). In the latter case, we
     * must calculate the point of intersection between the edge and
     * the X axis and determine whether its X coordinate is positive
     * or zero.
     */
    
    // Does Y segment covered by the given edge touch the X axis at all?
    if (( Math::pos(v0.y()) &&  Math::pos(v1.y())) ||
        ( Math::neg(v0.y()) &&  Math::neg(v1.y())) ||
        (Math::zero(v0.y()) && Math::zero(v1.y()))) {
        return 0;
    }
    
    
    // Is segment entirely on the positive side of the X axis?
    if (Math::pos(v0.x()) && Math::pos(v1.x())) {
        return 1;
    }
    
    // Is segment entirely on the negative side of the X axis?
    if (Math::neg(v0.x()) && Math::neg(v1.x())) {
        return 0;
    }
    
    // Calculate the point of intersection between the edge and the X axis.
    const T x = -v0.y() * (v1.x() - v0.x()) / (v1.y() - v0.y()) + v0.x();
    
    // Is the point of intersection on the given edge?
    if (Math::zero(x)) {
        return -1;
    }
    
    // Is the point of intersection on the positive X axis?
    if (Math::pos(x)) {
        return 1;
    }
    
    // The point of intersection is on the negative X axis.
    return 0;
}

/*
 Returns > 0 if p3.xy() is to the left of the line through p1.xy() and p2.xy(),
 < 0 if it is to the right of that line, or
 = 0 if it is on the line.
 */
template <typename T, size_t S>
int isLeft(const Vec<T,S>& p1, const Vec<T,S>& p2, const Vec<T,S>& p3) {
    assert(S >= 2);
    const T result = ( (p2.x() - p1.x()) * (p3.y() - p1.y())
                      -(p3.x() - p1.x()) * (p2.y() - p1.y()));
    if (result < 0.0)
        return -1;
    if (result > 0.0)
        return 1;
    return 0;
}


template <typename T>
class ConvexHull2D {
private:
    class LessThanByAngle {
    private:
        const Vec<T,3>& m_anchor;
    public:
        LessThanByAngle(const Vec<T,3>& anchor) : m_anchor(anchor) {}
    public:
        bool operator()(const Vec<T,3>& lhs, const Vec<T,3>& rhs) const {
            const int side = isLeft(m_anchor, lhs, rhs);
            if (side > 0)
                return true;
            if (side < 0)
                return false;
            
            // the points are colinear, the one that is further from the anchor is considered less
            const T dxl = Math::abs(lhs.x() - m_anchor.x());
            const T dxr = Math::abs(rhs.x() - m_anchor.x());
            if (dxl == dxr) {
                const T dyl = Math::abs(lhs.y() - m_anchor.y());
                const T dyr = Math::abs(rhs.y() - m_anchor.y());
                return dyl > dyr;
            }
            return dxl > dxr;
        }
    };
    
    typename Vec<T,3>::List m_points;
    bool m_hasResult;
public:
    ConvexHull2D(const typename Vec<T,3>::List& points) :
    m_points(points),
    m_hasResult(m_points.size() > 2) {
        if (m_hasResult) {
            const size_t thirdPointIndex = findLinearlyIndependentPoint();
            m_hasResult = (thirdPointIndex < m_points.size());
            
            if (m_hasResult) {
                const Math::Axis::Type axis = computeAxis(thirdPointIndex);
                swizzleTo(axis);
                
                findAnchor();
                sortPoints();
                m_hasResult = (m_points.size() > 2);
                if (m_hasResult)
                    buildHull();
                
                swizzleFrom(axis);
            }
        }
    }
    
    bool hasResult() const {
        return m_hasResult;
    }
    
    const typename Vec<T,3>::List& result() const {
        assert(m_hasResult);
        return m_points;
    }
private:
    size_t findLinearlyIndependentPoint() const {
        size_t index = 2;
        while (index < m_points.size() && linearlyDependent(m_points[0], m_points[1], m_points[index]))
            ++index;
        return index;
    }
    
    Math::Axis::Type computeAxis(const size_t thirdPointIndex) const {
        const Vec<T,3> ortho = crossed(m_points[thirdPointIndex] - m_points[0], m_points[1] - m_points[0]);
        return ortho.firstComponent();
    }
    
    void swizzleTo(const Math::Axis::Type axis) {
        for (size_t i = 0; i < m_points.size(); ++i)
            m_points[i] = swizzle(m_points[i], axis);
    }
    
    void swizzleFrom(const Math::Axis::Type axis) {
        swizzleTo(axis);
        swizzleTo(axis);
    }
    
    void findAnchor() {
        size_t anchor = 0;
        for (size_t i = 1; i < m_points.size(); ++i) {
            if ((m_points[i].y() < m_points[anchor].y()) ||
                (m_points[i].y() == m_points[anchor].y() &&
                 m_points[i].x() >  m_points[anchor].x()))
                anchor = i;
        }
        
        if (anchor > 0) {
            using std::swap;
            swap(m_points[0], m_points[anchor]);
        }
    }
    
    void sortPoints() {
        const Vec<T,3>& anchor = m_points[0];
        std::sort(std::begin(m_points) + 1, std::end(m_points), LessThanByAngle(anchor));
        
        // now remove the duplicates
        auto i = std::begin(m_points) + 1;
        while (i != std::end(m_points)) {
            const Vec<T,3>& p1 = *(i++);
            while (i != std::end(m_points)) {
                const Vec<T,3>& p2 = *i;
                if (isLeft(anchor, p1, p2) == 0)
                    i = m_points.erase(i);
                else
                    break;
            };
        }
    }
    
    void buildHull() {
        typename Vec<T,3>::List stack;
        stack.reserve(m_points.size());
        stack.push_back(m_points[0]);
        stack.push_back(m_points[1]);
        
        for (size_t i = 2; i < m_points.size(); ++i) {
            const Vec<T,3>& p = m_points[i];
            popStalePoints(stack, p);
            stack.push_back(p);
        }
        
        using std::swap;
        swap(m_points, stack);
        assert(m_points.size() > 2);
    }
    
    void popStalePoints(typename Vec<T,3>::List& stack, const Vec<T,3>& p) {
        if (stack.size() > 1) {
            const Vec<T,3>& t1 = stack[stack.size() - 2];
            const Vec<T,3>& t2 = stack[stack.size() - 1];
            const int side = isLeft(t1, t2, p);
            if (side < 0) {
                stack.pop_back();
                popStalePoints(stack, p);
            }
        }
    }
};

// see http://geomalgorithms.com/a10-_hull-1.html
template <typename T>
typename Vec<T,3>::List convexHull2D(const typename Vec<T,3>::List& points) {
    const ConvexHull2D<T> hull(points);
    if (!hull.hasResult())
        return Vec<T,3>::EmptyList;
    return hull.result();
}

#endif
