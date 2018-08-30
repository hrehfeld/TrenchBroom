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

#include "Lasso.h"

#include "Renderer/Camera.h"
#include "Renderer/RenderService.h"

namespace TrenchBroom {
    namespace View {
        Lasso::Lasso(const Renderer::Camera& camera, const FloatType distance, const vec3& point) :
        m_camera(camera),
        m_distance(distance),
        m_transform(coordinateSystemMatrix(m_camera.right(), m_camera.up(), -m_camera.direction(),
                                           m_camera.defaultPoint(static_cast<float>(m_distance)))),
        m_start(point),
        m_cur(m_start) {}
        
        void Lasso::update(const vec3& point) {
            m_cur = point;
        }

        bool Lasso::selects(const vec3& point, const Plane3& plane, const BBox2& box) const {
            const auto projected = project(point, plane);
            return !isNaN(projected) && box.contains(vec2(projected));
        }
        
        bool Lasso::selects(const Edge3& edge, const Plane3& plane, const BBox2& box) const {
            return selects(edge.center(), plane, box);
        }
        
        bool Lasso::selects(const Polygon3& polygon, const Plane3& plane, const BBox2& box) const {
            return selects(polygon.center(), plane, box);
        }
        
        vec3 Lasso::project(const vec3& point, const Plane3& plane) const {
            const auto ray = Ray3(m_camera.pickRay(vec3f(point)));
            const auto hitDistance = plane.intersectWithRay(ray);
            if (Math::isnan(hitDistance)) {
                return vec3::NaN;
            }

            const auto hitPoint = ray.pointAtDistance(hitDistance);
            return m_transform * hitPoint;
        }

        void Lasso::render(Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch) const {
            const auto box = this->box();
            const auto [invertible, inverseTransform] = invert(m_transform);
            assert(invertible); unused(invertible);

            vec3f::List polygon(4);
            polygon[0] = vec3f(inverseTransform * vec3(box.min.x(), box.min.y(), 0.0));
            polygon[1] = vec3f(inverseTransform * vec3(box.min.x(), box.max.y(), 0.0));
            polygon[2] = vec3f(inverseTransform * vec3(box.max.x(), box.max.y(), 0.0));
            polygon[3] = vec3f(inverseTransform * vec3(box.max.x(), box.min.y(), 0.0));
            
            Renderer::RenderService renderService(renderContext, renderBatch);
            renderService.setForegroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
            renderService.setLineWidth(2.0f);
            renderService.renderPolygonOutline(polygon);

            renderService.setForegroundColor(Color(1.0f, 1.0f, 1.0f, 0.25f));
            renderService.renderFilledPolygon(polygon);
        }

        Plane3 Lasso::plane() const {
            return Plane3(vec3(m_camera.defaultPoint(static_cast<float>(m_distance))), vec3(m_camera.direction()));
        }
        
        BBox2 Lasso::box() const {
            const auto start = m_transform * m_start;
            const auto cur   = m_transform * m_cur;
            
            const auto min = ::min(start, cur);
            const auto max = ::max(start, cur);
            return BBox2(vec2(min), vec2(max));
        }
    }
}
