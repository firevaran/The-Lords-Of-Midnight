//
//  LandscapeLand.hpp
//  citadel
//
//  Created by Chris Wild on 22/08/2017.
//
//

#ifndef LandscapeLand_hpp
#define LandscapeLand_hpp

#include "LandscapeNode.h"
#include "LandscapeGenerator.h"
#include "../system/moonring.h"

FORWARD_REFERENCE(SimpleShader);

class LandscapeLand : public LandscapeNode
{
    using Sprite = ax::Sprite;
    using DrawNode = ax::DrawNode;
    using Vec2 = ax::Vec2;
public:
    static LandscapeLand* create( LandscapeOptions* options );

    void Build() override;
    void RefreshPositions() override;

protected:
    bool initWithOptions( LandscapeOptions* options );
    
    bool CalcQuadCorners( LandscapeItem* item, Vec2* corners);
    void DrawFloorTiles();
    bool QuadIsValid( Vec2* corners );
    
    SimpleShader* floorShader;
};


class FloorTile : public ax::Node
{
    using V3F_C4B_T2F = ax::V3F_C4B_T2F;
    using Tex2F       = ax::Tex2F;
    using Color4B     = ax::Color4B;

public:
    static FloorTile* create(const std::string& frameName, int rotation, const ax::Color4F& colour)
    {
        auto tile = new (std::nothrow) FloorTile();
        if (tile && tile->init(frameName, rotation, colour))
        {
            tile->autorelease();
            return tile;
        }
        AX_SAFE_DELETE(tile);
        return nullptr;
    }

    void setCorners(const ax::Vec2* corners)
    {
        // corners: [0]=left-far, [1]=right-far, [2]=right-near, [3]=left-near
        _verts[0].vertices = ax::Vec3(corners[0].x, corners[0].y, 0); // tl
        _verts[1].vertices = ax::Vec3(corners[1].x, corners[1].y, 0); // tr
        _verts[2].vertices = ax::Vec3(corners[2].x, corners[2].y, 0); // br
        _verts[3].vertices = ax::Vec3(corners[3].x, corners[3].y, 0); // bl
    }

void draw(ax::Renderer* renderer, const ax::Mat4& transform, uint32_t flags) override
{
    _triangles.verts      = _verts;
    _triangles.vertCount  = 4;
    _triangles.indices    = _indices;
    _triangles.indexCount = 6;

    const auto& matrixP = ax::Director::getInstance()->getMatrix(ax::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    ax::Mat4 matrixMVP  = matrixP * transform;
    auto mvpLocation    = _programState->getUniformLocation("u_MVPMatrix");
    _programState->setUniform(mvpLocation, matrixMVP.m, sizeof(matrixMVP.m));

    _cmd.init(
        0,
        _texture,
        ax::BlendFunc::ALPHA_PREMULTIPLIED,
        _triangles,
        transform,
        flags
    );
    renderer->addCommand(&_cmd);
}

protected:
    bool init(const std::string& frameName, int rotation, const ax::Color4F& colour)
    {
        if (!Node::init())
            return false;

        auto frame = ax::SpriteFrameCache::getInstance()->getSpriteFrameByName(frameName);
        if (frame == nullptr) {
            UIDEBUG("FloorTile: frame not found: %s", frameName.c_str());
            return false;
        }

        _texture = frame->getTexture();
        if (_texture == nullptr) {
            UIDEBUG("FloorTile: texture is null for frame: %s", frameName.c_str());
            return false;
        }
    
        auto rect = frame->getRectInPixels();
        auto texSize = _texture->getContentSizeInPixels();

        float u0 = rect.origin.x    / texSize.width;
        float v0 = rect.origin.y    / texSize.height;
        float u1 = (rect.origin.x + rect.size.width)  / texSize.width;
        float v1 = (rect.origin.y + rect.size.height) / texSize.height;

        Color4B c(colour);

        // UVs for each corner: tl, tr, br, bl
        Tex2F uvBase[4] = {
            Tex2F(u0,v0), // tl
            Tex2F(u1,v0), // tr
            Tex2F(u1,v1), // br
            Tex2F(u0,v1), // bl
        };
    

        // _verts[0] = { ax::Vec3(0,0,0), c, Tex2F(u0, v0) }; // tl
        // _verts[1] = { ax::Vec3(0,0,0), c, Tex2F(u1, v0) }; // tr
        // _verts[2] = { ax::Vec3(0,0,0), c, Tex2F(u1, v1) }; // br
        // _verts[3] = { ax::Vec3(0,0,0), c, Tex2F(u0, v1) }; // bl

        // rotate clockwise by shifting UV index
        for (int ii = 0; ii < 4; ii++) {
            _verts[ii] = { ax::Vec3(0,0,0), c, uvBase[(ii + rotation) % 4] };
        }


        auto program = ax::ProgramManager::getInstance()->getBuiltinProgram(
            ax::ProgramType::POSITION_TEXTURE_COLOR
        );
        _programState = new ax::backend::ProgramState(program);

        // bind the texture to the program state
        auto textureLocation = _programState->getUniformLocation("u_texture0");
        _programState->setTexture(textureLocation, 0, _texture->getBackendTexture());

        _cmd.getPipelineDescriptor().programState = _programState;


        return true;
    }

private:
    ax::Texture2D*                  _texture  = nullptr;
    ax::backend::ProgramState*      _programState = nullptr;
    ax::TrianglesCommand            _cmd;
    ax::TrianglesCommand::Triangles _triangles;
    V3F_C4B_T2F                     _verts[4];
    unsigned short                  _indices[6] = { 0, 1, 2, 0, 2, 3 };
};


#endif /* LandscapeLand_hpp */

/*
    
    // Pano = 5062
    // points = 0 - 1080 - 2160
    if (!QuadIsValid(corners)) {
        //if (item->id == 162) {
             UIDEBUG("BEFORE ID: %d, loc(x=%.2f, y=%.2f), h=%.2f, p=%.2f, mx=%d, [0]=%.4f,%.4f, [1]=%.4f,%.4f, [2]=%.4f,%.4f, [3]=%.4f,%.4ff",
                item->id, gx, gy, h, p,
                x,
                corners[0].x, corners[0].y,
                corners[1].x, corners[1].y,
                corners[2].x, corners[2].y,
                corners[3].x, corners[3].y
            );
        //}
 */
