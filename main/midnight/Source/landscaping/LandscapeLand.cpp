//
//  LandscapeLand.cpp
//  citadel
//
//  Created by Chris Wild on 22/08/2017.
//
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

//const std::string floor_graphics[] = {
//      "t_land0"
//    , "t_land0"
//    , "t_land0" // "t_water0.png"
//    , "t_land0"
//    , "t_land0" //"t_water3.png"
//    , "t_land0"
//};

//const std::string floor_graphics[] = {
//      "t_land1.png"
//    , "t_snow1.png"
//    , "t_land1.png" // "t_water0.png"
//    , "t_water1.png"
//    , "t_land1.png" //"t_water3.png"
//    , "t_snow1.png"
//};

// -----------------------------------------------------------------------------
// Tuning constants
// -----------------------------------------------------------------------------

// How many grid units wide each water tile is rendered.
// > 1.0 causes neighbours to overlap, filling the gaps left by the
// elliptical brush-stroke shape of the water sprite.
// Start at 1.5 and increase if gaps are still visible.
constexpr f32 WATER_TILE_OVERLAP = 2.0f;

// Vertical squash applied on top of the X scale.
// Keeps water tiles flat relative to the horizon.
constexpr f32 WATER_HEIGHT_SCALE = 2.5f;

// Tiny extra X stretch to close sub-pixel cracks caused by floating-point
// rounding.  Usually 1.01 – 1.02 is enough.
constexpr f32 WATER_CRACK_GUARD = 1.01f;

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
    
    auto visibleSize = Director::getInstance()->getVisibleSize();

    auto floor = Sprite::createWithSpriteFrameName( "floor" );
    addChild(floor);
    
    floor->setPosition(Vec2::ZERO);
    floor->setAnchorPoint(Vec2::ZERO);
    floor->setName("floor");
    
    if ( options->terrainTimeShader ) {
        options->colour->updateTerrainNode(floor);
    }
    
    if ( options->debugLand )
        floor->setColor(Color3B::YELLOW);
    
    floorShader = mr->shader->GetTerrainTimeShader();
    
    return true;
}


void LandscapeLand::Build()
{
    auto items = options->generator->items;
    
    LandscapeItem* currentLocItem=nullptr;
    LandscapeItem* aheadLocItem=nullptr;
    
    // Scale the base floor sprite to fill the full width of the node.
    auto floor = getChildByName( "floor" );
    float scalex = getContentSize().width / floor->getContentSize().width ;
    floor->setScale(scalex,1.0);
    
    // -------------------------------------------------------------------------
    // Find the current and ahead location items so we can handle them specially
    // -------------------------------------------------------------------------
    for(auto const& item: *items) {
        if ( item->loc == options->currentLocation)
            currentLocItem = item;
        
        if ( item->loc == options->aheadLocation )
            aheadLocItem = item;
    }
    
    if ( currentLocItem == nullptr || aheadLocItem == nullptr )
        return;
        
    // -------------------------------------------------------------------------
    // Helper: draw one projected floor tile
    // -------------------------------------------------------------------------
    auto fnDrawTerrain = [=, this]( LandscapeItem* item, f32 adjustX, f32 adjustY )
   {
        if ( item->position.z < options->generator->viewportNear ||
             item->position.z >= options->generator->viewportFar )
            return;

        auto graphic = GetFloorImage(item->floor);
        if ( graphic == nullptr )
            return;

        // --- projection-correct scale ----------------------------------------
        // Use atan2 for accuracy at close distances (z = 1, 2 …).
        f32 cellAngle     = atan2f(1.0f, item->position.z);
        f32 projectedWidth = cellAngle * options->generator->PanoramaWidth / MX_PI2;

        f32 scaleX = (projectedWidth / graphic->getContentSize().width)
                     * WATER_TILE_OVERLAP
                     * WATER_CRACK_GUARD;
        f32 scaleY = scaleX * WATER_HEIGHT_SCALE;

        graphic->setScaleX(scaleX);
        graphic->setScaleY(scaleY);

        // --- position --------------------------------------------------------
        // Centre-anchored so the elliptical sprite is symmetric around the
        // projected map position.
        f32 x = options->generator->NormaliseXPosition(item->position.x);
        f32 y = this->getContentSize().height - item->position.y;

        graphic->setAnchorPoint(Vec2(0.5f, 0.5f));
        graphic->setPosition(x, y);

        // --- store for RefreshPositions --------------------------------------
        auto imageItem = new ImageItem(item, 0);
        imageItem->autorelease();
        graphic->setUserObject(imageItem);

        addChild(graphic);
    };
    
    // Skip the current location — it is drawn separately above.
    auto fnDrawItem = [=]( LandscapeItem* item ) {
        
        // we don't draw the current location here
        if ( item == currentLocItem  ) {
            return;
        }
        
        // simple draw for the location ahead
        //if ( item != aheadLocItem) {
            fnDrawTerrain(item, 1.0f, 1.0f);
        //    return ;
        //}
        
        // LandscapeItem under;
        // under.floor = item->floor;
        // under.position = item->position;
        // under.scale = item->scale;
    
    };
    
    // -------------------------------------------------------------------------
    // Draw current location floor (special-cased, full-width, moves with player)
    // -------------------------------------------------------------------------
 
    if ( options->showLand ) {
        // Current Location
        // TODO: Y popsition needs to be be also based on movement forward - options->movementAmount

        auto movementAmount = options->isMoving ? options->movementAmount : 0.0f;

        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto here = GetFloorImage(currentLocItem->floor);

        // TODO: ScaleX needs to be width of the screen +-
        // TODO: ScaleY needs to be 2 x location in front y pos
        if (here != nullptr) {
            here->setScale(4.0f);

            //here->setScale(8.0f);
            here->setPosition(Vec2(visibleSize.width/2,RES(-200)*movementAmount));
            here->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
            addChild(here);
        }
    }
    
    
    // -------------------------------------------------------------------------
    // Water — drawn first (underneath land)
    // -------------------------------------------------------------------------
    if ( options->showWater ) {
        for (auto const& item : *items) {
            if ( item->floor != floor_river &&
                 item->floor != floor_sea   &&
                 item->floor != floor_lake  )
                continue;
            fnDrawItem(item);
        }
    }
    
//    if ( options->showWater ) {
//        for(auto const& item: *items) {
//            if ( item->floor != floor_river && item->floor != floor_sea && item->floor != floor_lake )
//                continue;
//            
//         //   fnDrawItem(item);
//            
//            if(item->id == 128) {
//              fnDrawItem(item);
//                for(int ii=0; ii<8; ii++) {
//                    if(item->linked[ii]!=nullptr)
//                        fnDrawItem(item->linked[ii]);
//                }
//            }
//            
//        }
//    }
    
    
    // -------------------------------------------------------------------------
    // Land (non-snow, then snow on top)
    // -------------------------------------------------------------------------
    if ( options->showLand ) {
        for (auto const& item : *items) {
            if ( item->floor != floor_normal &&
                 item->floor != floor_debug  &&
                 item->floor != floor_none   )
                continue;
            fnDrawItem(item);
        }

        for (auto const& item : *items) {
            if ( item->floor != floor_snow )
                continue;
            fnDrawItem(item);
        }
    }
    
}

// -----------------------------------------------------------------------------
// GetFloorImage
// NOTE: No scaling is applied here — all scaling is owned by fnDrawTerrain.
// -----------------------------------------------------------------------------
Sprite* LandscapeLand::GetFloorImage( floor_t floor )
{
    // These floor types have no tile representation.
    if ( floor == floor_normal || floor == floor_none )
        return nullptr;

    auto image = Sprite::createWithSpriteFrameName("t_land0");
    if ( image == nullptr )
        return nullptr;

    // Choose tint colours per floor type.
    Color4F tint1 = Color4F(_clrWhite);
    Color4F tint2 = Color4F(_clrBlack);

    switch ( floor ) {
        case floor_lake:    tint2 = Color4F(Color3B(0x00, 0x00, 0x8d)); break;
        case floor_river:   tint2 = Color4F(Color3B(0x00, 0x00, 0xff)); break;
        case floor_sea:     tint2 = Color4F(Color3B(0x00, 0x20, 0x80)); break;
        case floor_snow:    tint2 = Color4F(Color3B(0xcc, 0xcc, 0xff)); break;
        case floor_debug:   image->setColor(Color3B::YELLOW);
                            break;
        default:            break;
    }

    mr->shader->AttachShader(image, floorShader);
    mr->shader->UpdateTerrainTimeShader(image, 0.5f, tint2, tint1);

    return image;
}

// -----------------------------------------------------------------------------
// RefreshPositions  — called every frame during panning/rotation
// -----------------------------------------------------------------------------
void LandscapeLand::RefreshPositions()
{
    for ( auto node : getChildren() ) {
        // Skip the base floor sprite — it has no ImageItem.
        if ( node->getName() == "floor" )
            continue;

        auto imageItem = static_cast<ImageItem*>(node->getUserObject());
        if ( imageItem != nullptr && imageItem->landscapeItem != nullptr ) {
            f32 x = imageItem->landscapeItem->position.x + imageItem->horizontalOffset;
            node->setPositionX(options->generator->NormaliseXPosition(x));
        }
        node->setTag(0);
    }
}
