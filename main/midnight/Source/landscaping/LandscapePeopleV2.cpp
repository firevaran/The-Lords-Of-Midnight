//
//  LandscapePeopleV2.cpp
//  midnight
//
//  Created by Chris Wild on 07/03/2023.
//  Copyright © 2023 Chilli Hugger Software. All rights reserved.
//
#include "../axmol_sdk.h"

#include "LandscapePeopleV2.h"
#include "LandscapeColour.h"
#include "ILandscape.h"
#include "../system/resolutionmanager.h"
#include "../system/tmemanager.h"
#include "../system/shadermanager.h"

#include "../system/moonring.h"

#include "../ui/uihelper.h"

#include "../tme/tme_interface.h"

USING_NS_AX;
USING_NS_TME;

#define adjusty                 LRES(8)
constexpr f32 default_scale = scale_normal;

LandscapePeopleV2::LandscapePeopleV2()
{
}

LandscapePeopleV2* LandscapePeopleV2::create( LandscapeOptions* options )
{
    LandscapePeopleV2* node = new (std::nothrow) LandscapePeopleV2();
    if (node && node->initWithOptions(options))
    {
        node->autorelease();
        return node;
    }
    AX_SAFE_DELETE(node);
    return nullptr;
}


bool LandscapePeopleV2::initWithOptions( LandscapeOptions* options )
{
    if ( !LandscapeNode::initWithOptions(options) )
        return false;

    //setLocalZOrder(ZORDER_DEFAULT);
    setPosition(0,adjusty);
    setCascadeOpacityEnabled(true);
    setOpacity(ALPHA(1.0));

    characters=0;
    CLEARARRAY(columns);
    
    return true;
}

void LandscapePeopleV2::Initialise( LandscapeItem* item )
{
    std::string person;

    removeAllChildren();
    
    setOpacity(ALPHA(1.0f));
    setPosition(0,adjusty);
    setScale(default_scale);
    
    
    // we have not printed anyone yet
    characters = 0;
    CLEARARRAY ( columns );
    
    
    // if there are characters in front of us then
    // print them on screen
    for (u32 ii = 0; ii < item->lords.Count(); ii++) {
        character c;
        TME_GetCharacter ( c, item->lords[ii] );
        
        person = GetCharacterImage(c);
        auto image = add(person,DEFAULT_PRINT_CHARACTER);
        
        if(image != nullptr) {
            // create tooltip text
            auto title = Label::createWithTTF( uihelper::font_config_medium, c.longname );
            title->setName("title");
            title->setTextColor(Color4B(_clrWhite));
            title->enableOutline(Color4B(_clrBlack),RES(2));
            title->setLineSpacing(RES(-2));
            title->getFontAtlas()->setAntiAliasTexParameters();
            title->setAnchorPoint(uihelper::AnchorCenter);
            title->setWidth(RES(256));
            title->setPosition( Vec2(image->getContentSize().width/2, image->getContentSize().height/2) );
            title->setHorizontalAlignment(TextHAlignment::CENTER);
            title->setVerticalAlignment(TextVAlignment::BOTTOM);
            title->setVisible(false);
            image->addChild(title);
            
   
            image->setTouchEnabled(true);
            image->addClickEventListener(callback);
            image->setTag(ID_THINK_PERSON);
            image->setUserData(c.userdata);
            
            image->addTouchEventListener(
                [title](Ref* ref, Widget::TouchEventType type) {
                    if(type == Widget::TouchEventType::BEGAN ) {
                        title->setVisible(true);
                    } else if( type != Widget::TouchEventType::MOVED) {
                        title->setVisible(false);
                    }
                }
            );
        }

    }
    
    
    s32 objectid = GET_ID(item->objectid) ;

#if defined(_LOM_) || defined(_CITADEL_)
    if ( item->riders )
    {
        // LoM shows characters when there is an army in the location in front
        // however DDR only shows the army terrain image for wandering lords
        person = GetRaceImage(MAKE_ID(IDT_RACEINFO,RA_DOOMGUARD),TRUE);
        add(person, DEFAULT_PRINT_RIDERS);
    } else if ( item->warriors ) {
        person = GetRaceImage(MAKE_ID(IDT_RACEINFO,RA_DOOMGUARD),FALSE);
        add(person,DEFAULT_PRINT_WARRIORS);
    } else
#endif
    if ( objectid >= OB_WOLVES && objectid <= OB_WILDHORSES)    {
        person = GetObjectBig(MAKE_ID(IDT_OBJECT, objectid));
        if ( objectid == OB_DRAGONS ) {
            add(person,DEFAULT_PRINT_DRAGONS);
        }else{
            add(person,DEFAULT_PRINT_OTHER);
        }
    }
    
}

ax::ui::Widget* LandscapePeopleV2::add( std::string& person, int number)
{
    int column;
    Widget* imageAdded = nullptr;
    
#if defined(_LOM_) || defined(_CITADEL_)
    static int xtable[] = { 3, 5, 4, 1, 2, 6, 0, 7 };
#endif
#if defined(_DDR_)
    // 0 1 2 3 4 5 6 7
    static int xtable[] = { 3, 4, 2, 5, 6, 1, 7, 0 };
#endif
    
    auto scale = scale_normal ;
    
    f32 width = getContentSize().width;
    f32 offsetX = (width - RES(1024) * scale)/2;
    
    if ( person.empty() )
        return nullptr;
    
    int max_chars = MAX_DISPLAY_CHARACTERS ;
    
    for (u32 ii = 0; ii < number; ii++)    {
        
        // make sure our printing position is free
        for(;characters<max_chars;characters++)
            if ( !columns[xtable[characters]].used)
                break;
        
        if ( characters >= max_chars )
            return nullptr;
        
        column = xtable[characters] ;
        
        auto image = Sprite::create(person);
        image->setScale(scale);
        
        auto widget = Widget::create();
        widget->addChild(image);
        
        
        auto size = image->getContentSize();
        widget->setContentSize(size);
        
        int x1 = offsetX + ( column * LRES(CHARACTER_COLUMN_WIDTH*scale) );
        int y1 = 0;
        
        widget->setAnchorPoint(uihelper::AnchorBottomLeft);
        widget->setPosition(Vec2(x1, y1));

        image->setAnchorPoint(uihelper::AnchorBottomLeft);
        image->setPosition(Vec2(0, 0));

        if(options->characterTimeShader)
        {
            options->colour->updateCharacterNode(image);
        }
        addChild(widget);

        columns[column].used=TRUE;
        columns[column].pos = point(x1,y1);
        columns[column].scale = scale;
        columns[column].image = person ;
        
        // mark this printing position as used
        // and see if we have taken the one to the right of us
        if ( column <max_chars ) {
            //if ( LRES(person->Width()) > LRES(MAX_ALLOWED_CHARACTER_WIDTH) )
            //    m_columns[column+1].used = TRUE;
        }
        
        characters++;
        imageAdded = widget;
    }
    
    return imageAdded;
    
}

void LandscapePeopleV2::setCallback(const WidgetClickCallback &callback)
{
    this->callback = callback;
}

void LandscapePeopleV2::clear()
{
    characters=0;
}
