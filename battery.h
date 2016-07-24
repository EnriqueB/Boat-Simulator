#ifndef BATTERY_H
#define BATTERY_H

class battery{
    private:
        double charge;
        double maxCharge;

    public:
        battery();
        battery(double max);
        battery(double ch, double max);

        //sets
        void setMax(double max);
        void setCharge(double ch);

        //gets
        double getCharge();
        double getMaxCharge();

        bool use(double ammount);

        void addCharge(double ammount);
};

battery::battery(){
    charge=0;
    maxCharge=0;
}

battery::battery(double max){
    charge = 0;
    maxCharge = max;
}

battery::battery(double ch, double max){
    charge = ch;
    maxCharge = max;
}

void battery::setMax(double max){
    maxCharge = max;
}

void battery::setCharge(double ch){
    charge = ch;
}

double battery::getCharge(){
    return charge;
}

double battery::getMaxCharge(){
    return maxCharge;
}


bool battery::use(double ammount){
    if(charge-ammount>=0){
        charge -= ammount;
        return true;
    }
    else{
        return false;
    }
}

void battery::addCharge(double ammount){
    charge = (charge + ammount)>=maxCharge?maxCharge:charge+ammount;
}
#endif
