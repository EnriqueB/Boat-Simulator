#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"
#include <string.h>
#include <iostream>

#define FPS 60.0

class boat{

    private:
        physVector position;
        double rudder;
        double sail;
    	battery bat;
        int targetIndex;
	    double direction;
        double angle_sailingPoints[3];
        double speed_sailingPoints[3];
        double tackAngle;
        int tackTimer;
        int tackLimit;
        int tackStatus;
        int loopCount;
        long long lastLoop;

    public:
       boat();
        boat(double x, double y, double dir, double angles[], double speeds[]);

        //setters
        void setPosition(physVector pos) { position = pos; }
        void setRudder(double r);
        void setSail(double s) { sail = s; }
	    void setDirection(double a);
        void setTargetIndex(int t) { targetIndex = t; }
        void setTackAngle(double t);
        void setTackTimer(int t) { tackTimer = t; }
        void setTackLimit(int t) { tackLimit = t; }
        void setTackStatus(int t) { tackStatus = t; }
        void setLastLoop(long long l) { lastLoop = l; }

        //getters
        physVector getPosition() { return position; }
        double getRudder() { return rudder; }
        double getSail() { return sail; }
        double getDirection() { return direction; }
        double getSailingPoint(int index) { return angle_sailingPoints[index]; }
        int getTargetIndex() { return targetIndex; }
        double getTackAngle() { return tackAngle; }
        int getTackTimer() { return tackTimer; }
        int getTackLimit() { return tackLimit; }
        int getTackStatus() { return tackStatus; }
        int getLoopCount() { return loopCount; }
        long long getLastLoop() { return lastLoop; }

        void completedLoop() { loopCount++; }

        void moveBoat(physVector wind, physVector tide, long long timeStep);
        double bestAngle(physVector wind, double startAngle, int tackLimit, physVector target);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 90;
    sail = 0;
    direction = 0;
    memset(angle_sailingPoints, 0, sizeof(angle_sailingPoints));
    memset(speed_sailingPoints, 0, sizeof(speed_sailingPoints));
    tackLimit = 300;
    tackTimer = 0;
    targetIndex = 0;
    tackStatus = 0;
    loopCount = 0;
    lastLoop = 0;
}

boat::boat(double x, double y, double dir, double angles[], double speeds[]){
    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 90;
    sail = 0;
    direction = dir;
    angle_sailingPoints[0] = angles[0];
    angle_sailingPoints[1] = angles[1];
    angle_sailingPoints[2] = angles[2];

    speed_sailingPoints[0] = speeds[0];
    speed_sailingPoints[1] = speeds[1];
    speed_sailingPoints[2] = speeds[2];

    tackLimit = 300;
    tackTimer = 0;
    targetIndex = 0;
    tackStatus = 0;
    loopCount = 0;
    lastLoop = 0;
}

void boat::setRudder(double r){
    //90 degrees is the center, less is to the left and more is to the right
    rudder = r;
    if(rudder < 20){
	    rudder = 20;
    }
    else if(rudder >160) {
        rudder = 160;
    }
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

void boat::setTackAngle(double t){
    tackAngle = t;
    if(tackAngle >= 360.0){
        tackAngle = tackAngle - 360.0;
    }
    if(tackAngle < 0.0){
        tackAngle = 360 + tackAngle;
    }
}


double boat::bestAngle(physVector wind, double startAngle, int tackLimit, physVector target){
    /*
    TODO: Change this function so that it returns
    the best angle that after x timeSteps will
    minimize the distance to target
    */
    if(startAngle < 0){
        startAngle = 360 + startAngle;
    }
    double bestMagnitude = 1000000000000;
    double bestAng = -1;
    for(int i = (int)startAngle; i<(int)startAngle+45; i++){
        double dir = (double)(i%360);
        double speed=0;
        physVector north(3);
        north.setComponent(0, 1);
        double windAngle = (wind*-1.0)%north;
        //compare windAngle to boatAngle
        double BoatWindAngle = abs(windAngle-dir);

        if (BoatWindAngle <= angle_sailingPoints[0]){
            //no go zone
            speed = 0.15;
        }
        else if(BoatWindAngle > angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
            double m = (speed_sailingPoints[1]-speed_sailingPoints[1])/(angle_sailingPoints[1]-angle_sailingPoints[0]);
            double b = speed_sailingPoints[0] - m*angle_sailingPoints[0];
            speed = m* BoatWindAngle + b;
        }
        else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
            speed = speed_sailingPoints[1];
        }

        else if(BoatWindAngle > angle_sailingPoints[2]){
            double m = (speed_sailingPoints[2]-speed_sailingPoints[1])/(180-angle_sailingPoints[2]);
            double b = speed_sailingPoints[1] - m*angle_sailingPoints[2];
            speed = m*BoatWindAngle+b;
        }
        speed *= wind.getMagnitude()/60.0;
        physVector directionVector(3);
        directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
        directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer
        physVector pos = position;
        pos = pos + directionVector*((double)tackLimit);
        physVector vectToTarget = target-pos;
        double magnitude = vectToTarget.getMagnitude();

        if(magnitude<bestMagnitude){
            bestMagnitude = magnitude;
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
    if (BoatWindAngle <= angle_sailingPoints[0]){
        //no go zone
        speed = 0.15;
    }
    else if(BoatWindAngle > angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
        double m = (speed_sailingPoints[1]-speed_sailingPoints[0])/(angle_sailingPoints[1]-angle_sailingPoints[0]);
        double b = speed_sailingPoints[0] - m*angle_sailingPoints[0];
        speed = m* BoatWindAngle + b;
    }
    else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
        speed = speed_sailingPoints[1];
    }

    else if(BoatWindAngle > angle_sailingPoints[2]){
        //double a1 = 0.7-0.95;
        //double a2 = 180-angle_sailingPoints[2];
        double m = (speed_sailingPoints[2]-speed_sailingPoints[1])/(180.0-angle_sailingPoints[2]);
        double b = speed_sailingPoints[1] - m*angle_sailingPoints[2];
        speed = m*BoatWindAngle+b;
    }
    speed *= wind.getMagnitude()/FPS;
    speed*=sail;

    //account for rudder
    if(rudder < 90){
	    direction-=((90.0-rudder)/100.0);
	    if(direction<0.0001){
		    direction+=360.0;
        }
    }
    else{
	    direction+=((rudder-90.0)/100.0);
        if(direction > 359.9999){
		    direction -=360.0;
	    }
    }
    physVector directionVector(3);
    directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
    directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer

    position = position + (directionVector);
    if(timeStep%300==0){
        std::cout<<"Rudder: "<<rudder<<" Direction: "<<direction<<" Speed: "<<speed<<" BoatWingAngle: "<<BoatWindAngle<<"\n";
    }
    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
