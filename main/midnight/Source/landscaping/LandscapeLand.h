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
public:
    static LandscapeLand* create( LandscapeOptions* options );

    void Build() override;
    void RefreshPositions() override;
    
protected:
    bool initWithOptions( LandscapeOptions* options );
    Sprite* GetFloorImage( floor_t floor );
    
    SimpleShader* floorShader;
};

#endif /* LandscapeLand_hpp */
