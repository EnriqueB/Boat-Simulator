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

#define FPS 60.0

#define POPULATION_SIZE 100
#define TOURNAMENT_SIZE 5
#define GENERATIONS 5
#define XOVER_RATE 0.9

using namespace std;

//Clock used for enforcing FPS
auto start = chrono::steady_clock::now();

// actual vector representing the camera's direction
float lx=0.0f,lz=-1.0f, cy=0;
// XZ position of the camera
float x=0.0f,z=100.0f;

float red=1.0f, blue=1.0f, green=1.0f;

// angle for rotating triangle
float angle = 0.0f;
float acceleration;
float accelerationVal;

vector <boat> boats;
physVector tide(3);
physVector targets [4];
physVector north(3);

bool GUI = false;

int runIndex =0;

long long timeStep;
long long lastLoop = 0;
double w[] = {0.0, 0.0, 10.0};
physVector wind(3, w);
double originalHeading;

double sailingPoints[][3] = {{10.0,  85.0, 95.0},
                                {15.0, 85.0, 95.0},
                                {20.0, 80.0, 100.0},
                                {25.0, 85.0, 95.0}};
double speedPoints[][3] = {{0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8}};
bool usedSrand = false;
struct INDIVIDUAL{
    int iterations;
    int loops;
    double fitness;
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
        fitness = 0;
        iterations = 0;
        loops=0;
        parameters[0] = rand()%180;
        parameters[1] = rand()%180;
        parameters[2] = rand()%50 + 1;
        parameters[3] = rand()%100 + 1;
        parameters[4] = rand()%3000 + 1;
        parameters[5] = rand()%100 + 1;

        while((parameters[3] > parameters[4]) || (parameters[3] == parameters[4])){
            parameters[3] = rand()%100 +1;
            parameters[4] = rand()%3000 +1;
        }
    }
};

vector <INDIVIDUAL> individuals(POPULATION_SIZE);

int tournament(bool type){
    //Tournament selection
    //type = true for normal, false for negative tournament
	double fitness = (double)INT32_MAX;
    if(type)fitness = 0;
	int index = 0;
	for (int i = 0; i < TOURNAMENT_SIZE; i++) {
		int ind = rand() % POPULATION_SIZE;
        //check tournament type
		if ((individuals[ind].fitness> fitness)==type) {
			fitness = individuals[ind].fitness;
			index = ind;
		}
	}
	return index;
}

INDIVIDUAL crossOver(int p1, int p2){
    INDIVIDUAL ind;
    double random;
    for(int i=0; i<6; i++){
        random = ((double)rand()/RAND_MAX);
        ind.parameters[i] = individuals[p1].parameters[i]*random + individuals[p2].parameters[i]*(1-random);
    }
    while((ind.parameters[3] > ind.parameters[4]) || (ind.parameters[3] == ind.parameters[4])){
        random = ((double)rand()/RAND_MAX);
        ind.parameters[3] = individuals[p1].parameters[3]*random + individuals[p2].parameters[3]*(1-random);
        ind.parameters[4] = individuals[p1].parameters[4]*random + individuals[p2].parameters[4]*(1-random);
    }

    return ind;
}

INDIVIDUAL mutate(int parent){
    INDIVIDUAL ind;
    double random;
    double sigmaAngle = 10;
    double sigmaTack = 100;
    for(int i =0; i<3; i++){
        do{
            random = ((double)rand()/RAND_MAX);
            random = -1.0 + random*2.0;
            ind.parameters[i] = individuals[parent].parameters[i]+sigmaAngle*random;
        }while(ind.parameters[i]<=0);
    }
    for(int i = 3; i<5; i++){
        do{
            random = ((double)rand()/RAND_MAX);
            random = -1.0 + random*2.0;
            ind.parameters[i] = individuals[parent].parameters[i]+sigmaTack*random;
        }while(ind.parameters[i]<=0);
    }
    while((ind.parameters[3] > ind.parameters[4]) || (ind.parameters[3] == ind.parameters[4])){
        random = ((double)rand()/RAND_MAX);
        random = -1.0 + random*2.0;
        ind.parameters[3] = individuals[parent].parameters[3]+sigmaTack*random;
        ind.parameters[4] = individuals[parent].parameters[4]+sigmaTack*random;
    }
    do{
       random = ((double)rand()/RAND_MAX);
       random = -1.0 + random*2.0;
       ind.parameters[5] = individuals[parent].parameters[5]+sigmaAngle*random;
    }while(ind.parameters[5]<=0);
    return ind;
}

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
        individuals[index] = ind;
    }
    else{                       //mutation
        //pick one parent
        index = tournament(true);

        ind = mutate(index);
        index = tournament(false);
        individuals[index] = ind;
    }
    return index;
}

void initVectors(int ammountIndividuals){
    timeStep=0;
    physVector pos;
    boats.clear();

    //generate random original positions
    double x, y, ang;
    for(int i=0; i<ammountIndividuals; i++){
        for(int j=0; j<4; j++){
        /*
            x= (double)rand() / RAND_MAX;               //HARDCODED
            x = -20 + x*40;
            y=(double)rand()/RAND_MAX;
            y = -20+y*40;
            cout<<x<<" "<<y<<endl;

            ang = (double)rand()/RAND_MAX;
            ang*=360.0;
        */
            boat b(0.0, 0.0, 270.0, sailingPoints[j], speedPoints[j], i+(50*runIndex));
            boats.push_back(b);
        }
    }

    boat base(0.0, 0.0, 270.0, sailingPoints[1], speedPoints[0], 0);
    boats.push_back(base);


    //generate tide
    /*
    tide.setComponent(0,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(2,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(1, 0);
    */
    //tide.setComponent(0, 0);
    tide.setComponent(2, -0.0005);
    //generate targets
    /*
    for(int i=0; i<2; i++){
        x = (double)rand()/RAND_MAX;
        x = -70 + x*140;
        y = (double)rand()/RAND_MAX;
        y = -70 + y*140;
        targets[i].setComponent(0, x);
        targets[i].setComponent(2, y);
        targets[i].setComponent(1, 1);
    }
    */
    targets[0].setComponent(0,0.0);
    targets[0].setComponent(2,-100.0);
    targets[0].setComponent(1, 0.5);

    targets[1].setComponent(0,0);
    targets[1].setComponent(2,100.0);
    targets[1].setComponent(1,0.5);
    /*
    targets[2].setComponent(0, 100);
    targets[2].setComponent(2, 0);
    targets[2].setComponent(1, 0.5);

    targets[3].setComponent(0, -100);
    targets[3].setComponent(2, 0);
    targets[3].setComponent(1, 0.5);
    north.setComponent(0, 1);
    */
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


void drawBoat(int boatIndex){
    physVector pos = boats[boatIndex].getPosition();
    double ang = 360-boats[boatIndex].getDirection();
    glPushMatrix();
        //glRotated(angle, 0, 1, 0);
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
            glTranslated(pos.getComponent(0), pos.getComponent(1)+2, pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(0.3, 5, 0.3);
            glColor3d(0, 0, 0);
            glutWireCube(0.8);
            glColor3d(0.65,0.35,0);
            glutSolidCube(0.8);
        glPopMatrix();

        //cloth sail
        glPushMatrix();
            glTranslated(pos.getComponent(0)+0.5, pos.getComponent(1)+3.6, pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(0.3, 0.9, 1);
            glColor3d(0, 0, 0);
            glutWireCube(0.8);
            if(boatIndex==boats.size()-1) glColor3d(0.0,0.0,0.0);
            else glColor3d(1,1,1);
            glutSolidCube(0.8);
        glPopMatrix();

        //sphere at the front of the boat
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1), pos.getComponent(2)+3);
            glRotated(ang, 0, 1, 0);
            glColor3d(0,0,0);
            //glutWireSphere(0.5, 20, 3);
            glColor3d(1, 0, 0);
            //glutSolidSphere(0.51, 20, 3);
        glPopMatrix();
        glTranslated(0,0,0);
    glPopMatrix();

}


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
    //if(timeStep%300 == 0)
     //   cout<<"Angle to target: "<<angleToTarget<<"   "<<direction<<"    ";
    //modify direction to reach target

    if(angleToTarget > -3 && angleToTarget < 3){
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

    //if(timeStep%300==0){
    //    cout<<"Tacking stat: "<<tackStatus<<" # ";
    //}

    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
    physVector directionVector(3);
    directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
    directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
    double angleToTarget = directionVector&vectToTarget;
    double trueWindAngle = (wind*-1)%north;
    int direction = directionVector|vectToTarget;

    if(tackStatus == 0){
        //turn towards target
        //find closest direction to target
        //if(timeStep%300 == 0)
        //    cout<<"Angle to target: "<<angleToTarget<<"  ";
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
        if(angleDifference >=30.0){
            tackStatus = 0;
            tackTimer = 0;
            tackLimit = 300;
            //boat can sail towards target
            chaseTarget(boatIndex);
        }
        else{
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
    if(tackStatus ==1){
        //turn until facing direction
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3); //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.5);
        }
        else{
            boats[boatIndex].setSail(1);
            //keep tack for a while
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    else if(tackStatus == 2){
        if(abs(tackAngle- boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3);  //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.5);
        }
        else{
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
        //cout<<"Begin tack\n";
        int iterations = 0;
        int bestTack = 0;
        //tackAngle = boats[boatIndex].bestAngle(wind, tide, 0, 359, 5, 10, 3000, 20, targets[targetIndex], iterations, bestTack);
        double targetAngle = (targets[targetIndex])%north;
        int pilot = boats[boatIndex].getPilot();
        int minAngle = targetAngle - individuals[pilot].parameters[0];
        int maxAngle = targetAngle + individuals[pilot].parameters[1];
        int angleStep = individuals[pilot].parameters[2];
        int minTack = individuals[pilot].parameters[3];
        int maxTack = individuals[pilot].parameters[4];
        int tackStep = individuals[pilot].parameters[5];
        tackAngle = boats[boatIndex].bestAngle(wind, tide, minAngle, maxAngle, angleStep, minTack, maxTack, tackStep, targets[targetIndex], iterations, bestTack);
        individuals[pilot].iterations = iterations;
        tackLimit = bestTack;
        //cout<<"Boat: "<<boatIndex<<" TackAngle: "<<tackAngle<<" Iterations: "<<iterations<<" BestTack: "<<bestTack<<endl;
        tackStatus = 1;
    }
    if(tackTimer > tackLimit){
        //cout<<"Finished tack\n";
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
        else{
            //keep tack for a while
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

    double trueWindAngle = (wind*-1)%north;
    double trueTargetAngle = (vectToTarget)%north;
    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
        if(targetIndex==0){
            int loopCount = boats[boatIndex].getLoopCount();
            cout<<"************************\n";
            cout<<"Boat: "<<boatIndex<<" ";
            cout<<"Loop count: "<<loopCount;
            cout<<" In: "<<(double)(timeStep-boats[boatIndex].getLastLoop())/FPS<<" seconds\n";
            cout<<"************************\n";
            boats[boatIndex].completedLoop();
            boats[boatIndex].setLastLoop(timeStep);
        }
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }
    if(angleDifference >=35.0){
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
    double trueWindAngle = (wind*-1)%north;
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
        if(targetIndex==0){
            int loopCount = boats[boatIndex].getLoopCount();
            cout<<"************************\n";
            cout<<"Boat: "<<boatIndex<<" ";
            cout<<"Loop count: "<<loopCount;
            cout<<" In: "<<(double)(timeStep-boats[boatIndex].getLastLoop())/FPS<<" seconds\n";
            cout<<"************************\n";
            boats[boatIndex].completedLoop();
            boats[boatIndex].setLastLoop(timeStep);
        }
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }

    //obtain vectToTarget angle
    double trueTargetAngle = (vectToTarget)%north;

    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(angleDifference>180.0){
        angleDifference = 360-angleDifference;
    }
    //if(timeStep%300==0){
       // cout<<"AngleDifference: "<<angleDifference<<"  ";
    //}

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

            //????
            //glColor3f(0,0,.8);
            glClearColor(0, 0, 1, 1);
            glBegin(GL_QUADS);
                glColor3d(0, 0, 0.6);
                glVertex3f(-1000.0f, -0.05f, -1000.0f);
                glVertex3f(-1000.0f, -0.05f, 1000.0f);
                glVertex3f( 1000.0f, -0.05f, 1000.0f);
                glVertex3f( 1000.0f, -0.05f, -1000.0f);
            glEnd();

            glColor3d(0.65,0.35,0);
            //physVector pos;

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
        for(int i =0; i<boats.size(); i++){
            //move and draw boats
            if(i<boats.size()-1)
               advancedMovement(i);
            else
                ruleSet(i);

            boats[i].moveBoat(wind, tide, timeStep);
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
            glutLeaveMainLoop();
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

    float fraction = 1.0f;

    switch (key) {
        case GLUT_KEY_LEFT :
            angle -= 0.01f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_RIGHT :
            angle += 0.01f;
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


    glClearColor(0.9,.9,.9,1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();
}

 //Program entry point
 void printIndividualsToFile(){
    FILE *fileHandler;
    fileHandler = fopen("individuals.txt", "w");
    fprintf(fileHandler, "Ind#\tFitness\tIterations\tLoops\tParameters\n");
    for(int i=0; i<POPULATION_SIZE; i++){
        fprintf(fileHandler, "%d\t\t%f\t\t%d\t\t%d", i, individuals[i].fitness, individuals[i].iterations, individuals[i].loops);
        for(int j =0; j<6; j++){
            fprintf(fileHandler, "\t\t%d", individuals[i].parameters[j]);
        }
        fprintf(fileHandler,"\n");
    }
    fclose(fileHandler);
 }

int main(int argc, char *argv[]){
    /*
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
    */

    double maxFitness = 0;
    int maxIndex =0;
    for(; runIndex<2; runIndex++){
        initVectors(50);
        if(GUI) initAndRun(argc, argv);
        else{
            timeStep = 0;
            while(timeStep <=18000){
                display();
            }
        }
        for(int i=0; i<50; i++){
            double fitness = 0;
            for(int j=0; j<4; j++){
                fitness+=(boats[i+j].getLoopCount()*10);
            }
            individuals[i+runIndex*50].loops = fitness/10;
            individuals[i+runIndex*50].fitness = fitness/4.0 - 0.001*((double)individuals[i].iterations);
            if(individuals[i+runIndex*50].fitness > maxFitness){
                maxIndex = i+runIndex*50;
                maxFitness = individuals[i+runIndex*50].fitness;
            }
        }
    }
    printIndividualToFile();
    FILE *fp;
    for(int i=0; i<GENERATIONS; i++){
        //generate offspring
        int pilot = generateOffspring();
        initVectors(1);
        for(int j=0; j<4; j++){
            boats[j].setPilot(pilot);
        }
        if(GUI) initAndRun(argc, argv);
        else{
            while(timeStep <=18000){
                display();
            }
        }
        //checkFitness
        double fitness = 0;
        for(int j=0; j<4; j++){
            fitness += (boats[j].getLoopCount()*10);
        }
        individuals[pilot].fitness = fitness/4.0 - 0.001*((double)individuals[i].iterations);
        if(individuals[pilot].fitness>maxFitness){
            maxIndex = pilot;
            maxFitness = individuals[pilot].fitness;
            cout<<"MaxFitness: "<<maxFitness<<endl;
        }
        fp = fopen("generations.txt", "a");
        fprintf(fp, "Generation: %d, MaxFitness: %f\n", i, maxFitness);
        fclose(fp);
        printIndividualToFile();
    }

    //check fitness

    for(int i=0; i<POPULATION_SIZE; i++){
        cout<<i<<" Fitness: "<<individuals[i].fitness<<" Iterations: "<<individuals[i].iterations<<endl;
        cout<<"MinAngle: "<<individuals[i].parameters[0]<<" MaxAngle: "<<individuals[i].parameters[1]<<" AngleStep: "<<individuals[i].parameters[2]<<endl;
        cout<<"MinTack: "<<individuals[i].parameters[3]<<" MaxTack: "<<individuals[i].parameters[4]<<" TackStep: "<<individuals[i].parameters[5]<<endl<<endl;
    }

    return EXIT_SUCCESS;
}

