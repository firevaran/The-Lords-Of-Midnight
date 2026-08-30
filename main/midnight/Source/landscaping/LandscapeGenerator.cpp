
#include "LandscapeGenerator.h"
#include "ILandscape.h"
#include "../system/moonring.h"

USING_NS_AX;
USING_NS_TME;

mxterrain_t toGeneralisedTerrain(mxterrain_t t);


LandscapeGenerator::LandscapeGenerator() :
    options(nullptr),
    items(new Vector<LandscapeItem*>()),
    mr(nullptr)
{
}

LandscapeGenerator::~LandscapeGenerator()
{
    if(items!=nullptr)
    {
        UIDEBUG("LandscapeGenerator: Clear Items");
        items->clear();
        SAFEDELETE(items);
    }
}


void LandscapeGenerator::Build(LandscapeOptions* options)
{
    this->options = options;
    this->mr = options->mr;
    
    auto visibleSize = Director::getInstance()->getVisibleSize();
    
    f32 aspect = 1024.0 / 768.0;
    f32 newWidth = visibleSize.height * aspect;
    options->resScale = newWidth / 1024.0;

    HorizonCentreX = LRES( (256*LANDSCAPE_GSCALE)/2  ) - options->lookOffsetAdjustment;
    HorizonCentreY = 0 ; //LRES( -112 );
    PanoramaWidth =  (float)LRES((800.0f*LANDSCAPE_GSCALE));
    PanoramaHeight = (float)LRES(38.0f*LANDSCAPE_GSCALE); // 32
    LocationHeight = (float)LRES(48.0f*LANDSCAPE_GSCALE);
    horizonAdjust = LRES((5*LANDSCAPE_GSCALE));
    horizonOffset = LRES( (112*LANDSCAPE_GSCALE) );
    

    loc = options->here;
    looking = 0;
    
    items->clear();
	
    if ( options->isInTunnel )
        return;
    
    BuildPanorama();

}
	
	
void LandscapeGenerator::BuildPanorama()
{
    UIDEBUG("LandscapeGenerator: BuildPanorama");

    s32	qDim;
    
    s32 x = loc.x/LANDSCAPE_DIR_STEPS;
    s32 y = loc.y/LANDSCAPE_DIR_STEPS;
    
    qDim = 8;
    
    s32 id = 1;
    
    LandscapeItem* cells[512];
    CLEARARRAY(cells);
    
    
    for ( int y1=y-qDim; y1<=y+qDim; y1++ ) {
        for ( int x1=x-qDim; x1<=x+qDim; x1++ ) {

            auto cell = ProcessLocation(x1, y1, id);
            if ( cell!= nullptr ) {
                items->pushBack(cell);
                cells[id-1] = cell;
            }
            id++;
        }
    }
 
    #define COPY(x,y) if( x>=0 && x<300) cell->linked[y] = cells[x]
    
    for(int ii=0; ii<300; ii++) {
        auto cell = cells[ii];
        CONTINUE_IF_NULL(cell);
        
        COPY(ii-18,0);
        COPY(ii-17,1);
        COPY(ii-16,2);
        
        COPY(ii-1,3);
        COPY(ii+1,4);
        
        COPY(ii+16,5);
        COPY(ii+17,6);
        COPY(ii+18,7);
    }
 
    for (auto const& item : *items) {
        item->tile = CalcTileMask(item);
    }
 
 
    sort( items->begin( ), items->end( ), [ ]( const LandscapeItem* lhs, const LandscapeItem* rhs )
    {
        return lhs->position.z > rhs->position.z;
    });
    
}

LandscapeItem* LandscapeGenerator::ProcessLocation(s32 x, s32 y, s32 id)
{
    maplocation     map;
    terraininfo     tinfo;

    LandscapeItem* item = new LandscapeItem();
    item->autorelease();
    
    auto locId = MAKE_LOCID(x, y);
    
    item->id = id;
    item->loc = loc_t(x,y);
    item->floor = floor_none;
    item->army = false;
    item->mist = false;
    item->position = Vec3(0,0,0);
    item->ahead = (item->loc == options->aheadLocation);
    item->current = (item->loc == options->currentLocation);
    item->tile = nullptr;
    CLEARARRAY(item->linked);
        
    
    TME_GetLocation( map, locId );
    TME_GetTerrainInfo ( tinfo, MAKE_ID(INFO_TERRAININFO, map.terrain) );

    if ( map.terrain == TN_LAKE3 )
        item->floor = floor_lake;
    else if ( map.terrain == TN_RIVER )
        item->floor = floor_river ;
    else if ( toGeneralisedTerrain(map.terrain) == TN_PLAINS )
        item->floor = floor_none ;
    else
        item->floor = floor_normal ;
    
    //if ( map.terrain == TN_SEA || map.terrain == TN_BAY  )
    //    item->floor = floor_sea ;
        
    //if (map.terrain == TN_ICYWASTE || map.terrain == TN_FROZENWASTE)
    //    item->floor = floor_snow ;


    item->terrain = map.terrain;
    
#if defined(_LOM_)
    item->graffiti = ( x == 4 && y == 10 );  // Lith in the domain of moon
    
    // Impassable mountains
    if (map.flags&lf_impassable && map.terrain == TN_MOUNTAIN) {
        item->terrain = TN_MOUNTAIN2;
    }
#endif

    if( map.flags&lf_army && tinfo.flags&tif_army )
        item->army = true;

    if ( !options->isMoving && item->army && item->current)
        item->army = false;
    
    // check the current lords army temporarily
    // popping up as we move in or out of a location
    if ( options->isMoving && item->army )
        if (   (x== options->moveFrom.x && y==options->moveFrom.y)
            || (x== options->moveTo.x && y==options->moveTo.y && !options->moveLocationHasArmy )  )
            item->army=false;
    
    // if there is mist here then we need to draw the mist
    if ( map.flags&lf_mist && !tme::variables::sv_display_no_mist)
        item->mist = true;
    
    
    item->distance = options->currentLocation.distance(item->loc);
    
// FEATURE LandscapePeopleV2
//    if(item->distance >0 && item->distance < 3) {
//        item = GetPeople(locId, map, item);
//
//    }
    
    return CalcCylindricalProjection(item);
    
}


// spectrum screen was 256x192
// Sky was 112  height
// floor was 80 height
// location in front was at 48 pixels from the bottom
// thus the panorama height was 32
// we need a 3 pixel horizon adjustment to put the far locations on the horizon

LandscapeItem* LandscapeGenerator::CalcCylindricalProjection(LandscapeItem* item)
{
    float	x, y, xOff, yOff;
    double angle, objAngle, viewAngle;
    
    x = (float)( (item->loc.x*LANDSCAPE_DIR_STEPS) - loc.x) / (float)LANDSCAPE_DIR_STEPS;
    y = (float)( (item->loc.y*LANDSCAPE_DIR_STEPS) - loc.y) / (float)LANDSCAPE_DIR_STEPS;
    
    f32 looking_amount = looking;
    viewAngle = RadiansFromFixedPointAngle( looking_amount );
    objAngle = atan2f(x, -y);
    angle = objAngle - viewAngle;

    if (angle>MX_PI)
        angle -= MX_PI2;
    if (angle<-MX_PI)
        angle += MX_PI2;
    
    //	convert angle to horizon centre xOffset (cylindrical projection)
    xOff = angle*PanoramaWidth/(MX_PI2);
    
    //	now do the horizon centre yOffset perspective projection
    item->position.z = sqrtf(x*x + y*y);
    
    item->scale = 1.0f/item->position.z;
    
    yOff = PanoramaHeight*item->scale;
    
    item->position.x = xOff + HorizonCentreX;
    item->position.y = yOff + HorizonCentreY - horizonAdjust;
    
    // We are running a panorama that runs from N to NW along a linear
    // so place all locations to the right
    if (item->position.x<=LRES(-225))
        item->position.x += PanoramaWidth;
    
    // Calculate quad corners in raw projection space
    // before any NormaliseXPosition is applied
    item->quadValid = false;

    if (item->position.z >= viewportNear && item->position.z < viewportFar)
    {
        const f32 p = 0.5f;
        f32 gx = item->loc.x;
        f32 gy = item->loc.y;

        // Project all 4 corners
        auto projectCorner = [&](f32 cx, f32 cy) -> ax::Vec2
        {
            f32 dx = cx - (loc.x / LANDSCAPE_DIR_STEPS);
            f32 dy = cy - (loc.y / LANDSCAPE_DIR_STEPS);

            f32 dist = sqrtf(dx*dx + dy*dy);
            if (dist < 0.0001f) dist = 0.0001f;

            f32 cObjAngle  = atan2f(dx, -dy);
            f32 cAngle     = cObjAngle - viewAngle;

            if (cAngle >  MX_PI) cAngle -= MX_PI2;
            if (cAngle < -MX_PI) cAngle += MX_PI2;

            f32 screenX = cAngle * PanoramaWidth / MX_PI2 + HorizonCentreX;
            f32 screenY = (PanoramaHeight / dist) + HorizonCentreY - horizonAdjust;
            screenY     = std::min(screenY, horizonOffset);

            return ax::Vec2(screenX, screenY);
        };


        item->quadCorners[0] = projectCorner(gx - p, gy - p);
        item->quadCorners[1] = projectCorner(gx + p, gy - p);
        item->quadCorners[2] = projectCorner(gx + p, gy + p);
        item->quadCorners[3] = projectCorner(gx - p, gy + p);

        // Wraparound: find the median X and shift outlier corners
        // to be consistent with the group
        f32 xs[4] = {
            item->quadCorners[0].x,
            item->quadCorners[1].x,
            item->quadCorners[2].x,
            item->quadCorners[3].x
        };

//         UIDEBUG("Item ID: %d, [0]=%.4f, [1]=%.4f [2]=%.4f, [3]=%.4f",
//            item->id,
//            xs[0],
//            xs[1],
//            xs[2],
//            xs[3]
//        );
    

        // Use corner[0] as reference, bring others within half panorama of it
        f32 ref = xs[0];
        for (int ii = 1; ii < 4; ii++) {
            while (xs[ii] - ref >  PanoramaWidth * 0.5f) xs[ii] -= PanoramaWidth;
            while (xs[ii] - ref < -PanoramaWidth * 0.5f) xs[ii] += PanoramaWidth;
        }

        for (int ii = 0; ii < 4; ii++)
            item->quadCorners[ii].x = xs[ii];

        item->quadValid = true;
    }
    
//    if (item->current) {
//        UIDEBUG("currentLocItem z=%.4f near=%.4f far=%.4f valid=%d",
//            item->position.z,
//            viewportNear, viewportFar,
//            item->quadValid);
//    }
//  
//    if (item->ahead) {
//        UIDEBUG("aheadItem z=%.4f near=%.4f far=%.4f valid=%d",
//            item->position.z,
//            viewportNear, viewportFar,
//            item->quadValid);
//    }
    
    return item;
}

float LandscapeGenerator::RadiansFromFixedPointAngle(s32 fixed)
{
    float angle = (float)fixed;
    angle = angle*MX_PI2/4096.0f;
    return angle;
}

//
// X coordinafe is in Scaled Panorama units (ie: real screen units)
//
f32 LandscapeGenerator::NormaliseXPosition(f32 x)
{
    x = x-LRES(horizontalOffset) ;
    
    // Boundary in panoramic units
    f32 boundary = LANDSCAPE_DIR_STEPS*3;
    f32 maxScreenX = landscapeScreenWidth+LRES(512);
    f32 minScreenX = LRES(-512);
    
    // to the left
    if ( horizontalOffset<boundary && x>maxScreenX )
    {
        x -= PanoramaWidth;
    }

    // to the right
    if ( horizontalOffset>=boundary && x<minScreenX )
    {
        x += PanoramaWidth;
    }

    return x;
}

LandscapeItem* LandscapeGenerator::GetPeople(mxid locId, maplocation& map, LandscapeItem* item)
{
    c_mxid objects;

    item->warriors = item->riders = false;
    item->objectid = OB_NONE ;
    item->lords.Clear();
    
    if( GET_ID(map.object) > OB_NONE && GET_ID(map.object) < OB_SHELTER ) {
        item->objectid = map.object;
    }
    
    if(map.terrain == TN_CITADEL) {
        int a = 100;
    }
    
    // get the characters infront of us
#if defined(_LOM_) || defined(_CITADEL_)
    u32 recruited;
    TME_GetCharacters ( locId, objects, recruited );
#endif
    
#if defined(_DDR_)
    TME_GetCharactersAtLocation(locid, objects, TRUE, options->isInTunnel);

    if ( options->isInTunnel ) {
        item->objectid = options->isLookingDownTunnel
            ? map.object_tunnel
            : OB_NONE;
    }
#endif

   for (u32 ii = 0; ii < objects.Count(); ii++) {
        character c;
        TME_GetCharacter ( c, objects[ii] );
        
        CONTINUE_IF(c.id==options->characterId);
            
        
#if defined(_DDR_)
        CONTINUE_IF( options->isLookingDownTunnel && !Character_IsInTunnel(c) );
#endif
        CONTINUE_IF( Character_IsDead(c) ||  Character_IsHidden(c) );


        item->lords.Add(c.id);
    }
  
    
#if defined(_LOM_) || defined(_CITADEL_)
    //if (item->army || item->lords.Count()) {
        TME_GetLocationInfo(item->loc);
        if ( location_armies.foe_riders )
        {
            item->riders = true;
        } else if ( location_armies.foe_warriors ) {
            item->warriors = true;
        }
    //}
#endif

    item->people = item->lords.Count() ||
                    item->riders ||
                    item->warriors ||
                    item->objectid != OB_NONE;
    
    return item;
}


mxterrain_t toGeneralisedTerrain(mxterrain_t t)
{
    switch (t) {
        case TN_PLAINS2:
        case TN_PLAINS3:
        case TN_LAND:
        case TN_PLAIN:
            return TN_PLAINS;
            
        case TN_FOREST2:
        case TN_FOREST3:
        case TN_TREES:
            return TN_FOREST;
            
        case TN_MOUNTAIN2:
        case TN_MOUNTAIN3:
        case TN_ICY_MOUNTAIN:
            return TN_MOUNTAIN;
            
        case TN_WATCHTOWER:
            return TN_TOWER;
            
        case TN_ICYWASTE:
            return TN_FROZENWASTE;
            
        case TN_LAKE3:
            return TN_LAKE;
            
        case TN_HILLS3:
        case TN_DOWNS:
        case TN_FOOTHILLS:
            return TN_HILLS;
            
        case TN_STONES:
            return TN_LITH;
            
        default:
            return t;
    }
}


// Projects a raw world-space grid position (in grid units, NOT multiplied by
// LANDSCAPE_DIR_STEPS) onto the screen ground plane using the same cylindrical
// projection as CalcCylindricalProjection(), but returns only the screen X/Y
// at ground level (horizon baseline).
//
// worldX, worldY  — grid coordinates of the point to project
//

ax::Vec2 LandscapeGenerator::CalcGroundProjection(f32 worldX, f32 worldY)
{
    // Offset from the player's position (same as CalcCylindricalProjection).
    f32 x = worldX - (loc.x / LANDSCAPE_DIR_STEPS);
    f32 y = worldY - (loc.y / LANDSCAPE_DIR_STEPS);

    // Distance — guard against divide-by-zero at the player's own tile.
    f32 dist = sqrtf(x * x + y * y);
    if ( dist < 0.0001f )
        dist = 0.0001f;

    // Cylindrical projection: angle relative to current view direction.
    f32 viewAngle = RadiansFromFixedPointAngle(looking);
    f32 objAngle  = atan2f(x, -y);
    f32 angle     = objAngle - viewAngle;

    if ( angle >  MX_PI ) angle -= MX_PI2;
    if ( angle < -MX_PI ) angle += MX_PI2;

    // Screen X — identical to CalcCylindricalProjection.
    f32 screenX = angle * PanoramaWidth / MX_PI2 + HorizonCentreX;

    // Screen Y — ground plane Y at this distance.
    // PanoramaHeight / dist matches the terrain sprite anchor Y.
    f32 screenY = (PanoramaHeight / dist) + HorizonCentreY - horizonAdjust;

    // Clamp in panorama space — screenY must never exceed horizonOffset
    // (the ground plane baseline). Without this, near corners where dist → 0
    // blow up to huge values, creating V-shaped polys.
    screenY = std::min(screenY, horizonOffset);

    // Panorama wraparound.
    if ( screenX <= LRES(-225) )
        screenX += PanoramaWidth;

    return ax::Vec2(screenX, screenY);
}

// index is the 4-bit bitmask: bit0=N, bit1=E, bit2=S, bit3=W
static const tile_def_t tile_def_table[16] = {
    { tile_type::None,  0 },  // 0000 -  0 - no connections
    { tile_type::Edge,  0 },  // 0001 -  1 - N
    { tile_type::Edge,  1 },  // 0010 -  2 - E
    { tile_type::Corner,0 },  // 0011 -  3 - N+E
    { tile_type::Edge,  2 },  // 0100 -  4 - S
    { tile_type::Solid, 0 },  // 0101 -  5 - N+S
    { tile_type::Solid, 1 },  // 0110 -  6 - E+S
    { tile_type::Solid, 0 },  // 0111 -  7 - N+E+S
    { tile_type::Edge,  3 },  // 1000 -  8 - W
    { tile_type::Corner,3 },  // 1001 -  9 - W+N
    { tile_type::Solid, 1 },  // 1010 - 10 - E+W
    { tile_type::Solid, 3 },  // 1011 - 11 - E+S+W
    { tile_type::Corner,2 },  // 1100 - 12 - S+W
    { tile_type::Solid, 2 },  // 1101 - 13 - S+W+N
    { tile_type::Solid, 1 },  // 1110 - 14 - W+N+E
    { tile_type::Solid, 0 },  // 1111 - 15 - all
};

bool isWater( LandscapeItem* item ) {
    if (item==nullptr) return false;
    return (item->floor == floor_lake || item->floor == floor_river);
}

const tile_def_t* LandscapeGenerator::CalcTileMask( LandscapeItem* item )
{
    if (!isWater(item)) return nullptr;
        
    u8 mask = 0;
    
    // linked[1]=N, linked[4]=E, linked[6]=S, linked[3]=W
    if ( isWater(item->linked[1]) ) mask |= 1; // N
    if ( isWater(item->linked[4]) ) mask |= 2; // E
    if ( isWater(item->linked[6]) ) mask |= 4; // S
    if ( isWater(item->linked[3]) ) mask |= 8; // W
    
    item->tilemask = mask;
    item->tile = &tile_def_table[mask];
    return item->tile;
}
