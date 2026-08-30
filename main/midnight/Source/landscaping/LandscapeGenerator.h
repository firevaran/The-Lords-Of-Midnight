//
//  LandscapeGenerator.h
//  citadel
//
//  Created by Chris Wild on 09/08/2017.
//
//

#ifndef LandscapeGenerator_h
#define LandscapeGenerator_h

#include "LandscapeNode.h"

#include "../system/resolutionmanager.h"

#define BASE_HEIGHT             768
#define BASE_WIDTH              1024

#define PEOPLE_WINDOW_HEIGHT    256
#define LANDSCAPE_DIR_AMOUNT    400.0f
#define LANDSCAPE_DIR_STEPS     400.0f
#define LANDSCAPE_DIR           (LANDSCAPE_DIR_AMOUNT/LANDSCAPE_DIR_STEPS)
#define LANDSCAPE_DIR_LEFT      (-LANDSCAPE_DIR)
#define LANDSCAPE_DIR_RIGHT     LANDSCAPE_DIR
#define LANDSCAPE_GSCALE        4.0f
#define LANDSCAPE_FULL_WIDTH    (LANDSCAPE_DIR_AMOUNT*8)

constexpr f32 TRANSITION_DURATION = 0.25f;

typedef enum floor_t {
      floor_none = 0
    , floor_normal
    , floor_snow
    , floor_river
    , floor_sea
    , floor_lake
    , floor_debug
} floor_t ;

typedef enum class tile_type {
    None        = 0,
    Solid       = 1,
    Edge        = 2,
    Corner      = 3,
} tile_type;

typedef struct tile_def_t {
    tile_type       type;
    int             rotation; // 0,1,2,3 = 0,90,180,270 clockwise
} tile_def_t;


class LandscapeItem : public Ref
{
    using Vec3 = ax::Vec3;
    using Vec2 = ax::Vec2;

public:
    tme::loc_t  loc;
    mxterrain_t terrain;
    floor_t     floor;
    s32         distance;
    bool        army;
    bool        mist;
    bool        graffiti;
    Vec3        position;
    f32         scale;
    
    bool        people;
    bool        riders;
    bool        warriors;
    mxid        objectid;
    c_mxid      lords;
    
    s32         id;
    
    LandscapeItem*  linked[8];
    Vec2            quadCorners[4];
    bool            quadValid;
    const tile_def_t*       tile;
    int             tilemask;
    
    bool        ahead;
    bool        current;
};

using LandscapeItems = ax::Vector<LandscapeItem*>;

FORWARD_REFERENCE(LandscapeOptions);
FORWARD_REFERENCE(moonring);

class LandscapeGenerator
{
public:
    
    float HorizonCentreX;
    float HorizonCentreY;
    float PanoramaWidth;
    float PanoramaHeight;
    float LocationHeight;
    
    const float viewportNear=0.25f;
    const float viewportFar=6.5f;
    
    // adjustment for the furthest of our visible locations being on the horizon
    float horizonAdjust;
    float horizonOffset;
    
public:
    LandscapeGenerator();
    virtual ~LandscapeGenerator();
    
    
    void Build(LandscapeOptions* options);
    void BuildPanorama();
    
    //void ProcessQuadrant(s32 x, s32 y, s32 dx, s32 dy, s32 qDim);
    LandscapeItem* ProcessLocation(s32 x, s32 y, s32 id);
    LandscapeItem* CalcCylindricalProjection(LandscapeItem* item);
    float RadiansFromFixedPointAngle(s32 fixed);
    f32 NormaliseXPosition(f32 x);
    
    LandscapeItem* GetPeople(mxid locId, maplocation& map, LandscapeItem* item);
    ax::Vec2 CalcGroundProjection(f32 worldX, f32 worldY);
    
protected:
    const tile_def_t* CalcTileMask( LandscapeItem* item );

    
public:
    moonring*           mr;
    LandscapeItems*     items;
    LandscapeOptions*   options;
    s32                 location_infront_y;
    tme::loc_t          loc;
    f32                 looking;
    f32                 horizontalOffset;
    f32                 landscapeScreenWidth;
};



#endif /* LandscapeGenerator_h */
