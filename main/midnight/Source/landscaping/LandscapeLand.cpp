//
//  LandscapeLand.cpp
//  citadel
//
//  Created by Chris Wild on 22/08/2017.
//

#include "LandscapeView.h"
#include "LandscapeGenerator.h"
#include "LandscapeNode.h"
#include "LandscapeSky.h"
#include "LandscapeLand.h"
#include "LandscapeTerrain.h"
#include "LandscapeDebug.h"
#include "LandscapeColour.h"
#include "../system/moonring.h"
#include "../system/shadermanager.h"
#include "../ui/uihelper.h"

USING_NS_AX;

// -----------------------------------------------------------------------------
// Tuning constants
// -----------------------------------------------------------------------------
constexpr std::string FLOOR_TILE_NAME = "floortiles";

static const Color4F palette[] = {
    Color4F(0.5f, 0.5f, 0.5f, 1.0f),   // floor_none   — mid grey
    Color4F(0.3f, 0.6f, 0.3f, 1.0f),   // floor_normal — green
    Color4F(0.9f, 0.9f, 1.0f, 1.0f),   // floor_snow   — pale blue-white
    Color4F(0.0f, 0.3f, 1.0f, 1.0f),   // floor_river  — bright blue
    Color4F(0.0f, 0.1f, 0.8f, 1.0f),   // floor_sea    — dark blue
    Color4F(0.0f, 0.0f, 0.6f, 1.0f),   // floor_lake   — navy
    Color4F(1.0f, 1.0f, 0.0f, 1.0f),   // floor_debug  — yellow
};


// -----------------------------------------------------------------------------

LandscapeLand* LandscapeLand::create( LandscapeOptions* options )
{
    LandscapeLand* node = new (std::nothrow) LandscapeLand();
    if (node && node->initWithOptions(options))
    {
        node->autorelease();
        return node;
    }
    AX_SAFE_DELETE(node);
    return nullptr;
}

bool LandscapeLand::initWithOptions( LandscapeOptions* options )
{
    if ( !LandscapeNode::initWithOptions(options) )
        return false;

    auto floor = Sprite::createWithSpriteFrameName( "floor" );
    addChild(floor);
    floor->setPosition(Vec2::ZERO);
    floor->setAnchorPoint(Vec2::ZERO);
    floor->setName("floor");
    if ( options->terrainTimeShader )
        options->colour->updateTerrainNode(floor);

    if ( options->debugLand )
        floor->setColor(Color3B::YELLOW);

    // floorShader = mr->shader->GetTerrainTimeShader();

    auto container = Node::create();
    container->setName(FLOOR_TILE_NAME);
    container->setLocalZOrder(10);
    addChild(container);

    return true;
}

// -----------------------------------------------------------------------------
// Build
// -----------------------------------------------------------------------------

void LandscapeLand::Build()
{
    auto items = options->generator->items;

    // Scale the base floor sprite to fill the full width of the node.
    auto floor = getChildByName("floor");
    float scalex = getContentSize().width / floor->getContentSize().width;
    floor->setScale(scalex, 1.0f);

    // -------------------------------------------------------------------------
    // Debug checkerboard — draws all tiles as projected quads, then returns.
    // -------------------------------------------------------------------------
    if ( options->showWater ) {
        DrawFloorTiles();
    }

    // -------------------------------------------------------------------------
    // Draw current location floor (full-width, moves with player)
    // -------------------------------------------------------------------------
//    if ( options->showLand ) {
//        // f32  movementAmount = options->isMoving ? options->movementAmount : 1.0f;
//        auto visibleSize    = Director::getInstance()->getVisibleSize();
//
//        auto here = Sprite::createWithSpriteFrameName("floor_mask0");
//        if ( here == nullptr )
//            return nullptr;
// 
//        LandscapeItem* found = nullptr;
//        for (auto const& item : *items) {
//            if (item->current) { found = item; break;}
//        }
//        if (found == nullptr) return;
//
//        if (found->floor == floor_lake || found->floor == floor_river) {
//            auto c = palette[(int)found->floor];
//            here->setColor(Color3B(c));
//        } else {
//            if ( options->terrainTimeShader )
//                options->colour->updateTerrainNode(here);
//        }
//
//        here->setPosition(Vec2(visibleSize.width / 2, 0));
//        here->setScale( (visibleSize.width/here->getContentSize().width)+0.5f, 2.5f);
//        here->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
//        here->setLocalZOrder(11);
// 
//        addChild(here);
//    }

}

// -----------------------------------------------------------------------------
// CalcQuadCorners
// Projects the 4 ground-plane corners of a tile, normalises X, clamps Y.
// Returns false if the quad should be culled.
// -----------------------------------------------------------------------------
bool LandscapeLand::CalcQuadCorners( LandscapeItem* item, Vec2* corners)
{
    if (!item->quadValid) {
        // UIDEBUG("ID: %d - not drawn", item->id);
        return false;
    }

    f32 h        = getContentSize().height;
    f32 sw       = getContentSize().width;
    f32 panorama = options->generator->PanoramaWidth;
    f32 offset   = LRES(options->generator->horizontalOffset);


    for (int ii = 0; ii < 4; ii++) {
        corners[ii].x = item->quadCorners[ii].x - offset;
        corners[ii].y = h - item->quadCorners[ii].y;
    }

    // Shift all corners together if the group is off-screen
    f32 minX = corners[0].x, maxX = corners[0].x;
    for (int ii = 1; ii < 4; ii++) {
        minX = std::min(minX, corners[ii].x);
        maxX = std::max(maxX, corners[ii].x);
    }

    if (maxX < 0) {
        for (int ii = 0; ii < 4; ii++) corners[ii].x += panorama;
    } else if (minX > sw) {
        for (int ii = 0; ii < 4; ii++) corners[ii].x -= panorama;
    }

    if (!QuadIsValid(corners) ) {
//         UIDEBUG("Invalid ID: %d, [0]=%.4f,%.4f, [1]=%.4f,%.4f, [2]=%.4f,%.4f, [3]=%.4f,%.4ff",
//            item->id,
//            corners[0].x, corners[0].y,
//            corners[1].x, corners[1].y,
//            corners[2].x, corners[2].y,
//            corners[3].x, corners[3].y
//        );
        return false;
    }
    
    return true;
}


// -----------------------------------------------------------------------------
// QuadIsValid
// Only rejects torn wraparound quads. Renderer clips everything else.
// -----------------------------------------------------------------------------

bool LandscapeLand::QuadIsValid( Vec2* corners )
{
    // Only reject torn wraparound quads — no real tile should be this wide.
    // The renderer will clip anything outside the viewport automatically.
    f32 minX = corners[0].x, maxX = corners[0].x;
    for (int ii = 1; ii < 4; ii++) {
        minX = std::min(minX, corners[ii].x);
        maxX = std::max(maxX, corners[ii].x);
    }
    return (maxX - minX) < (options->generator->PanoramaWidth * 0.26f);
}

void LandscapeLand::DrawFloorTiles()
{
    static const char* tile_names[] = {
        "land__0001_Isolated",  // none  - 4 corners
        "land__0004_Open",      // solid - 0 corners
        "land__0002_Edge",      // edge  - 2 corners
        "land__0004_Open",      // solid - 0 corners
    };
    
    auto items = options->generator->items;
    auto container = getChildByName(FLOOR_TILE_NAME);

    for (auto const& item : *items) {
        if ( !item->quadValid && !item->ahead )
            continue;
            
        if ( item->floor == floor_none || item->floor == floor_normal )
            continue;
            
        if ( item->tile == nullptr )
            continue;

        if ( item->position.z < options->generator->viewportNear ||
             item->position.z >= options->generator->viewportFar )
            continue;

        auto c = palette[(int)item->floor];
        auto tile = FloorTile::create(tile_names[(int)item->tile->type], item->tile->rotation, c);
        if ( tile == nullptr )
            continue;

        Vec2 corners[4];
        CalcQuadCorners(item, corners);
        tile->setCorners(corners);
        
        container->addChild(tile);
    }
}

// -----------------------------------------------------------------------------
// RefreshPositions
// -----------------------------------------------------------------------------

void LandscapeLand::RefreshPositions()
{
    if ( options->showLand || options->showWater ) {
        for ( auto node : getChildren() ) {
            if ( node->getName() == "floor" )
                continue;
            auto imageItem = static_cast<ImageItem*>(node->getUserObject());
            if ( imageItem != nullptr && imageItem->landscapeItem != nullptr ) {
                f32 x = imageItem->landscapeItem->position.x + imageItem->horizontalOffset;
                node->setPositionX(options->generator->NormaliseXPosition(x));
            }
        }
    }
    
    // rebuild checkerboard for panning
    if ( options->showWater ) {
        auto container = getChildByName(FLOOR_TILE_NAME);
        container->removeAllChildrenWithCleanup(true);
        DrawFloorTiles();
    }
}

