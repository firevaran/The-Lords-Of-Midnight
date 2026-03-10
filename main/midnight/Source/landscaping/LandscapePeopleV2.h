//
//  LandscapePeopleV2.h
//  midnight
//
//  Created by Chris Wild on 07/03/2023.
//  Copyright © 2023 Chilli Hugger Software. All rights reserved.
//

#ifndef LandscapePeopleV2_hpp
#define LandscapePeopleV2_hpp

#include <stdio.h>

#include "LandscapeNode.h"
#include "LandscapeGenerator.h"
#include "../ui/uielement.h"

typedef struct {
    bool        used;
    point       pos;
    f32         scale;
    std::string image;
} persontodraw_t ;


#define DEFAULT_PRINT_RIDERS            8
#define DEFAULT_PRINT_WARRIORS          8
#if defined(_LOM_) || defined(_CITADEL_)
#define DEFAULT_PRINT_DRAGONS           2
#endif
#if defined(_DDR_)
#define DEFAULT_PRINT_DRAGONS           4
#endif
#define DEFAULT_PRINT_OTHER             4
#define DEFAULT_PRINT_CHARACTER         1

#if defined(_DDR_)
#define MAX_DISPLAY_CHARACTERS          6
#define MAX_DISPLAY_CHARACTERS_TUNNEL   4
#else
#define MAX_DISPLAY_CHARACTERS          8
#endif

#define CHARACTER_COLUMN_WIDTH          128
#define MAX_ALLOWED_CHARACTER_WIDTH     128


using namespace chilli::types;

class LandscapePeopleV2 : public LandscapeNode
{
    using WidgetClickCallback = chilli::ui::WidgetClickCallback;
    using Widget = ax::ui::Widget;
        
public:
    static LandscapePeopleV2* create( LandscapeOptions* options );

    void Initialise( LandscapeItem* item );
   
    void clear();
    void stopAnim();
    
    void startSlideFromRight(s32 distance);
    void startSlideFromLeft(s32 distance);
    void startSlideOffLeft(s32 distance);
    void startSlideOffRight(s32 distance);
    void adjustMovement( f32 amount );

    void startFadeIn();
    void startFadeOut();
    
    void setCallback(const WidgetClickCallback& callback);

    
protected:
    LandscapePeopleV2();
    
    bool initWithOptions( LandscapeOptions* options );
    Widget* add( std::string& image, int number );

public:
    s32             characters;
    persontodraw_t  columns[8];
//    f32             amountmoved;
//    f32             startlocationx;
//    f32             startlocationy;
//    f32             amountmoving;
    WidgetClickCallback callback;
};



#endif /* LandscapePeopleV2_hpp */
