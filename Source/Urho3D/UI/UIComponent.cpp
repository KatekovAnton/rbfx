//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/BillboardSet.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Material.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Camera.h"
#include "../Graphics/VertexBuffer.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Resource/ResourceCache.h"
#include "../IO/Log.h"
#include "../UI/UI.h"
#include "../UI/UIComponent.h"
#include "../UI/UIEvents.h"

#include "../DebugNew.h"

namespace Urho3D
{

static const int UICOMPONENT_DEFAULT_TEXTURE_SIZE = 512;
static const int UICOMPONENT_MIN_TEXTURE_SIZE = 64;
static const int UICOMPONENT_MAX_TEXTURE_SIZE = 4096;

class UIElement3D : public UIElement
{
    URHO3D_OBJECT(UIElement3D, UIElement);
public:
    /// Construct.
    explicit UIElement3D(Context* context) : UIElement(context) { }
    /// Destruct.
    ~UIElement3D() override = default;
    /// Set UIComponent which is using this element as root element.
    void SetNode(Node* node) { node_ = node; }
    /// Set active viewport through which this element is rendered. If viewport is not set, it defaults to first viewport.
    void SetViewport(Viewport* viewport) { viewport_ = viewport; }
    /// Convert element coordinates to screen coordinates.
    IntVector2 ElementToScreen(const IntVector2& position) override
    {
        URHO3D_LOGERROR("UIElement3D::ElementToScreen is not implemented.");
        return {-1, -1};
    }
    /// Convert screen coordinates to element coordinates.
    IntVector2 ScreenToElement(const IntVector2& screenPos) override
    {
        IntVector2 result(-1, -1);

        if (node_.Expired())
            return result;

        Scene* scene = node_->GetScene();
        auto* model = node_->GetComponent<StaticModel>();
        if (scene == nullptr || model == nullptr)
            return result;

        auto* renderer = GetSubsystem<Renderer>();
        if (renderer == nullptr)
            return result;

        // \todo Always uses the first viewport, in case there are multiple
        auto* octree = scene->GetComponent<Octree>();
        if (viewport_ == nullptr)
            viewport_ = renderer->GetViewportForScene(scene, 0);

        if (viewport_.Expired() || octree == nullptr)
            return result;

        if (viewport_->GetScene() != scene)
        {
            URHO3D_LOGERROR("UIComponent and Viewport set to component's root element belong to different scenes.");
            return result;
        }

        Camera* camera = viewport_->GetCamera();

        if (camera == nullptr)
            return result;

        IntRect rect = viewport_->GetRect();
        if (rect == IntRect::ZERO)
        {
            auto* graphics = GetSubsystem<Graphics>();
            rect.right_ = graphics->GetWidth();
            rect.bottom_ = graphics->GetHeight();
        }

        auto* ui = GetSubsystem<UI>();

        // Convert to system mouse position
        IntVector2 pos;
        pos = ui->ConvertUIToSystem(screenPos);

        Ray ray(camera->GetScreenRay((float)pos.x_ / rect.Width(), (float)pos.y_ / rect.Height()));
        ea::vector<RayQueryResult> queryResultVector;
        RayOctreeQuery query(queryResultVector, ray, RAY_TRIANGLE_UV, M_INFINITY, DRAWABLE_GEOMETRY, DEFAULT_VIEWMASK);

        octree->Raycast(query);

        if (queryResultVector.empty())
            return result;

        for (unsigned i = 0; i < queryResultVector.size(); i++)
        {
            RayQueryResult& queryResult = queryResultVector[i];
            if (queryResult.drawable_ != model)
            {
                // ignore billboard sets by default
                if (queryResult.drawable_->GetTypeInfo()->IsTypeOf(BillboardSet::GetTypeStatic()))
                    continue;
                return result;
            }

            Vector2& uv = queryResult.textureUV_;
            result = IntVector2(static_cast<int>(uv.x_ * GetWidth()),
                static_cast<int>(uv.y_ * GetHeight()));

            // Convert back to scaled UI position
            result = ui->ConvertSystemToUI(result);

            return result;
        }
        return result;
    }

protected:
    /// A UIComponent which owns this element.
    WeakPtr<Node> node_;
    /// Viewport which renders this element.
    WeakPtr<Viewport> viewport_;
};

UIComponent::UIComponent(Context* context)
    : Component(context),
    viewportIndex_(0)
{
    texture_ = MakeShared<Texture2D>(context_);
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);                                        // No mipmaps
    if (texture_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE, Graphics::GetRGBAFormat(),
        TEXTURE_RENDERTARGET))
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_MANUALUPDATE);
    else
        URHO3D_LOGERROR("Resizing of UI rendertarget texture failed.");

    rootElement_ = MakeShared<UIElement3D>(context_);
    rootElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootElement_->SetEnabled(true);
    rootElement_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE);

    rootModalElement_ = MakeShared<UIElement3D>(context_);
    rootModalElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootModalElement_->SetEnabled(true);
    rootModalElement_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE);

    offScreenUI_ = new UI(context_);
    offScreenUI_->SetRoot(rootElement_);
    offScreenUI_->SetRootModalElement(rootModalElement_);
    offScreenUI_->SetRenderTarget(texture_);

    material_ = MakeShared<Material>(context_);
    material_->SetTechnique(0, GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/Diff.xml"));
    material_->SetTexture(TU_DIFFUSE, texture_);
}

UIComponent::~UIComponent() = default;

void UIComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<UIComponent>();
    context->AddFactoryReflection<UIElement3D>();
}

UIElement* UIComponent::GetRoot() const
{
    return rootElement_;
}

Material* UIComponent::GetMaterial() const
{
    return material_;
}

Texture2D* UIComponent::GetTexture() const
{
    return texture_;
}

void UIComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    rootElement_->SetNode(node_);
    if (node_)
    {
        auto* renderer = GetSubsystem<Renderer>();
        auto* model = node_->GetComponent<StaticModel>();
        rootElement_->SetViewport(renderer->GetViewportForScene(GetScene(), viewportIndex_));
        if (model == nullptr)
            model_ = model = node_->CreateComponent<StaticModel>();
        model->SetMaterial(material_);
    }
    else
    {
        if (model_)
        {
            model_->Remove();
            model_ = nullptr;
        }
    }
}

void UIComponent::SetViewportIndex(unsigned int index)
{
    viewportIndex_ = index;
    if (Scene* scene = GetScene())
    {
        auto* renderer = GetSubsystem<Renderer>();
        Viewport* viewport = renderer->GetViewportForScene(scene, index);
        rootElement_->SetViewport(viewport);
    }
}

}
