/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#include "UVViewOriginTool.h"
#include "PreferenceManager.h"
#include "Preferences.h"
#include "Assets/Texture.h"
#include "Model/BrushFace.h"
#include "Model/BrushVertex.h"
#include "Model/ModelTypes.h"
#include "Renderer/Circle.h"
#include "Renderer/EdgeRenderer.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/Transformation.h"
#include "Renderer/VertexSpec.h"
#include "View/ControllerFacade.h"
#include "View/InputState.h"
#include "View/UVViewHelper.h"

namespace TrenchBroom {
    namespace View {
        const Hit::HitType UVViewOriginTool::XHandleHit = Hit::freeHitType();
        const Hit::HitType UVViewOriginTool::YHandleHit = Hit::freeHitType();
        const FloatType UVViewOriginTool::MaxPickDistance = 5.0;
        const float UVViewOriginTool::OriginHandleRadius =  5.0f;

        UVViewOriginTool::UVViewOriginTool(MapDocumentWPtr document, ControllerWPtr controller, UVViewHelper& helper) :
        ToolImpl(document, controller),
        m_helper(helper) {}

        void UVViewOriginTool::doPick(const InputState& inputState, Hits& hits) {
            if (m_helper.valid()) {
                Line3 xHandle, yHandle;
                computeOriginHandles(xHandle, yHandle);

                const Model::BrushFace* face = m_helper.face();
                const Mat4x4 fromTex = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
                const Vec3 origin = fromTex * Vec3(m_helper.originInFaceCoords());
                
                const Ray3& pickRay = inputState.pickRay();
                const Ray3::PointDistance oDistance = pickRay.distanceToPoint(origin);
                if (oDistance.distance <= 2.0f * OriginHandleRadius / m_helper.cameraZoom()) {
                    const Vec3 hitPoint = pickRay.pointAtDistance(oDistance.rayDistance);
                    hits.addHit(Hit(XHandleHit, oDistance.rayDistance, hitPoint, xHandle, oDistance.distance));
                    hits.addHit(Hit(YHandleHit, oDistance.rayDistance, hitPoint, xHandle, oDistance.distance));
                } else {
                    const Ray3::LineDistance xDistance = pickRay.distanceToLine(xHandle.point, xHandle.direction);
                    const Ray3::LineDistance yDistance = pickRay.distanceToLine(yHandle.point, yHandle.direction);
                    
                    assert(!xDistance.parallel);
                    assert(!yDistance.parallel);
                    
                    const FloatType maxDistance  = MaxPickDistance / m_helper.cameraZoom();
                    if (xDistance.distance <= maxDistance) {
                        const Vec3 hitPoint = pickRay.pointAtDistance(xDistance.rayDistance);
                        hits.addHit(Hit(XHandleHit, xDistance.rayDistance, hitPoint, xHandle, xDistance.distance));
                    }
                    
                    if (yDistance.distance <= maxDistance) {
                        const Vec3 hitPoint = pickRay.pointAtDistance(yDistance.rayDistance);
                        hits.addHit(Hit(YHandleHit, yDistance.rayDistance, hitPoint, yHandle, yDistance.distance));
                    }
                }
            }
        }

        void UVViewOriginTool::computeOriginHandles(Line3& xHandle, Line3& yHandle) const {
            const Model::BrushFace* face = m_helper.face();
            const Mat4x4 toWorld = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
            
            const Vec3 origin = m_helper.originInFaceCoords();
            xHandle.point = yHandle.point = toWorld * origin;
            
            xHandle.direction = (toWorld * (origin + Vec3::PosY) - xHandle.point).normalized();
            yHandle.direction = (toWorld * (origin + Vec3::PosX) - yHandle.point);
        }

        bool UVViewOriginTool::doStartMouseDrag(const InputState& inputState) {
            assert(m_helper.valid());
            
            if (!inputState.modifierKeysPressed(ModifierKeys::MKNone) ||
                !inputState.mouseButtonsPressed(MouseButtons::MBLeft))
                return false;
            
            const Hits& hits = inputState.hits();
            const Hit& xHandleHit = hits.findFirst(XHandleHit, true);
            const Hit& yHandleHit = hits.findFirst(YHandleHit, true);

            if (!xHandleHit.isMatch() && !yHandleHit.isMatch())
                return false;
            
            if (xHandleHit.isMatch())
                m_selector[0] = 1.0f;
            else
                m_selector[0] = 0.0f;
            
            if (yHandleHit.isMatch())
                m_selector[1] = 1.0f;
            else
                m_selector[1] = 0.0f;
            
            m_lastPoint = computeHitPoint(inputState.pickRay());
            return true;
        }
        
        bool UVViewOriginTool::doMouseDrag(const InputState& inputState) {
            const Vec2f curPoint = computeHitPoint(inputState.pickRay());
            const Vec2f delta = curPoint - m_lastPoint;
            
            const Vec2f snapped = snapDelta(delta * m_selector);
            if (snapped.null())
                return true;
            
            m_helper.setOrigin(m_helper.originInFaceCoords() + snapped);
            m_lastPoint += snapped;
            
            return true;
        }
        
        Vec2f UVViewOriginTool::computeHitPoint(const Ray3& ray) const {
            const Model::BrushFace* face = m_helper.face();
            const Plane3& boundary = face->boundary();
            const FloatType distance = boundary.intersectWithRay(ray);
            const Vec3 hitPoint = ray.pointAtDistance(distance);
            
            const Mat4x4 transform = face->toTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
            return Vec2f(transform * hitPoint);
        }

        Vec2f UVViewOriginTool::snapDelta(const Vec2f& delta) const {
            if (delta.null())
                return delta;
            
            const Model::BrushFace* face = m_helper.face();
            assert(face != NULL);
            
            // The delta is given in non-translated and non-scaled texture coordinates because that's how the origin
            // is stored. We have to convert to translated and scaled texture coordinates to do our snapping because
            // that's how the helper computes the distance to the texture grid.
            // Finally, we will convert the distance back to non-translated and non-scaled texture coordinates and
            // snap the delta to the distance.
            
            const Mat4x4 w2fTransform = face->toTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
            const Mat4x4 w2tTransform = face->toTexCoordSystemMatrix(face->offset(), face->scale(), true);
            const Mat4x4 f2wTransform = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
            const Mat4x4 t2wTransform = face->fromTexCoordSystemMatrix(face->offset(), face->scale(), true);
            const Mat4x4 f2tTransform = w2tTransform * f2wTransform;
            const Mat4x4 t2fTransform = w2fTransform * t2wTransform;
            
            const Vec2f newOriginInFaceCoords = m_helper.originInFaceCoords() + delta;
            const Vec2f newOriginInTexCoords  = Vec2f(f2tTransform * Vec3(newOriginInFaceCoords));
            
            // now snap to the vertices
            // TODO: this actually doesn't work because we're snapping to the X or Y coordinate of the vertices
            // instead, we must snap to the edges!
            const Model::BrushVertexList& vertices = face->vertices();
            Vec2f distanceInTexCoords = Vec2f::Max;
            
            for (size_t i = 0; i < vertices.size(); ++i)
                distanceInTexCoords = absMin(distanceInTexCoords, newOriginInTexCoords - Vec2f(w2tTransform * vertices[i]->position));
            
            // and to the texture grid
            const Assets::Texture* texture = face->texture();
            if (texture != NULL)
                distanceInTexCoords = absMin(distanceInTexCoords, m_helper.computeDistanceFromTextureGrid(Vec3(newOriginInTexCoords)));
            
            // finally snap to the face center
            const Vec2f faceCenter(w2tTransform * face->boundsCenter());
            distanceInTexCoords = absMin(distanceInTexCoords, newOriginInTexCoords - faceCenter);

            // now we have a distance in the scaled and translated texture coordinate system
            // so we transform the new position plus distance back to the unscaled and untranslated texture coordinate system
            // and take the actual distance
            const Vec2f distanceInFaceCoords = newOriginInFaceCoords - Vec2f(t2fTransform * Vec3(newOriginInTexCoords + distanceInTexCoords));
            return m_helper.snapDelta(delta, distanceInFaceCoords);
        }

        void UVViewOriginTool::doEndMouseDrag(const InputState& inputState) {}
        void UVViewOriginTool::doCancelMouseDrag(const InputState& inputState) {}
        
        void UVViewOriginTool::doRender(const InputState& inputState, Renderer::RenderContext& renderContext) {
            if (!m_helper.valid())
                return;

            glDisable(GL_DEPTH_TEST);
            renderLineHandles(inputState, renderContext);
            renderOriginHandle(inputState, renderContext);
            glEnable(GL_DEPTH_TEST);
        }

        void UVViewOriginTool::renderLineHandles(const InputState& inputState, Renderer::RenderContext& renderContext) {
            EdgeVertex::List vertices = getHandleVertices(inputState.hits());
            
            glLineWidth(2.0f);
            Renderer::EdgeRenderer edgeRenderer(Renderer::VertexArray::swap(GL_LINES, vertices));
            edgeRenderer.render(renderContext);
            glLineWidth(1.0f);
        }

        UVViewOriginTool::EdgeVertex::List UVViewOriginTool::getHandleVertices(const Hits& hits) const {
            const Hit& xHandleHit = hits.findFirst(XHandleHit, true);
            const Hit& yHandleHit = hits.findFirst(YHandleHit, true);
            
            const bool highlightXHandle = (dragging() && m_selector.x() > 0.0) || (!dragging() && xHandleHit.isMatch());
            const bool highlightYHandle = (dragging() && m_selector.y() > 0.0) || (!dragging() && yHandleHit.isMatch());
            
            const Color xColor = highlightXHandle ? Color(1.0f, 0.0f, 0.0f, 1.0f) : Color(0.7f, 0.0f, 0.0f, 1.0f);
            const Color yColor = highlightYHandle ? Color(1.0f, 0.0f, 0.0f, 1.0f) : Color(0.7f, 0.0f, 0.0f, 1.0f);
            
            Vec3 x1, x2, y1, y2;
            m_helper.computeOriginHandleVertices(x1, x2, y1, y2);

            EdgeVertex::List vertices(4);
            vertices[0] = EdgeVertex(Vec3f(x1), xColor);
            vertices[1] = EdgeVertex(Vec3f(x2), xColor);
            vertices[2] = EdgeVertex(Vec3f(y1), yColor);
            vertices[3] = EdgeVertex(Vec3f(y2), yColor);
            return vertices;
        }

        void UVViewOriginTool::renderOriginHandle(const InputState& inputState, Renderer::RenderContext& renderContext) {
            const Hits& hits = inputState.hits();
            const Hit& xHandleHit = hits.findFirst(XHandleHit, true);
            const Hit& yHandleHit = hits.findFirst(YHandleHit, true);
            
            const bool highlight = xHandleHit.isMatch() && yHandleHit.isMatch();;
            
            const PreferenceManager& prefs = PreferenceManager::instance();
            const Color& handleColor = prefs.get(Preferences::HandleColor);
            const Color& highlightColor = prefs.get(Preferences::SelectedHandleColor);
            const float cameraZoom = m_helper.cameraZoom();
            
            const Model::BrushFace* face = m_helper.face();
            const Mat4x4 fromFace = face->fromTexCoordSystemMatrix(Vec2f::Null, Vec2f::One, true);
            
            const Plane3& boundary = face->boundary();
            const Mat4x4 toPlane = planeProjectionMatrix(boundary.distance, boundary.normal);
            const Mat4x4 fromPlane = invertedMatrix(toPlane);
            
            const Vec2f originPosition(toPlane * fromFace * Vec3(m_helper.originInFaceCoords()));
            
            Renderer::Vbo vbo(0xFFF);
            Renderer::SetVboState vboState(vbo);
            Renderer::Circle originHandle(OriginHandleRadius / cameraZoom, 16, true);
            Renderer::Circle highlightHandle(OriginHandleRadius / cameraZoom * 2.0f, 32, false);
            
            vboState.mapped();
            originHandle.prepare(vbo);
            highlightHandle.prepare(vbo);
            vboState.active();
            
            Renderer::ActiveShader shader(renderContext.shaderManager(), Renderer::Shaders::VaryingPUniformCShader);
            const Renderer::MultiplyModelMatrix toWorldTransform(renderContext.transformation(), fromPlane);
            {
                const Mat4x4 translation = translationMatrix(Vec3(originPosition));
                const Renderer::MultiplyModelMatrix centerTransform(renderContext.transformation(), translation);
                shader.set("Color", handleColor);
                originHandle.render();
                
                if (highlight) {
                    shader.set("Color", highlightColor);
                    highlightHandle.render();
                }
            }
        }
    }
}
