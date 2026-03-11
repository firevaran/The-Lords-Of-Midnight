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
    Sprite* GetFloorImage( floor_t floor );
    
    void DrawWaterQuad( DrawNode* drawNode, LandscapeItem* item );
    bool CalcQuadCorners( LandscapeItem* item, Vec2* corners, f32 padding );
    void DrawDebugCheckerboard();
    bool QuadIsValid( Vec2* corners );
    
    SimpleShader* floorShader;
};


class FloorTile : public ax::Sprite
{
public:
    static FloorTile* create(const std::string& frameName)
    {
        auto tile = new (std::nothrow) FloorTile();
        if (tile && tile->initWithSpriteFrameName(frameName))
        {
            tile->autorelease();
            return tile;
        }
        AX_SAFE_DELETE(tile);
        return nullptr;
    }
    
    void setCorners(const ax::Vec2* corners)
    {
        // corners: [0]=bl, [1]=br, [2]=tr, [3]=tl
        _quad.bl.vertices = ax::Vec3(corners[0].x, corners[0].y, 0);
        _quad.br.vertices = ax::Vec3(corners[1].x, corners[1].y, 0);
        _quad.tr.vertices = ax::Vec3(corners[2].x, corners[2].y, 0);
        _quad.tl.vertices = ax::Vec3(corners[3].x, corners[3].y, 0);
    }
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
