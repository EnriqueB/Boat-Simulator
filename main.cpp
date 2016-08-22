#include <GL/freeglut.h>
#include <chrono>
#include "physVector.h"
#include "boat.h"

#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <cmath>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <string.h>

#define FPS 60.0

int RUN_SIZE = 3;
int POPULATION_SIZE = 300;
int TOURNAMENT_SIZE = 5;
int GENERATIONS = 300;
int XOVER_RATE = 0.9;

bool GUI = true;

using namespace std;

//Clock used for enforcing FPS
auto start = chrono::steady_clock::now();

// actual vector representing the camera's direction
float lx=0.0f,lz=-1.0f, cy=0;
// XZ position of the camera
float x=0.0f,z=100.0f;

// angle for rotating triangle
float angle = 0.0f;
float acceleration;
float accelerationVal;

vector <boat> boats;
physVector tide[4];
physVector targets [2];
physVector north(3);
physVector wind[4];

double avgFitness = 0;

//bool GUI = false;

int runIndex =0;

long long timeStep;
long long lastLoop = 0;
double originalHeading;

//change to 15 to 30
double sailingPoints[][3] = {{15.0,  85.0, 95.0},
                                {20.0, 85.0, 95.0},
                                {25.0, 80.0, 100.0},
                                {30.0, 85.0, 95.0}};
double speedPoints[][3] = {{0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8}};

double colors [][3] = {{1.0, 0.0, 0.0},         //red
                        {0.0, 1.0, 0.0},        //green
                        {1.0, 1.0, 0.0},        //yellow
                        {1.0, 1.0, 1.0},        //white
                        {1.0, 0.0, 1.0}};       //purple

bool usedSrand = false;
bool racing = false;


/*
 * This struct is used as individuals for the
 * genetic algorithm system.
 */
struct INDIVIDUAL{
    long long iterations;
    int loops;
    double fitness=0;
    /*
     * 0: minAngle
     * 1: maxAngle
     * 2: angleStep
     * 3: minTack
     * 4: maxTack
     * 5: tackStep
     */
    int parameters[6];

    INDIVIDUAL(){
        if(!usedSrand){
            srand(time(NULL));
            usedSrand = true;
        }
        fitness = -1;
        iterations = 0;
        loops=0;
        parameters[0] = rand()%180;
        parameters[1] = rand()%180;
        parameters[2] = rand()%20 + 1;
        parameters[3] = rand()%100 + 1;
        parameters[4] = rand()%3000 + 1;
        parameters[5] = rand()%50 + 1;

        while((parameters[3] > parameters[4]) || (parameters[3] == parameters[4])){
            parameters[3] = rand()%100 +1;
            parameters[4] = rand()%3000 +1;
        }
    }
};

vector <INDIVIDUAL> individuals(POPULATION_SIZE);

/*
 * The genetic algorithm uses tournament selection.
 * The parameter type determines the type of
 * tournament, true for normal, false for negative
 */
int tournament(bool type){
	double fitness = (double)INT32_MAX;
    if (!type)  fitness = 0;
	int index = 0;
	for (int i = 0; i < TOURNAMENT_SIZE; i++) {
		int ind = rand() % POPULATION_SIZE;
        if(individuals[ind].fitness == -1){
            i--;
            continue;
        }
		if ((individuals[ind].fitness < fitness)==type) {  //check tournament type
			fitness = individuals[ind].fitness;
			index = ind;
		}
	}
	return index;
}

/*
 * The crossover method returns a new individual
 * product of crossing the genes of parent p1 and
 * parent p2. It uses the box crossover method
 */
INDIVIDUAL crossOver(int p1, int p2){
    INDIVIDUAL ind;
    double random;
    for(int i=0; i<6; i++){
        random = ((double)rand()/RAND_MAX);
        ind.parameters[i] = individuals[p1].parameters[i]*random + individuals[p2].parameters[i]*(1-random);
    }
    //minTack cannot be smaller than maxTack and they cannot be equal
    while((ind.parameters[3] > ind.parameters[4]) || (ind.parameters[3] == ind.parameters[4])){
        random = ((double)rand()/RAND_MAX);
        ind.parameters[3] = individuals[p1].parameters[3]*random + individuals[p2].parameters[3]*(1-random);
        ind.parameters[4] = individuals[p1].parameters[4]*random + individuals[p2].parameters[4]*(1-random);
    }
    return ind;
}

/*
 * The mutate method returns a new individual
 * product of mutating the genes of the parent
 */
INDIVIDUAL mutate(int parent){
    INDIVIDUAL ind;
    double random;

    //A different sigma is used for angles and tacks
    double sigmaAngle = 20;
    double sigmaTack = 500;
    for(int i =0; i<3; i++){
        do{ //parameters cannot be negative
            random = ((double)rand()/RAND_MAX);
            random = -1.0 + random*2.0;
            ind.parameters[i] = individuals[parent].parameters[i]+sigmaAngle*random;
        }while(ind.parameters[i]<=0);
    }
    for(int i = 3; i<5; i++){
        do{ //parameters cannot be negative
            random = ((double)rand()/RAND_MAX);
            random = -1.0 + random*2.0;
            ind.parameters[i] = individuals[parent].parameters[i]+sigmaTack*random;
        }while(ind.parameters[i]<=0);
    }
    //minTack cannot be smaller than maxTack and they cannot be equal
    while((ind.parameters[3] > ind.parameters[4]) || (ind.parameters[3] == ind.parameters[4]) || ind.parameters[3] <=0 || ind.parameters[4] <= 0){
        random = ((double)rand()/RAND_MAX);
        random = -1.0 + random*2.0;
        ind.parameters[3] = individuals[parent].parameters[3]+sigmaTack*random;
        random = ((double)rand()/RAND_MAX);
        random = -1.0 + random*2.0;
        ind.parameters[4] = individuals[parent].parameters[4]+sigmaTack*random;
    }
    do{ //parameters cannot be negative
       random = ((double)rand()/RAND_MAX);
       random = -1.0 + random*2.0;
       ind.parameters[5] = individuals[parent].parameters[5]+sigmaAngle*random;
    }while(ind.parameters[5]<=0);
    return ind;
}

/*
 * This methid generates an offspring.
 * It decides randomly over generating
 * it by crossover or mutation
 */
int generateOffspring(){
    double random = ((double)rand() / RAND_MAX);
    INDIVIDUAL ind;
    int index;

    if(random < XOVER_RATE){ //crossover
        //pick two parents
        int parent1 = tournament(true);
        int parent2 = tournament(true);

        ind = crossOver(parent1, parent2);

        index = tournament(false);
        avgFitness = avgFitness - (individuals[index].fitness/((double)POPULATION_SIZE));
        individuals[index] = ind;
    }
    else{                       //mutation
        //pick one parent
        index = tournament(true);

        ind = mutate(index);
        index = tournament(false);
        avgFitness = avgFitness - (individuals[index].fitness/((double)POPULATION_SIZE));
        individuals[index] = ind;
    }
    return index;
}

/*
 * This method initiates the initial vectors:
 * Boats, targets and tide.
 */
void initVectors(int ammountIndividuals){
    timeStep=0;
    physVector pos;
    boats.clear();

    wind[0].setComponent(0, 0.0);
    wind[0].setComponent(1, 0.0);
    wind[0].setComponent(2, 10.0);

    wind[1].setComponent(0, 0.0);
    wind[1].setComponent(1, 0.0);
    wind[1].setComponent(2, -10.0);

    wind[2].setComponent(0, 10.0);
    wind[2].setComponent(1, 0.0);
    wind[2].setComponent(2, 0);

    wind[3].setComponent(0, 7.07106);
    wind[3].setComponent(1, 0.0);
    wind[3].setComponent(2, 7.07106);

    tide[0].setComponent(0, 0.0);
    tide[0].setComponent(1, 0.0);
    tide[0].setComponent(2, -0.0007);

    tide[1].setComponent(0, 0.0);
    tide[1].setComponent(1, 0.0);
    tide[1].setComponent(2, 0.0007);

    tide[2].setComponent(0, 0.0007);
    tide[2].setComponent(1, 0.0);
    tide[2].setComponent(2, 0.0);

    tide[3].setComponent(0, 0.0004);
    tide[3].setComponent(1, 0.0);
    tide[3].setComponent(2, 0.0004);

    //generate random original positions
    double x, y, ang;
    for(int i=0; i<ammountIndividuals; i++){
        for(int j=0; j<4; j++){
            for (int z = 0; z<4; z++){
                for(int k =0; k<4; k++){
                    boat b(wind[z], tide[k], 0.0, 0.0, 270.0, sailingPoints[j], speedPoints[j], i+(RUN_SIZE*runIndex), 5.0);
                    boats.push_back(b);
                }
            }
        }
    }

    boat base(wind[0], tide[0], 0.0, 0.0, 270.0, sailingPoints[1], speedPoints[0], 0, 5.0);
    boats.push_back(base);


    targets[0].setComponent(0,0.0);
    targets[0].setComponent(2,-125.0);
    targets[0].setComponent(1, 0.5);

    targets[1].setComponent(0,0);
    targets[1].setComponent(2,125.0);
    targets[1].setComponent(1,0.5);

    north.setComponent(0, 1);
}


void changeSize(int w, int h){
// Prevent a divide by zero, when window is too short
// (you cant make a window of zero width).
    if (h == 0)
        h = 1;
    float ratio = w * 1.0 / h;

    // Use the Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset Matrix
    glLoadIdentity();

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // Set the correct perspective.
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    // Get Back to the Modelview
    glMatrixMode(GL_MODELVIEW);
}

/*
 * This method draws the boat
 */
void drawBoat(int boatIndex){
    physVector pos = boats[boatIndex].getPosition();
    double ang = 360-boats[boatIndex].getDirection(); //rotate angle to the coordinate system used by opengl
    glLineWidth(2);
    glPushMatrix();
        //hull
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1), pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(1.2, .8, 0.5);
            glColor3d(0, 0, 0);
            glutWireCube(4);
            glColor3d(0.55,0.35, 0);
            glutSolidCube(4);
        glPopMatrix();

        //mast
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1)+2.5, pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(0.3, 6.0, 0.3);
            glColor3d(0, 0, 0);
            glutWireCube(0.8);
            glColor3d(0.65,0.35,0);
            glutSolidCube(0.8);
        glPopMatrix();

        //cloth sail
        double movX, movZ;
        movX = 3.9*cos(ang*M_PI/180.0);
        movZ = -3.9*sin(ang*M_PI/180.0);
        double windAngle = (boats[boatIndex].getWind()*-1)%north;
        double BoatWindAngle = abs(windAngle - boats[boatIndex].getDirection());
        double rot = BoatWindAngle/2.0;
        glPushMatrix();
            glTranslated(pos.getComponent(0)+movX, pos.getComponent(1)+3.9, pos.getComponent(2)+movZ);
            glRotated(ang+90, 0, 1, 0);

            if(boatIndex==boats.size()-1) glColor3d(0.0,0.0,0.0);
            else    glColor3d(colors[boatIndex/64][0],colors[boatIndex/64][1],colors[boatIndex/64][2]);
            
            glBegin(GL_TRIANGLE_STRIP);
                glVertex3f(0.0f, 1.0f, -4.0f);    // lower left vertex
                glVertex3f(0.0f, -1.6f, -4.0f);    // lower right vertex
                glVertex3f(1.5f,  -1.6f, -4.0f);    // upper vertex
                glVertex3f(0.0f, 1.0f, -3.9f);    // lower left vertex
                glVertex3f(0.0f, -1.6f, -3.9f);    // lower right vertex
                glVertex3f(1.5f,  -1.6f, -3.9f);    // upper vertex
            glEnd();

            glColor3d(0, 0, 0);
                glBegin(GL_LINE_STRIP);
                glVertex3f(0.0f, 1.0f, -4.0f);    // lower left vertex
                glVertex3f(0.0f, -1.6f, -4.0f);    // lower right vertex
                glVertex3f(1.5f,  -1.6f, -4.0f);    // upper vertex
                glVertex3f(0.0f, 1.0f, -3.9f);    // lower left vertex
                glVertex3f(0.0f, -1.6f, -3.9f);    // lower right vertex
                glVertex3f(1.5f,  -1.6f, -3.9f);    // upper vertex
            glEnd();
        glPopMatrix();

        //rudder
        movX = 2.3*cos(boats[boatIndex].getDirection()*M_PI/180.0);
        movZ = 2.3*sin(boats[boatIndex].getDirection()*M_PI/180.0);
        double rudderAngle = boats[boatIndex].getRudder();
        rudderAngle -= 90;
        glPushMatrix();
            glTranslated(pos.getComponent(0)-movX, pos.getComponent(1)-0.5, pos.getComponent(2)-movZ);
            glRotated(ang+rudderAngle, 0, 1, 0);
            glColor3d(0,0,0);
            glScalef(1.8, 1.2, 0.3);
            glutWireCube(1);
            glColor3d(1, 1, 1);
            glutSolidCube(1);
        glPopMatrix();

        //cabin at the back of the boat
        movX = 1.8*cos(boats[boatIndex].getDirection()*M_PI/180.0);
        movZ = 1.8*sin(boats[boatIndex].getDirection()*M_PI/180.0);
        glPushMatrix();
            glTranslated(pos.getComponent(0)-movX, pos.getComponent(1)+1.5, pos.getComponent(2)-movZ);
            glRotated(ang, 0, 1, 0);
            glScalef(0.6, 0.8, 1.0);
            glColor3d(0,0,0);
            glutWireCube(1.9);
            glColor3d(0.65, 0.35, 0);
            glutSolidCube(1.9);
        glPopMatrix();

        //windows at the side of the cabin
        for(int i=0; i<2; i++){
            double h = pow(pow(2.0-(double)i*0.5, 2)+pow(0.8,2), 0.5);
            double theta = asin(0.8/h);

            //cout<<h<<"   "<<theta<<endl;
            movX = h*cos((boats[boatIndex].getDirection())*M_PI/180.0 + theta);
            movZ = h*sin((boats[boatIndex].getDirection())*M_PI/180.0 + theta);
            glPushMatrix();
                glTranslated(pos.getComponent(0)-movX, pos.getComponent(1)+1.9, pos.getComponent(2)-movZ);
                glRotated(ang, 0, 1, 0);
                glColor3d(1, 1, 1);
                glutSolidSphere(0.2, 20, 20);
            glPopMatrix();
        }

        for(int i=0; i<2; i++){
            double h = pow(pow(2.0-(double)i*0.5, 2)+pow(0.8,2), 0.5);
            double theta = asin(0.8/h);
            //cout<<h<<"   "<<theta<<endl;
            movX = h*cos((boats[boatIndex].getDirection())*M_PI/180.0 - theta);
            movZ = h*sin((boats[boatIndex].getDirection())*M_PI/180.0 - theta);
            glPushMatrix();
                glTranslated(pos.getComponent(0)-movX, pos.getComponent(1)+1.9, pos.getComponent(2)-movZ);
                glRotated(ang, 0, 1, 0);
                glColor3d(1, 1, 1);
                glutSolidSphere(0.2, 20, 20);
            glPopMatrix();
        }

        //window at the back and front of the cabin
        movX = 1.8*cos(boats[boatIndex].getDirection()*M_PI/180.0);
        movZ = 1.8*sin(boats[boatIndex].getDirection()*M_PI/180.0);
        glPushMatrix();
            glTranslated(pos.getComponent(0)-movX, pos.getComponent(1)+1.9, pos.getComponent(2)-movZ);
            glRotated(ang, 0, 1, 0);
            glScalef(0.6, 0.2, 0.6);
            glColor3d(0,0,0);
            glutWireCube(1.95);
            glColor3d(1, 1, 1);
            glutSolidCube(1.95);
        glPopMatrix();

        //front of the boat
        movX = 2.7*cos(boats[boatIndex].getDirection()*M_PI/180.0);
        movZ = 2.7*sin(boats[boatIndex].getDirection()*M_PI/180.0);
        glPushMatrix();
            glTranslated(pos.getComponent(0)+movX, pos.getComponent(1)+0.1, pos.getComponent(2)+movZ);
            glRotated(ang+45, 0, 1, 0);
            glScalef(1, 2.5, 1);
            glColor3d(0,0,0);
            glutWireCube(pow(2, 0.5));
            glColor3d(0.65, 0.35, 0);
            glutSolidCube(pow(2, 0.5));
        glPopMatrix();

        movX = 2*cos(boats[boatIndex].getDirection()*M_PI/180.0);
        movZ = 2*sin(boats[boatIndex].getDirection()*M_PI/180.0);
        glPushMatrix();
            glTranslated(pos.getComponent(0)+movX, pos.getComponent(1)-0.51, pos.getComponent(2)+movZ);
            glRotated(ang, 0, 1, 0);
            glScalef(0.7, 2.5, 1);
            glColor3d(0,0,0);
            glutWireCube(1.95);
            glColor3d(0.65, 0.35, 0);
            glutSolidCube(1.95);
        glPopMatrix();



    glPopMatrix();



}

/*
 * Base chasing method. The boat makes a
 * straight line towards the target
 */
void chaseTarget(int boatIndex){
    int targetIndex = boats[boatIndex].getTargetIndex();
    //move towards target
    //find the angle difference to the target
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
    physVector directionVector(3);
    directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
    directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
    double angleToTarget = directionVector&vectToTarget;

    //find closest direction to target
    int direction = directionVector|vectToTarget;

    //modify direction to reach target
    if(angleToTarget > -3 && angleToTarget < 3){    //if the boat's direction is close to the target's direction, reset rudder
        boats[boatIndex].setRudder(90);
    }
    if(direction == -1){
        //boat's direction is to the left, move towards the right
        boats[boatIndex].setRudder(boats[boatIndex].getRudder()+(0.5));
        boats[boatIndex].setSail(0.4);
    }
    else{
        //boat's direction is to the right, move towards the left
        boats[boatIndex].setRudder(boats[boatIndex].getRudder()-(0.5));
        boats[boatIndex].setSail(0.4);
    }
    if(angleToTarget > -10 && angleToTarget <10){
        boats[boatIndex].setSail(1);
    }
}

/*
 * This method implements basic tacking.
 */
void baseTacking(int boatIndex){
    /*
     * 1.- Turn towards target
     * 2.- Pick a random side and commit to a tack
     * 3.- Evaluate after tack, if target is still upwind, tack to the other side
     */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    double tackAngle = boats[boatIndex].getTackAngle();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();

    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
    physVector directionVector(3);
    directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
    directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
    double angleToTarget = directionVector&vectToTarget;
    physVector w = boats[boatIndex].getWind();
    double trueWindAngle = (w*-1)%north;
    int direction = directionVector|vectToTarget;

    if(tackStatus == 0){
        //turn towards target
        //find closest direction to target
        //modify direction to reach target
        if(angleToTarget > -2 && angleToTarget < 2){
            boats[boatIndex].setRudder(90);
            //pick a side at random
            tackStatus = rand()%2 + 1;
            if(tackStatus==1){
                tackAngle = trueWindAngle + boats[boatIndex].getSailingPoint(0) + 5.0;
                if(tackAngle>=360.0){
                    tackAngle = tackAngle - 360.0;
                }
            }
            else{
                tackAngle = trueWindAngle - boats[boatIndex].getSailingPoint(0) - 5.0;
                if(tackAngle<0.0){
                    tackAngle = 360+tackAngle;
                }
            }
        }
        else{
            boats[boatIndex].setSail(0.2); //reduce speed while turning
            //face target
            if(direction == -1){
                //boat's direction is to the left, move towards the right
                boats[boatIndex].setRudder(boats[boatIndex].getRudder() + 0.5);
            }
            else{
                //boat's direction is to the right, move towards the left
                boats[boatIndex].setRudder(boats[boatIndex].getRudder() - 0.5);
            }
        }
    }
    if(tackTimer == tackLimit){
        double trueTargetAngle = (vectToTarget)%north;
        double angleDifference = abs(trueWindAngle-trueTargetAngle);
        if(angleDifference>180.0){
            angleDifference = 360-angleDifference;
        }
        if(angleDifference >=30.0){ //boat can sail towards target
            tackStatus = 0;
            tackTimer = 0;
            tackLimit = 300;
            chaseTarget(boatIndex);
        }
        else{                       //target is still upwind
            tackLimit = 600;
            tackTimer = 0;
            if(tackStatus == 1){
                tackStatus = 2;
                tackAngle = trueWindAngle-boats[boatIndex].getSailingPoint(0) - 5.0;
                if(tackAngle < 0.0){
                    tackAngle = 360.0 + tackAngle;
                }

            }
            else{
                tackStatus = 1;
                tackAngle = trueWindAngle + boats[boatIndex].getSailingPoint(0) + 5.0;
                if(tackAngle >=360.0){
                    tackAngle = tackAngle-360.0;
                }
            }
        }
    }
    if(tackStatus == 1){
        //turn until facing direction
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3); //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.5);
        }
        else{   //keep tack for a while
            boats[boatIndex].setSail(1);
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    else if(tackStatus == 2){
        if(abs(tackAngle- boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3);  //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.5);
        }
        else{   //keep tack for a while
            boats[boatIndex].setSail(1);
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    boats[boatIndex].setTackAngle(tackAngle);
    boats[boatIndex].setTackTimer(tackTimer);
    boats[boatIndex].setTackLimit(tackLimit);
    boats[boatIndex].setTackStatus(tackStatus);
}

void tackManager(int boatIndex){
    /*
    Check what side to start tacking (closest one to boat's direction)
    From there, check what angle would give the best speed
    Follow that until tack limit, repeat
    */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    double tackAngle = boats[boatIndex].getTackAngle();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();


    if(tackStatus == 0){
        //obtain side
        long long it = 0;
        int bestTack = 0;
        double targetAngle = (targets[targetIndex])%north;
        int pilot = boats[boatIndex].getPilot();
        int minAngle = targetAngle - individuals[pilot].parameters[0];
        int maxAngle = targetAngle + individuals[pilot].parameters[1];
        int angleStep = individuals[pilot].parameters[2];
        int minTack = individuals[pilot].parameters[3];
        int maxTack = individuals[pilot].parameters[4];
        int tackStep = individuals[pilot].parameters[5];
        tackAngle = boats[boatIndex].bestAngle(minAngle, maxAngle, angleStep, minTack, maxTack, tackStep, targets[targetIndex], it, bestTack);
        individuals[pilot].iterations = it;

        tackLimit = bestTack;
        tackStatus = 1;
    }
    if(tackTimer > tackLimit){
        tackTimer = 0;
        //change tack
        tackStatus = 0;
    }
    if(tackStatus == 1){
        //turn until
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1){
            physVector directionVector(3);
            directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
            directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
            physVector wantedDirection(3);
            wantedDirection.setComponent(0, cos((tackAngle/180.0)*M_PI));
            wantedDirection.setComponent(2, sin((tackAngle/180.0)*M_PI));

            //find closest direction to target
            int direction = directionVector|wantedDirection;
            boats[boatIndex].setSail(0.4);

            if(direction == -1){
                //boat direction is to the left

                boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.8);

            }
            else{
                boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.8);
            }
        }
        else{   //keep tack for a while
            boats[boatIndex].setSail(1);
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    boats[boatIndex].setTackAngle(tackAngle);
    boats[boatIndex].setTackTimer(tackTimer);
    boats[boatIndex].setTackLimit(tackLimit);
    boats[boatIndex].setTackStatus(tackStatus);
}

void advancedMovement(int boatIndex){
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();

    physVector w = boats[boatIndex].getWind();
    double trueWindAngle = (w*-1)%north;
    double trueTargetAngle = (vectToTarget)%north;
    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
        boats[boatIndex].completedLoop(timeStep);
        boats[boatIndex].setLastLoop(timeStep);
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }
    if(angleDifference >=35.0 || vectToTarget.getMagnitude() < 10.0){
        tackStatus = 0;
        //boat can sail towards target
        chaseTarget(boatIndex);
        boats[boatIndex].setTackStatus(tackStatus);
    }
    else{
        tackManager(boatIndex);
    }
}

void ruleSet(int boatIndex){
    /*
     * 1.- Understand the situation:
     *          Distance to target
     *          Angle to target
     *          Direction of the wind
     * 2.- Depending on direction of the wind:
     *          If the desired direction is inside
     *              the no-go zone, then the boat
     *              must tack to reach its destination
     *          If the desired direction is not inside
     *              the no-go zone, then a direct
     *              approach can be used.
     */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();
    //obtain true wind angle
    physVector w = boats[boatIndex].getWind();
    double trueWindAngle = (w*-1)%north;
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
            int loopCount = boats[boatIndex].getLoopCount();
            boats[boatIndex].completedLoop(timeStep);
            boats[boatIndex].setLastLoop(timeStep);
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }

    //obtain vectToTarget angle
    double trueTargetAngle = (vectToTarget)%north;

    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(angleDifference>180.0){
        angleDifference = 360-angleDifference;
    }

    if(angleDifference >=30.0 && tackStatus == 0){
        tackStatus = 0;
        tackTimer = 0;
        tackLimit = 300;
        //boat can sail towards target
        chaseTarget(boatIndex);
        boats[boatIndex].setTackStatus(tackStatus);
        boats[boatIndex].setTackTimer(tackTimer);
        boats[boatIndex].setTackLimit(tackLimit);
    }
    else{
        //If the angle towards the wind is lower than 30, tack
        if(tackStatus == 0){
            originalHeading = trueTargetAngle;
        }
        baseTacking(boatIndex);
    }
}

static void display(void){
    if(timeStep >= 18000){
        if(GUI) glutLeaveMainLoop();
    }
    auto now = chrono::steady_clock::now();
    chrono::duration<double> diff = now-start;

    //60 FPS
    if(diff.count() >  1.0/FPS){
        if(GUI){
            glMatrixMode (GL_PROJECTION);
            glLoadIdentity ();
            glFrustum (-1.0, 1.0, -1.0, 1.0, 1.5, 2000.0);
            glMatrixMode(GL_MODELVIEW);
            const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
            const double a = t*90.0;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glLoadIdentity();
            // Set the camera
            gluLookAt(	x, 10.0f+cy, z,
            x+lx, 10.0f, z+lz,
            0.0f, 1.0f, 0.0f);

            // Draw ground

            glClearColor(1, 1, 1, 1);
            glBegin(GL_QUADS);
                glColor3d(0, 0, 0.5);
                glVertex3f(-1000.0f, -0.05f, -1000.0f);
                glVertex3f(-1000.0f, -0.05f, 1000.0f);
                glVertex3f( 1000.0f, -0.05f, 1000.0f);
                glVertex3f( 1000.0f, -0.05f, -1000.0f);
            glEnd();

            //draw targets
            for(int i=0; i<2; i++){
                glPushMatrix();
                    glTranslated(targets[i].getComponent(0), targets[i].getComponent(1), targets[i].getComponent(2));
                    glColor3d(0, 0, 0);
                    glutWireSphere(1, 10, 10);
                    glColor3d(1, (double)i*0.25, (double)i*0.2);
                    glutSolidSphere(1, 10, 10);
                glPopMatrix();
            }
        }
        if(timeStep%600==0 && racing){
            cout<<"\nRace stats: \n";
            int racers = (boats.size()-1)/64;
            for(int i =0; i< racers; i++){
                double targetsReached = 0;
                for(int j =0; j<64; j++){
                    targetsReached += boats[j+64*i].getLoopCount();
                }
                cout<<"Pilot "<<i+1<<" average targets reached at "<<timeStep/60<<" seconds into the run: "<<targetsReached/64<<endl; 
            }
            cout<<"Base pilot targets reached at "<<timeStep/60<<" seconds into the run: "<<boats[boats.size()-1].getLoopCount()<<endl;
        }
        for(int i =0; i<boats.size(); i++){
            if(i<boats.size()-1)
               advancedMovement(i);
            else
                ruleSet(i);

            boats[i].moveBoat(timeStep);
            if(GUI) drawBoat(i);
        }
        if(GUI) glutSwapBuffers();
        timeStep++;
	    start = chrono::steady_clock::now();
    }
}


static void key(unsigned char key, int xx, int yy){
    float fraction = 0.1f;
    switch (key){
        case 27 :
        case 'q':
            exit(0);
            break;
    }

    glutPostRedisplay();
}

static void idle(void){
    glutPostRedisplay();
}

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void processSpecialKeys(int key, int xx, int yy){

    float fraction = 2.0f;

    switch (key) {
        case GLUT_KEY_LEFT :
            angle -= 0.05f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_RIGHT :
            angle += 0.05f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_UP :
            x += lx * fraction;
            z += lz * fraction;
            //cy += fraction/10;
            break;
        case GLUT_KEY_DOWN :
            x -= lx * fraction;
            z -= lz * fraction;
            //cy -=fraction/10;
            break;
    }
}


void initAndRun(int argc, char *argv[]){
    glutInit(&argc, argv);
    glutInitWindowSize(1024,768);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("GLUT Shapes");

    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(changeSize);
    glutSpecialFunc(processSpecialKeys);
    glutIdleFunc(idle);


    glClearColor(1.0,1,1,1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();
}

 void printIndividualsToFile(int c){
    ostringstream strs;
	strs << c;
    string file = "individuals"+strs.str()+".txt";
    FILE *fileHandler;
    fileHandler = fopen(file.c_str(), "w");
    fprintf(fileHandler, "Ind#\t\tFitness\t\tIterations\t\tLoops\t\tParameters\n");
    for(int i=0; i<POPULATION_SIZE; i++){
        fprintf(fileHandler, "%d\t\t%f\t\t%d\t\t%d", i, individuals[i].fitness, individuals[i].iterations, individuals[i].loops);
        for(int j =0; j<6; j++){
            fprintf(fileHandler, "\t\t%d", individuals[i].parameters[j]);
        }
        fprintf(fileHandler,"\n");
    }
    fprintf(fileHandler, "Normal Boat-> Finish time: %d\tLoops count: %d", boats[RUN_SIZE*4].getFinishTime(), boats[RUN_SIZE*4].getLoopCount());
    fprintf(fileHandler,"\n");
    fclose(fileHandler);
 }

void train(int argc, char *argv[]){
    double bestFitness = (double)INT32_MAX;
    int bestIndex =0;
    for(; runIndex<(POPULATION_SIZE/RUN_SIZE); runIndex++){
        cout<<"Run: "<<runIndex<<endl;
        initVectors(RUN_SIZE);
        if(GUI) initAndRun(argc, argv);
        else{
            timeStep = 0;
            while(timeStep <=18000){
                display();
            }
        }
        for(int i=0; i<RUN_SIZE; i++){
            double fitness = 0;
            int loops = 0;
            for(int j=0; j<64; j++){
                fitness+=boats[i+j].getFinishTime();
                loops+=boats[i+j].getLoopCount();
            }
            individuals[i+runIndex*RUN_SIZE].loops = loops;
            individuals[i+runIndex*RUN_SIZE].fitness = fitness/64.0 + 0.1*((double)individuals[i+runIndex*RUN_SIZE].iterations);
            avgFitness+=individuals[i+runIndex*RUN_SIZE].fitness;
            if(individuals[i+runIndex*RUN_SIZE].fitness < bestFitness){
                bestIndex = i+runIndex*RUN_SIZE;
                bestFitness = individuals[i+runIndex*RUN_SIZE].fitness;
            }
        }
    }
    avgFitness = avgFitness/((double)POPULATION_SIZE);
    printIndividualsToFile(0);
    FILE *fp;
    for(int i=0; i<GENERATIONS; i++){
        cout<<"Generation: "<<i<<endl;
        //generate offspring
        initVectors(RUN_SIZE);
        int pilot[RUN_SIZE];
        for(int j =0; j<RUN_SIZE; j++){
            pilot[j] = generateOffspring();
            for(int l=0; l<64; l++){
                boats[l+(j*64)].setPilot(pilot[j]);
            }
        }
        //finished generating offspring
        cout<<"Finished generating offspring\n";
        if(GUI) initAndRun(argc, argv);
        else{
            while(timeStep <=18000){
                display();
            }
        }
        //checkFitness
        for(int l =0; l<RUN_SIZE; l++){
            double fitness = 0;
            int loops = 0;
            for(int j=0; j<64; j++){
                fitness += boats[l+j].getFinishTime();
                loops += boats[l+j].getLoopCount();
            }
            individuals[pilot[l]].loops = loops;
            individuals[pilot[l]].fitness = fitness/64.0 + 0.1*((double)individuals[pilot[l]].iterations);
            if(individuals[pilot[l]].fitness < bestFitness){
                bestIndex = pilot[l];
                bestFitness = individuals[pilot[l]].fitness;
                cout<<"MaxFitness: "<<bestFitness<<endl;
            }
            avgFitness = avgFitness + individuals[pilot[l]].fitness/((double)POPULATION_SIZE);
        }
        fp = fopen("generations.txt", "a");
        fprintf(fp, "Generation: %d, MaxFitness: %f, AVGFitness: %f\n", i, bestFitness, avgFitness);
        fclose(fp);
        printIndividualsToFile(i+1);
    }

    //check fitness

    for(int i=0; i<POPULATION_SIZE; i++){
        cout<<i<<" Fitness: "<<individuals[i].fitness<<" Iterations: "<<individuals[i].iterations<<endl;
        cout<<"MinAngle: "<<individuals[i].parameters[0]<<" MaxAngle: "<<individuals[i].parameters[1]<<" AngleStep: "<<individuals[i].parameters[2]<<endl;
        cout<<"MinTack: "<<individuals[i].parameters[3]<<" MaxTack: "<<individuals[i].parameters[4]<<" TackStep: "<<individuals[i].parameters[5]<<endl<<endl;
    }

}

void menu(int argc, char *argv[]){
    string ans;
    cout<<"Would you like to train pilots or race pilots? (t for training, r for racing): ";
    cin>>ans;
    if(toupper(ans[0])=='T'){
        //traing
        cout<<"Would you like to run the GUI? (y/n): ";
        cin>>ans;
        if(toupper(ans[0])=='Y'){
            GUI = true;
        }
        else{
            GUI = false;
        }

        cout<<"Would you like to run with the default parameters? (y/n): ";
        cin>>ans;
        if(toupper(ans[0])=='N'){
            //ask for new parameters
            cout<<"How many individuals do you want for the initial population? ";
            cin>>POPULATION_SIZE;
            cout<<"For how many generations do you want to run the GA? ";
            cin>>GENERATIONS;
        }
        train(argc, argv);
    }
    else if (toupper(ans[0])=='R'){
        //race
        int racers;
        cout<<"How many individuals would you like to race? (Maximum 5) ";
        cin>>racers;
        if(racers > 5){
            cout<<"Invalid number of racers.\n";
            exit(0);
        }
        cout<<"Please type the parameters for the racers\n";
        for(int i=0; i<racers; i++){
            cout<<"MinAngle: ";
            cin>>individuals[i].parameters[0];
            cout<<"MaxAngle: ";
            cin>>individuals[i].parameters[1];
            cout<<"AngleStep: ";
            cin>>individuals[i].parameters[2];
            cout<<"MinTack: ";
            cin>>individuals[i].parameters[3];
            cout<<"MaxTack: ";
            cin>>individuals[i].parameters[4];
            cout<<"tackStep: ";
            cin>>individuals[i].parameters[5];
        }
        initVectors(racers);
        GUI = true;
        racing = true;
        cout<<"\nEach pilot is assigned a color. Pilot 1 has red sails, ";
        cout<<"Pilot 2 has green sails,\nPilot 3 has yellow sails,";
        cout<<"Pilot 4 has white sails and Pilot 5 has purple sails\n";
        cout<<"Base pilot has black sails\n";
        cout<<"To exit the simulation press the letter 'q'\n";
        initAndRun(argc, argv);
    }
    else{
        cout<<"Not a valid option, exiting program...\n";
        exit(0);
    }
}

int main(int argc, char *argv[]){
    menu(argc, argv);

    return EXIT_SUCCESS;
}
