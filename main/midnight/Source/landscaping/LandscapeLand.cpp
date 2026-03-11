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

// Overlap multiplier for shoreline brushstroke ellipses (edge tiles only).
constexpr f32 WATER_TILE_OVERLAP = 2.0f;
constexpr f32 WATER_HEIGHT_SCALE = 2.5f;
constexpr f32 WATER_CRACK_GUARD  = 1.01f;

// Inward padding for quad corners (0=none, 0.08=8%).
constexpr f32 QUAD_CORNER_PADDING = 0.0f;

// Solid fill colours for projected water quads.
static const Color4F WATER_FILL_LAKE  (0x00/255.f, 0x00/255.f, 0x8d/255.f, 1.0f);
static const Color4F WATER_FILL_RIVER (0x00/255.f, 0x00/255.f, 0xff/255.f, 1.0f);
static const Color4F WATER_FILL_SEA   (0x00/255.f, 0x20/255.f, 0x80/255.f, 1.0f);


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

    floorShader = mr->shader->GetTerrainTimeShader();

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
    if ( options->debugMode != 0 ) {
        DrawDebugCheckerboard();
        return;
    }

    // -------------------------------------------------------------------------
    // Find current and ahead location items.
    // -------------------------------------------------------------------------
    LandscapeItem* currentLocItem = nullptr;
    LandscapeItem* aheadLocItem   = nullptr;

    for (auto const& item : *items) {
        if ( item->loc == options->currentLocation ) currentLocItem = item;
        if ( item->loc == options->aheadLocation   ) aheadLocItem   = item;
    }

    if ( currentLocItem == nullptr || aheadLocItem == nullptr )
        return;

    // -------------------------------------------------------------------------
    // Draw current location floor (full-width, moves with player)
    // -------------------------------------------------------------------------
    if ( options->showLand ) {
        f32  movementAmount = options->isMoving ? options->movementAmount : 1.0f;
        auto visibleSize    = Director::getInstance()->getVisibleSize();
        auto here           = GetFloorImage(floor_snow);

        if ( here ) {
            here->setScale(4.0f, 5.0f);
            here->setPosition(Vec2(visibleSize.width / 2, RES(50)));
            here->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
            addChild(here);
        }
    }

    // -------------------------------------------------------------------------
    // Helper: draw one projected floor ellipse tile
    // -------------------------------------------------------------------------
    auto fnDrawTerrain = [=]( LandscapeItem* item )
    {
        if ( item->position.z < options->generator->viewportNear ||
             item->position.z >= options->generator->viewportFar )
            return;

        auto graphic = GetFloorImage(item->floor);
        if ( graphic == nullptr )
            return;

        f32 cellAngle      = atan2f(1.0f, item->position.z);
        f32 projectedWidth = cellAngle * options->generator->PanoramaWidth / MX_PI2;

        f32 scaleX = (projectedWidth / graphic->getContentSize().width)
                     * WATER_TILE_OVERLAP
                     * WATER_CRACK_GUARD;
        f32 scaleY = scaleX * WATER_HEIGHT_SCALE;

        graphic->setScaleX(scaleX);
        graphic->setScaleY(scaleY);
        graphic->setAnchorPoint(Vec2(0.5f, 0.5f));
        graphic->setPosition(
            options->generator->NormaliseXPosition(item->position.x),
            this->getContentSize().height - item->position.y
        );

        auto imageItem = new ImageItem(item, 0);
        imageItem->autorelease();
        graphic->setUserObject(imageItem);
        addChild(graphic);
    };

    auto fnDrawItem = [=]( LandscapeItem* item )
    {
        if ( item == currentLocItem )
            return;
        fnDrawTerrain(item);
    };

    // -------------------------------------------------------------------------
    // Water — solid projected quads first, shoreline ellipses on top
    // -------------------------------------------------------------------------
    if ( options->showWater ) {

        auto waterFill = DrawNode::create();
        waterFill->setAnchorPoint(Vec2::ZERO);
        waterFill->setPosition(Vec2::ZERO);
        addChild(waterFill);

        for (auto const& item : *items) {
            if ( item->floor != floor_river &&
                 item->floor != floor_sea   &&
                 item->floor != floor_lake  )
                continue;

            //DrawWaterQuad(waterFill, item);

            // Shoreline ellipse only for edge tiles (any non-water neighbour).
            bool isEdge = false;
            for (int ii = 0; ii < 8; ii++) {
                auto n = item->linked[ii];
                if ( n == nullptr ||
                    (n->floor != floor_river &&
                     n->floor != floor_sea   &&
                     n->floor != floor_lake  ) ) {
                    isEdge = true;
                    break;
                }
            }
            if ( isEdge )
                fnDrawItem(item);
        }
    }

    // -------------------------------------------------------------------------
    // Land (non-snow first, snow on top)
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
// CalcQuadCorners
// Projects the 4 ground-plane corners of a tile, normalises X, clamps Y.
// Returns false if the quad should be culled.
// -----------------------------------------------------------------------------
bool LandscapeLand::CalcQuadCorners( LandscapeItem* item, Vec2* corners, f32 padding )
{
    f32 gx = item->loc.x;
    f32 gy = item->loc.y;
    f32 h  = getContentSize().height;
    f32 sw = getContentSize().width;
    f32 p  = 0.5f - padding;
    f32 panorama = options->generator->PanoramaWidth;

    corners[0] = options->generator->CalcGroundProjection(gx - p, gy - p);
    corners[1] = options->generator->CalcGroundProjection(gx + p, gy - p);
    corners[2] = options->generator->CalcGroundProjection(gx + p, gy + p);
    corners[3] = options->generator->CalcGroundProjection(gx - p, gy + p);

    for (int ii = 0; ii < 4; ii++) {
        corners[ii].x = options->generator->NormaliseXPosition(corners[ii].x);
        corners[ii].y = std::min(h, h - corners[ii].y);
        
        // Second pass - wrap any remaining out-of-range corners
        // independently of horizontalOffset direction
        f32 maxScreenX = options->generator->landscapeScreenWidth + LRES(512);
        f32 minScreenX = LRES(-512);
        if (corners[ii].x > maxScreenX)
            corners[ii].x -= options->generator->PanoramaWidth;
        if (corners[ii].x < minScreenX)
            corners[ii].x += options->generator->PanoramaWidth;
    }

    if (!QuadIsValid(corners)) {

        // Try fixing only negative corners by shifting right
        Vec2 fixed[4];
        memcpy(fixed, corners, sizeof(Vec2) * 4);
        for (int ii = 0; ii < 4; ii++) {
            if (fixed[ii].x < 0)
                fixed[ii].x += panorama;
        }
        if (QuadIsValid(fixed)) {
            UIDEBUG("ONSCREEN id=%d [0]=%.1f [1]=%.1f [2]=%.1f [3]=%.1f sw=%.1f minX=%.1f",
                item->id,
                fixed[0].x, fixed[1].x, fixed[2].x, fixed[3].x,
                sw, LRES(-512));
            
            // Reject if all corners are off screen
            bool anyOnScreen = false;
            for (int ii = 0; ii < 4; ii++) {
                if (fixed[ii].x > LRES(-512) && fixed[ii].x < sw + LRES(512)) {
                    anyOnScreen = true;
                    break;
                }
            }
            if (anyOnScreen) {
                memcpy(corners, fixed, sizeof(Vec2) * 4);
                return true;
            }
        }

        // Try fixing only positive-overflow corners by shifting left
        memcpy(fixed, corners, sizeof(Vec2) * 4);
        for (int ii = 0; ii < 4; ii++) {
            if (fixed[ii].x > sw)
                fixed[ii].x -= panorama;
        }
        if (QuadIsValid(fixed)) {
            UIDEBUG("PATH id=%d loc=(%d,%d) FIXED_POS [0]=%.1f [1]=%.1f [2]=%.1f [3]=%.1f",
                item->id, (int)item->loc.x, (int)item->loc.y,
                fixed[0].x, fixed[1].x, fixed[2].x, fixed[3].x);
            memcpy(corners, fixed, sizeof(Vec2) * 4);
            return true;
        }

        // Near tile perspective spread fallback
        f32 farSpread = std::abs(corners[1].x - corners[0].x);
        UIDEBUG("PATH id=%d loc=(%d,%d) FARSPREAD=%.1f limit=%.1f %s",
            item->id, (int)item->loc.x, (int)item->loc.y,
            farSpread, panorama * 0.25f,
            farSpread < panorama * 0.25f ? "ACCEPTED" : "REJECTED");
        if (farSpread < panorama * 0.25f)
            return true;

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
    return (maxX - minX) < (options->generator->PanoramaWidth * 0.25f);
}


// -----------------------------------------------------------------------------
// DrawWaterQuad
// -----------------------------------------------------------------------------

void LandscapeLand::DrawWaterQuad( DrawNode* drawNode, LandscapeItem* item )
{
    if ( item->position.z < options->generator->viewportNear ||
         item->position.z >= options->generator->viewportFar )
        return;

    Vec2 corners[4];
    if ( !CalcQuadCorners(item, corners, QUAD_CORNER_PADDING) )
        return;

    Color4F colour = WATER_FILL_LAKE;
    if ( item->floor == floor_river ) colour = WATER_FILL_RIVER;
    if ( item->floor == floor_sea   ) colour = WATER_FILL_SEA;

    drawNode->drawSolidPoly(corners, 4, colour);
}

// -----------------------------------------------------------------------------
// DrawDebugCheckerboard
// -----------------------------------------------------------------------------

void LandscapeLand::DrawDebugCheckerboard()
{
    static const Color4F palette[] = {
        Color4F(0.5f, 0.5f, 0.5f, 1.0f),   // floor_none   — mid grey
        Color4F(0.3f, 0.6f, 0.3f, 1.0f),   // floor_normal — green
        Color4F(0.9f, 0.9f, 1.0f, 1.0f),   // floor_snow   — pale blue-white
        Color4F(0.0f, 0.3f, 1.0f, 1.0f),   // floor_river  — bright blue
        Color4F(0.0f, 0.1f, 0.8f, 1.0f),   // floor_sea    — dark blue
        Color4F(0.0f, 0.0f, 0.6f, 1.0f),   // floor_lake   — navy
        Color4F(1.0f, 1.0f, 0.0f, 1.0f),   // floor_debug  — yellow
    };
    static const int paletteSize = sizeof(palette) / sizeof(palette[0]);

    
    Color4F matrix[16][16];
    for (int row = 0; row < 16; ++row) {
        for (int col = 0; col < 16; ++col) {
            float r = row / 15.0f; // Range [0,1]
            float g = col / 15.0f; // Range [0,1]
            float b = (row + col) / 30.0f; // Range [0,1]
            matrix[row][col] = Color4F(r, g, b, 1.0f);
            UIDEBUG("Colour[%d][%d]= R:%.4f,G:%.4f,B:%.4f", row, col, r, g, b);
        }
    }


    auto drawNode = DrawNode::create();
    drawNode->setAnchorPoint(Vec2::ZERO);
    drawNode->setPosition(Vec2::ZERO);
    addChild(drawNode);

    auto items = options->generator->items;

    for (auto const& item : *items) {
        if ( item->position.z < options->generator->viewportNear ||
             item->position.z >= options->generator->viewportFar )
            continue;

        Vec2 corners[4];
        if ( !CalcQuadCorners(item, corners, QUAD_CORNER_PADDING) )
            continue;

        int floorIdx = (int)item->floor;
        if ( floorIdx < 0 || floorIdx >= paletteSize )
            floorIdx = 0;


        // auto c = Color4F(item->id/255.0f, item->loc.x/255.0f, item->loc.y/255.0f, 1.0f);
        auto x = item->id % 16;
        auto y = item->id / 16;
        auto c = matrix[y][x];
        
        drawNode->drawSolidPoly(corners, 4, c);
    }
}

// -----------------------------------------------------------------------------
// GetFloorImage
// -----------------------------------------------------------------------------

Sprite* LandscapeLand::GetFloorImage( floor_t floor )
{
    if ( floor == floor_normal || floor == floor_none )
        return nullptr;

    auto image = Sprite::createWithSpriteFrameName("t_land0");
    if ( image == nullptr )
        return nullptr;

    Color4F tint1 = Color4F(_clrWhite);
    Color4F tint2 = Color4F(_clrBlack);

    switch ( floor ) {
        case floor_lake:   tint2 = Color4F(Color3B(0x00, 0x00, 0x8d)); break;
        case floor_river:  tint2 = Color4F(Color3B(0x00, 0x00, 0xff)); break;
        case floor_sea:    tint2 = Color4F(Color3B(0x00, 0x20, 0x80)); break;
        case floor_snow:   tint2 = Color4F(Color3B(0xcc, 0xcc, 0xff)); break;
        case floor_debug:  image->setColor(Color3B::YELLOW);            break;
        default:                                                         break;
    }

    mr->shader->AttachShader(image, floorShader);
    mr->shader->UpdateTerrainTimeShader(image, 0.5f, tint2, tint1);

    return image;
}

// -----------------------------------------------------------------------------
// RefreshPositions
// -----------------------------------------------------------------------------

void LandscapeLand::RefreshPositions()
{
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

