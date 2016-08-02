#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"
#include <iostream>

class boat{

    private:
        physVector position;
        double rudder;
        double sail;
    	battery bat;
	    double direction;

    public:
       boat();
        boat(double x, double y, double dir);
        //setters
        void setPosition(physVector pos);
        void setRudder(double r);
        void setSail(double s);
	void setDirection(double a);

        //getters
        physVector getPosition();
        double getRudder();
        double getSail();
        double getDirection();

        void moveBoat(physVector wind, physVector tide, long long timeStep);
        double bestAngle(physVector wind, double startAngle);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 90;
    sail = 0;
    direction = 0;
}

boat::boat(double x, double y, double dir){
    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 90;
    sail = 0;
    direction = dir;
}

void boat::setPosition(physVector pos){
    position = pos;
}

void boat::setRudder(double r){
    //90 degrees is the center, less is to the left and more is to the right
    rudder = r;
    if(rudder < 20){
	    rudder = 20;
    }
    else if(rudder >160){
        rudder = 160;
    }
}

void boat::setSail(double s){
    sail = s;
}

void boat::setDirection(double a){
    direction = a;
    if(direction>360){
        direction -= 360;
    }
    if(direction<0){
        direction += 359.999;
    }
}

physVector boat::getPosition(){
    return position;
}

double boat::getRudder(){
    return rudder;
}

double boat::getSail(){
    return sail;
}

double boat::getDirection(){
    return direction;
}

double boat::bestAngle(physVector wind, double startAngle){
    /*
    TODO: Change this function so that it returns
    the best angle that after x timeSteps will
    minimize the distance to target
    */
    if(startAngle < 0){
        startAngle = 360 + startAngle;
    }
    double bestSpeed = 0;
    double bestAng = -1;
    for(int i = (int)startAngle; i<(int)startAngle+40; i++){
        double dir = (double)(i%360);
        double speed=0;
        physVector north(3);
        north.setComponent(0, 1);
        double windAngle = (wind*-1.0)%north;
        //compare windAngle to boatAngle
        double BoatWindAngle = abs(windAngle-dir);

        if (BoatWindAngle> 180.0){
            BoatWindAngle = 360.0-BoatWindAngle;
        }
        if (BoatWindAngle <= 20.0){
            //no go zone
            speed = 0;
        }
        else if(BoatWindAngle > 20.0 && BoatWindAngle <=85.0){
            speed = (BoatWindAngle-20.0)/(85.0-20.0);
        }
        else if(BoatWindAngle > 85.0 && BoatWindAngle <=95.0){
            speed = 0.9;
        }

        else if(BoatWindAngle > 95.0){
            speed = (0.7-1)/(180-95)*BoatWindAngle + (1-((0.7-1)/(180-95)*95));

        }
        if(speed>bestSpeed){
            bestSpeed = speed;
            bestAng = dir;
        }
    }
    return bestAng;

}

void boat::moveBoat(physVector wind, physVector tide, long long timeStep){
    position = position + tide;
    //leeway, depends on boat


    //direction going

    //get angle of wind
    double speed=0;
    physVector north(3);
    north.setComponent(0, 1);
    double windAngle = (wind*-1.0)%north;
    //compare windAngle to boatAngle
    double BoatWindAngle = abs(windAngle-direction);

    if (BoatWindAngle> 180.0){
        BoatWindAngle = 360.0-BoatWindAngle;
    }
    if (BoatWindAngle <= 20.0){
        //no go zone
        speed = 0;
    }
    else if(BoatWindAngle > 20.0 && BoatWindAngle <=85.0){
        speed = (BoatWindAngle-20.0)/(85.0-20.0);
    }
    else if(BoatWindAngle > 85.0 && BoatWindAngle <=95.0){
        speed = 0.9;
    }

    else if(BoatWindAngle > 95.0){
        speed = (0.7-1)/(180-95)*BoatWindAngle + (1-((0.7-1)/(180-95)*95));
    }
    speed *= wind.getMagnitude()/60.0;

    //account for rudder
    if(rudder < 90){
	    direction-=((90.0-rudder)/150.0);
	    if(direction<0.0001){
		    direction+=360.0;
        }
    }
    else{
	    direction+=((rudder-90.0)/150.0);
        if(direction > 359.9999){
		    direction -=360.0;
	    }
    }
    physVector directionVector(3);
    directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
    directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer

    position = position + (directionVector);
    if(timeStep%50==0){
        std::cout<<"Rudder: "<<rudder<<" Direction: "<<direction<<" Speed: "<<speed<<" BoatWingAngle: "<<BoatWindAngle<<"\n";
    }
    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
