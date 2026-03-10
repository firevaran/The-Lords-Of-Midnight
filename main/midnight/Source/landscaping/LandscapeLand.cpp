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
    
    auto floor = getChildByName( "floor" );
    float scalex = getContentSize().width / floor->getContentSize().width ;
    floor->setScale(scalex,1.0);
    
    for(auto const& item: *items) {
        
        if ( item->loc == options->currentLocation)
            currentLocItem = item;
        
        if ( item->loc == options->aheadLocation )
            aheadLocItem = item;
    }
    
    if ( currentLocItem == nullptr || aheadLocItem == nullptr )
        return;
    
    auto fnDrawTerrain = [=, this]( LandscapeItem* item, f32 adjustX, f32 adjustY )
    {
        if ((item->position.z>=options->generator->viewportNear)&&(item->position.z<options->generator->viewportFar))
        {
            auto graphic = GetFloorImage(item->floor);
            
            if ( graphic ) {
                graphic->setPosition(options->generator->NormaliseXPosition(item->position.x), this->getContentSize().height - item->position.y);
                
                graphic->setScaleX( graphic->getScaleX() * item->scale );
                graphic->setScaleY( graphic->getScaleY() * item->scale );
                graphic->setUserObject(item);
                
                auto imageItem = new ImageItem(item,0);
                imageItem->autorelease();
                
                graphic->setUserObject(imageItem);
                addChild(graphic);
            }
        }
        
    };
    
    auto fnDrawItem = [=]( LandscapeItem* item ) {
        
        // we don't draw the current location here
        if ( item == currentLocItem  ) {
            return;
        }
        
        // simple draw for the location ahead
       // if ( item != aheadLocItem) {
            fnDrawTerrain(item);
            return ;
        //}
        

        
    };
    
    if ( options->showLand ) {
        // Current Location
        // TODO: Y popsition needs to be be also based on movement forward - options->movementAmount

        auto movementAmount = options->isMoving ? options->movementAmount : 0.0f;

        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto here = GetFloorImage(currentLocItem->floor);

        // TODO: ScaleX needs to be width of the screen +-
        // TODO: ScaleY needs to be 2 x location in front y pos

        here->setScale(4.0f);

        //here->setScale(8.0f);
        here->setPosition(Vec2(visibleSize.width/2,RES(-200)*movementAmount));
        here->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        addChild(here);
    }
    
    
    //
    // Do the water first
    //
    if ( options->showWater ) {
        for(auto const& item: *items) {
            if ( item->floor != floor_river && item->floor != floor_sea && item->floor != floor_lake )
                continue;
            
         //   fnDrawItem(item);
            
            if(item->id == 128) {
              fnDrawItem(item);
                for(int ii=0; ii<8; ii++) {
                    if(item->linked[ii]!=nullptr)
                        fnDrawItem(item->linked[ii]);
                }
            }
            
        }
    }
    
    
    //
    // The everything else apart from snow
    //
    if ( options->showLand ) {
        for(auto const& item: *items) {
            if ( item->floor != floor_normal && item->floor != floor_debug && item->floor != floor_none )
                continue;

            fnDrawItem(item);
        }

        //
        // then snow
        //
        for(auto const& item: *items) {

            if ( item->floor != floor_snow )
                continue;

            fnDrawItem(item);

        }

    }
    
}

Sprite* LandscapeLand::GetFloorImage( floor_t floor )
{
    
    Sprite* image = nullptr;
    
    //if( floor == floor_lake || floor == floor_river )
        image = Sprite::createWithSpriteFrameName( "t_land0" );
    //else
    //    image = Sprite::createWithSpriteFrameName( "t_land1" );
    
    if ( image == nullptr )
        return image;
        

    auto tint1 = Color4F(_clrWhite);
    auto tint2 = Color4F(_clrBlack);
    
    
    if ( floor != floor_normal && floor != floor_none) {
        image->setScaleX(options->landScaleX);
        image->setScaleY(options->landScaleY);
    }
    
//    if ( floor == floor_debug ) {
//        image->setColor(Color3B::YELLOW);
//    }
//
//    if ( floor == floor_normal ) {
//        image->setColor(Color3B(tint2));
//    }

    if ( floor == floor_lake ) {
        //tint2 = Color4F(Color3B(0x00,0x00,0xcd));
        tint2 = Color4F(Color3B(0x00,0x00,0x8d));
    }

    if ( floor == floor_river ) {
        tint2 = Color4F(Color3B(0x00,0x00,0xff));
    }

    if ( floor == floor_normal ) {
        //tint1 = Color4F(options->colour->CalcCurrentMovementTint(TINT::TerrainOutline));
        //tint2 = Color4F(options->colour->CalcCurrentMovementTint(TINT::TerrainFill));
        //image->setScaleX(options->landScaleX*0.5f);
        //image->setScaleY(options->landScaleY*0.5f);
        return nullptr;
    }

    if ( floor == floor_none ) {
        //tint1 = Color4F(options->colour->CalcCurrentMovementTint(TINT::TerrainOutline));
        //tint2 = Color4F(options->colour->CalcCurrentMovementTint(TINT::TerrainFill));
        //image->setScaleX(options->landScaleX*0.5f);
        //image->setScaleY(options->landScaleY*0.5f);
        //tint2 = Color4F(Color3B(0xff,0xff,0xff));
        return nullptr;
    }

    
    mr->shader->AttachShader(image,floorShader);
    mr->shader->UpdateTerrainTimeShader(image, 0.5f, tint2, tint1);

    return image;
}


void LandscapeLand::RefreshPositions()
{
    for ( auto node : getChildren() ) {
        auto imageItem = static_cast<ImageItem*>(node->getUserObject());
        if (imageItem != nullptr && imageItem->landscapeItem!=nullptr) {
            f32 x = imageItem->landscapeItem->position.x+imageItem->horizontalOffset;
            node->setPositionX(options->generator->NormaliseXPosition(x));
        }
        node->setTag(0);
    }
}

