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

#include "PerspectiveCamera.h"

#include "Color.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Shaders.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/Vbo.h"
#include "Renderer/VertexArray.h"
#include "Renderer/VertexSpec.h"

namespace TrenchBroom {
    namespace Renderer {
        PerspectiveCamera::PerspectiveCamera() :
        Camera(),
        m_fov(90.0) {}
        
        PerspectiveCamera::PerspectiveCamera(const float fov, const float nearPlane, const float farPlane, const Viewport& viewport, const vec3f& position, const vec3f& direction, const vec3f& up)
        : Camera(nearPlane, farPlane, viewport, position, direction, up),
        m_fov(fov) {
            assert(m_fov > 0.0);
        }
        
        float PerspectiveCamera::fov() const {
            return m_fov;
        }
        
        void PerspectiveCamera::setFov(const float fov) {
            assert(fov > 0.0f);
            if (fov == m_fov)
                return;
            m_fov = fov;
            m_valid = false;
            cameraDidChangeNotifier(this);
        }
        
        Ray3f PerspectiveCamera::doGetPickRay(const vec3f& point) const {
            const vec3f direction = normalize(point - position());
            return Ray3f(position(), direction);
        }
        
        Camera::ProjectionType PerspectiveCamera::doGetProjectionType() const {
            return Projection_Perspective;
        }

        void PerspectiveCamera::doValidateMatrices(mat4x4f& projectionMatrix, mat4x4f& viewMatrix) const {
            const Viewport& viewport = unzoomedViewport();
            projectionMatrix = perspectiveMatrix(fov(), nearPlane(), farPlane(), viewport.width, viewport.height);
            viewMatrix = ::viewMatrix(direction(), up()) * translationMatrix(-position());
        }
        
        void PerspectiveCamera::doComputeFrustumPlanes(plane3f& topPlane, plane3f& rightPlane, plane3f& bottomPlane, plane3f& leftPlane) const {
            const vec2f frustum = getFrustum();
            const vec3f center = position() + direction() * nearPlane();
            
            vec3f d = center + up() * frustum.y() - position();
            topPlane = plane3f(position(), normalize(cross(right(), d)));
            
            d = center + right() * frustum.x() - position();
            rightPlane = plane3f(position(), normalize(cross(d, up())));
            
            d = center - up() * frustum.y() - position();
            bottomPlane = plane3f(position(), normalize(cross(d, right())));
            
            d = center - right() * frustum.x() - position();
            leftPlane = plane3f(position(), normalize(cross(up(), d)));
        }
        
        void PerspectiveCamera::doRenderFrustum(RenderContext& renderContext, Vbo& vbo, const float size, const Color& color) const {
            typedef VertexSpecs::P3C4::Vertex Vertex;
            Vertex::List triangleVertices(6);
            Vertex::List lineVertices(8 * 2);
            
            vec3f verts[4];
            getFrustumVertices(size, verts);
            
            triangleVertices[0] = Vertex(position(), Color(color, 0.7f));
            for (size_t i = 0; i < 4; ++i)
                triangleVertices[i + 1] = Vertex(verts[i], Color(color, 0.2f));
            triangleVertices[5] = Vertex(verts[0], Color(color, 0.2f));
            
            for (size_t i = 0; i < 4; ++i) {
                lineVertices[2 * i + 0] = Vertex(position(), color);
                lineVertices[2 * i + 1] = Vertex(verts[i], color);
            }
            
            for (size_t i = 0; i < 4; ++i) {
                lineVertices[8 + 2 * i + 0] = Vertex(verts[i], color);
                lineVertices[8 + 2 * i + 1] = Vertex(verts[Math::succ(i, 4)], color);
            }
            
            VertexArray triangleArray = VertexArray::ref(triangleVertices);
            VertexArray lineArray = VertexArray::ref(lineVertices);
            
            ActivateVbo activate(vbo);
            triangleArray.prepare(vbo);
            lineArray.prepare(vbo);
            
            ActiveShader shader(renderContext.shaderManager(), Shaders::VaryingPCShader);
            triangleArray.render(GL_TRIANGLE_FAN);
            lineArray.render(GL_LINES);
        }
        
        float PerspectiveCamera::doPickFrustum(const float size, const Ray3f& ray) const {
            vec3f verts[4];
            getFrustumVertices(size, verts);
            
            float distance = Math::nan<float>();
            for (size_t i = 0; i < 4; ++i)
                distance = Math::selectMin(distance, intersectRayWithTriangle(ray, position(), verts[i], verts[Math::succ(i, 4)]));
            return distance;
        }

        void PerspectiveCamera::getFrustumVertices(const float size, vec3f (&verts)[4]) const {
            const vec2f frustum = getFrustum();
            
            verts[0] = position() + (direction() * nearPlane() + frustum.y() * up() - frustum.x() * right()) / nearPlane() * size; // top left
            verts[1] = position() + (direction() * nearPlane() + frustum.y() * up() + frustum.x() * right()) / nearPlane() * size; // top right
            verts[2] = position() + (direction() * nearPlane() - frustum.y() * up() + frustum.x() * right()) / nearPlane() * size; // bottom right
            verts[3] = position() + (direction() * nearPlane() - frustum.y() * up() - frustum.x() * right()) / nearPlane() * size; // bottom left
        }

        vec2f PerspectiveCamera::getFrustum() const {
            const Viewport& viewport = unzoomedViewport();
            const float v = std::tan(Math::radians(fov()) / 2.0f) * 0.75f * nearPlane();
            const float h = v * static_cast<float>(viewport.width) / static_cast<float>(viewport.height);
            return vec2f(h, v);
        }

        float PerspectiveCamera::doGetPerspectiveScalingFactor(const vec3f& position) const {
            const float perpDist = perpendicularDistanceTo(position);
            return perpDist / viewportFrustumDistance();
        }

        float PerspectiveCamera::viewportFrustumDistance() const {
            const float height = static_cast<float>(unzoomedViewport().height);
            return (height / 2.0f) / std::tan(Math::radians(m_fov) / 2.0f);
        }
    }
}
