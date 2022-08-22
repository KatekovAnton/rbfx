//
// Copyright (c) 2008-2020 the Urho3D project.
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

#pragma once

#include <stdio.h>
#include <EASTL/unordered_map.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Component.h>



namespace Urho3D {
class Material;
class Technique;
}



class ProjectNMetaData : public Urho3D::Component {
    
    URHO3D_OBJECT(ProjectNMetaData, Urho3D::Component);
    
    bool _roofVisible = true;
    ea::vector<Urho3D::SharedPtr<Urho3D::Material>> _visibleMaterials;
    ea::vector<Urho3D::SharedPtr<Urho3D::Material>> _invisibleMaterials;
    
public:
    
    enum ObjectType {
        Door = 1
    };
    
    bool _groundPathClickable;
    bool _clicksDisabled = false;
    bool _isRoof = false;
    int _gameObjectType = -1;
    ea::string _gameObjectId;
    ea::string _gameObjectElementId;
    
    ProjectNMetaData(Urho3D::Context* context);
    ~ProjectNMetaData() override;
    
    bool LoadXML(const Urho3D::XMLElement& source) override;
    
    static void RegisterObject(Urho3D::Context* context);
    
    void setRoofVisible(bool visible, ea::map<Urho3D::SharedPtr<Urho3D::Material>, Urho3D::SharedPtr<Urho3D::Material>> &roofMaterials, ea::map<Urho3D::SharedPtr<Urho3D::Technique>, Urho3D::SharedPtr<Urho3D::Technique>> &roofTechniques);
    
};



struct GameSceneObjectNode {
    Urho3D::Node * _node;
    Urho3D::IntVector2 _position;
};

struct GameSceneObject {
    
    ea::string _objectId;
    int _objectType;
    ea::map<ea::string, GameSceneObjectNode> _nodes;
    
    GameSceneObject(const ea::string &objectId, int objectType);
    
};

class GameSceneData {
    
    void analyseNode(Urho3D::Node *node);
    
public:
    
    ea::vector<Urho3D::Node *> _roofs;
    ea::map<ea::string, std::shared_ptr<GameSceneObject>> _objects;
    
    void analyse(Urho3D::Scene *scene);

};
